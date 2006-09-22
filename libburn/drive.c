/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "libburn.h"
#include "drive.h"
#include "transport.h"
#include "message.h"
#include "debug.h"
#include "init.h"
#include "toc.h"
#include "util.h"
#include "sg.h"
#include "structure.h"
#include "back_hacks.h"

static struct burn_drive drive_array[255];
static int drivetop = -1;

int burn_drive_is_open(struct burn_drive *d);

/* ts A60904 : ticket 62, contribution by elmom */
/* splitting former burn_drive_free() (which freed all, into two calls) */
void burn_drive_free(struct burn_drive *d)
{
	if (d->global_index == -1)
		return;
	/* ts A60822 : close open fds before forgetting them */
	if (burn_drive_is_open(d))
		close(d->fd);
	free((void *) d->idata);
	free((void *) d->mdata);
	free((void *) d->toc_entry);
	free(d->devname);
	d->global_index = -1;
}

void burn_drive_free_all(void)
{
	int i;

	for (i = 0; i < drivetop + 1; i++)
		burn_drive_free(&(drive_array[i]));
	drivetop = -1;
	memset(drive_array, 0, sizeof(drive_array));
}


/* ts A60822 */
int burn_drive_is_open(struct burn_drive *d)
{
	/* a bit more detailed case distinction than needed */
	if (d->fd == -1337)
		return 0;
	if (d->fd < 0)
		return 0;
	return 1;
}


/* ts A60906 */
int burn_drive_force_idle(struct burn_drive *d)
{
	d->busy = BURN_DRIVE_IDLE;
	return 1;
}


/* ts A60906 */
int burn_drive_is_released(struct burn_drive *d)
{
        return !!d->released;
}


/* ts A60906 */
/** Inquires drive status in respect to degree of app usage.
    @param return -2 = drive is forgotten 
                  -1 = drive is closed (i.e. released explicitely)
                   0 = drive is open, not grabbed (after scan, before 1st grab)
                   1 = drive is grabbed but BURN_DRIVE_IDLE
                  10 = drive is grabbing (BURN_DRIVE_GRABBING)
                 100 = drive is busy in cancelable state
                1000 = drive is in non-cancelable state 
           Expect a monotonous sequence of usage severity to emerge in future.
*/
int burn_drive_is_occupied(struct burn_drive *d)
{
	if(d->global_index < 0)
		return -2;
	if(!burn_drive_is_open(d))
		return -1;
	if(d->busy == BURN_DRIVE_GRABBING)
		return 10;
	if(d->released)
		return 0;
	if(d->busy == BURN_DRIVE_IDLE)
		return 1;
	if(d->busy == BURN_DRIVE_READING || d->busy == BURN_DRIVE_WRITING)
		return 50;
	return 1000;
}


/*
void drive_read_lead_in(int dnum)
{
	mmc_read_lead_in(&drive_array[dnum], get_4k());
}
*/
unsigned int burn_drive_count(void)
{
	return drivetop + 1;
}

int burn_drive_grab(struct burn_drive *d, int le)
{
	int errcode;
	int was_equal = 0, must_equal = 3, max_loop = 20;

	/* ts A60907 */
	int loop_count, old_speed = -1234567890, new_speed = -987654321;
	int old_erasable = -1234567890, new_erasable = -987654321;

	if (!d->released) {
		burn_print(1, "can't grab - already grabbed\n");
		return 0;
	}
	errcode = d->grab(d);

	if (errcode == 0) {
		burn_print(1, "low level drive grab failed\n");
		return 0;
	}
	d->busy = BURN_DRIVE_GRABBING;

	if (le)
		d->load(d);

	d->lock(d);
	d->status = BURN_DISC_BLANK;
	if (d->mdata->cdr_write || d->mdata->cdrw_write ||
	    d->mdata->dvdr_write || d->mdata->dvdram_write) {

#ifdef Libburn_grab_release_and_grab_agaiN

		d->read_disc_info(d);

#else
		/* ts A60908 */
		/* Trying to stabilize the disc status after eventual load
		   without closing and re-opening the drive */
		/* This seems to work for burn_disc_erasable() .
		   Speed values on RIP-14 and LITE-ON 48125S are stable
		   and false, nevertheless. So cdrskin -atip is still
		   forced to finish-initialize. */
		/*
		fprintf(stderr,"libburn: experimental: read_disc_info()\n");
		*/
		for (loop_count = 0; loop_count < max_loop; loop_count++){
			old_speed = new_speed;
			old_erasable = new_erasable;

			d->read_disc_info(d);

			new_speed = burn_drive_get_write_speed(d);
			new_erasable = burn_disc_erasable(d);
		        if (new_speed == old_speed &&
			    new_erasable == old_erasable) {
				was_equal++;
				if (was_equal >= must_equal)
		break;
			} else
				was_equal = 0;
			/*
			if (loop_count >= 1 && was_equal == 0)
				fprintf(stderr,"libburn: experimental: %d : speed %d:%d   erasable %d:%d\n",
					loop_count,old_speed,new_speed,old_erasable,new_erasable);
			*/
			usleep(100000);
		}
#endif /* ! Libburn_grab_release_and_grab_agaiN */

	} else {
		d->read_toc(d);
	}
	d->busy = BURN_DRIVE_IDLE;
	return 1;
}

struct burn_drive *burn_drive_register(struct burn_drive *d)
{
#ifdef Libburn_ticket_62_re_register_is_possiblE
	int i;
#endif

	d->block_types[0] = 0;
	d->block_types[1] = 0;
	d->block_types[2] = 0;
	d->block_types[3] = 0;
	d->toc_temp = 0;
	d->nwa = 0;
	d->alba = 0;
	d->rlba = 0;
	d->cancel = 0;
	d->busy = BURN_DRIVE_IDLE;
	d->toc_entries = 0;
	d->toc_entry = NULL;
	d->disc = NULL;
	d->erasable = 0;

#ifdef Libburn_ticket_62_re_register_is_possiblE
	/* ts A60904 : ticket 62, contribution by elmom */
	/* Not yet accepted because no use case seen yet */

	/* This is supposed to find an already freed drive struct among
	   all the the ones that have been used before */
	for (i = 0; i < drivetop + 1; i++)
		if (drive_array[i].global_index == -1)
			break;
	d->global_index = i;
	memcpy(&drive_array[i], d, sizeof(struct burn_drive));
	pthread_mutex_init(&drive_array[i].access_lock, NULL);
	if (drivetop < i)
		drivetop = i;
	return &(drive_array[i]);

#else /* Libburn_ticket_62_re_register_is_possiblE */
	/* old A60904 : */
	/* Still active by default */

	d->global_index = drivetop + 1;
	memcpy(&drive_array[drivetop + 1], d, sizeof(struct burn_drive));
	pthread_mutex_init(&drive_array[drivetop + 1].access_lock, NULL);
	return &drive_array[++drivetop];

#endif /* ! Libburn_ticket_62_re_register_is_possiblE */

}

void burn_drive_release(struct burn_drive *d, int le)
{
	if (d->released)
		burn_print(1, "second release on drive!\n");

	/* ts A60906: one should not assume BURN_DRIVE_IDLE == 0 */
	assert(d->busy == BURN_DRIVE_IDLE);

	d->unlock(d);
	if (le)
		d->eject(d);

	d->release(d);

	d->status = BURN_DISC_UNREADY;
	d->released = 1;
	if (d->toc_entry)
		free(d->toc_entry);
	d->toc_entry = NULL;
	d->toc_entries = 0;
	if (d->disc != NULL) {
		burn_disc_free(d->disc);
		d->disc = NULL;
	}
}

void burn_wait_all(void)
{
	unsigned int i;
	int finished = 0;
	struct burn_drive *d;

	while (!finished) {
		finished = 1;
		d = drive_array;
		for (i = burn_drive_count(); i > 0; --i, ++d) {

			/* ts A60904 : ticket 62, contribution by elmom */
			if (d->global_index==-1)
				continue;

			assert(d->released);
		}
		if (!finished)
			sleep(1);
	}
}

void burn_disc_erase_sync(struct burn_drive *d, int fast)
{
	burn_message_clear_queue();

	burn_print(1, "erasing drive %s %s\n", d->idata->vendor,
		   d->idata->product);

	/* ts A60825 : allow on parole to blank appendable CDs */
	if ( ! (d->status == BURN_DISC_FULL ||
		(d->status == BURN_DISC_APPENDABLE &&
		 ! libburn_back_hack_42) ) )
		return;
	d->cancel = 0;
	d->busy = BURN_DRIVE_ERASING;
	d->erase(d, fast);
	/* reset the progress */
	d->progress.session = 0;
	d->progress.sessions = 1;
	d->progress.track = 0;
	d->progress.tracks = 1;
	d->progress.index = 0;
	d->progress.indices = 1;
	d->progress.start_sector = 0;
	d->progress.sectors = 0x10000;
	d->progress.sector = 0;
	/* read the initial 0 stage */
	while (!d->test_unit_ready(d) && d->get_erase_progress(d) == 0)
		sleep(1);
	while ((d->progress.sector = d->get_erase_progress(d)) > 0 ||
	!d->test_unit_ready(d))
		sleep(1);
	d->progress.sector = 0x10000;
	d->busy = BURN_DRIVE_IDLE;
}

enum burn_disc_status burn_disc_get_status(struct burn_drive *d)
{
	assert(!d->released);
	return d->status;
}

int burn_disc_erasable(struct burn_drive *d)
{
	return d->erasable;
}
enum burn_drive_status burn_drive_get_status(struct burn_drive *d,
					     struct burn_progress *p)
{
	if (p) {
		memcpy(p, &(d->progress), sizeof(struct burn_progress));
		/* TODO: add mutex */
	}
	return d->busy;
}

void burn_drive_cancel(struct burn_drive *d)
{
	pthread_mutex_lock(&d->access_lock);
	d->cancel = 1;
	pthread_mutex_unlock(&d->access_lock);
}

int burn_drive_get_block_types(struct burn_drive *d,
			       enum burn_write_types write_type)
{
	burn_print(12, "write type: %d\n", write_type);
	assert(			/* (write_type >= BURN_WRITE_PACKET) && */
		      (write_type <= BURN_WRITE_RAW));
	return d->block_types[write_type];
}

static void strip_spaces(char *str)
{
	char *tmp;

	tmp = str + strlen(str) - 1;
	while (isspace(*tmp))
		*(tmp--) = '\0';

	tmp = str;
	while (*tmp) {
		if (isspace(*tmp) && isspace(*(tmp + 1))) {
			char *tmp2;

			for (tmp2 = tmp + 1; *tmp2; ++tmp2)
				*(tmp2 - 1) = *tmp2;
			*(tmp2 - 1) = '\0';
		} else
			++tmp;
	}
}

static int drive_getcaps(struct burn_drive *d, struct burn_drive_info *out)
{
	struct scsi_inquiry_data *id;

	assert(d->idata);
	assert(d->mdata);
	if (!d->idata->valid || !d->mdata->valid)
		return 0;

	id = (struct scsi_inquiry_data *)d->idata;

	memcpy(out->vendor, id->vendor, sizeof(id->vendor));
	strip_spaces(out->vendor);
	memcpy(out->product, id->product, sizeof(id->product));
	strip_spaces(out->product);
	memcpy(out->revision, id->revision, sizeof(id->revision));
	strip_spaces(out->revision);
	strncpy(out->location, d->devname, 16);
	out->location[16] = '\0';
	out->buffer_size = d->mdata->buffer_size;
	out->read_dvdram = !!d->mdata->dvdram_read;
	out->read_dvdr = !!d->mdata->dvdr_read;
	out->read_dvdrom = !!d->mdata->dvdrom_read;
	out->read_cdr = !!d->mdata->cdr_read;
	out->read_cdrw = !!d->mdata->cdrw_read;
	out->write_dvdram = !!d->mdata->dvdram_write;
	out->write_dvdr = !!d->mdata->dvdr_write;
	out->write_cdr = !!d->mdata->cdr_write;
	out->write_cdrw = !!d->mdata->cdrw_write;
	out->write_simulate = !!d->mdata->simulate;
	out->c2_errors = !!d->mdata->c2_pointers;
	out->drive = d;
	/* update available block types for burners */
	if (out->write_dvdram || out->write_dvdr ||
	    out->write_cdrw || out->write_cdr)
		d->probe_write_modes(d);
	out->tao_block_types = d->block_types[BURN_WRITE_TAO];
	out->sao_block_types = d->block_types[BURN_WRITE_SAO];
	out->raw_block_types = d->block_types[BURN_WRITE_RAW];
	out->packet_block_types = d->block_types[BURN_WRITE_PACKET];
	return 1;
}

int burn_drive_scan_sync(struct burn_drive_info *drives[],
			 unsigned int *n_drives)
{
	/* state vars for the scan process */
	/* ts A60904 : did set some default values to feel comfortable */
	static int scanning = 0, scanned = 0, found = 0;
	static unsigned num_scanned = 0, count = 0;
	unsigned int i;
#ifdef Libburn_ticket_62_enable_redundant_asserT
	struct burn_drive *d;
#endif

	assert(burn_running);

	if (!scanning) {
		scanning = 1;
		/* make sure the drives aren't in use */
		burn_wait_all();	/* make sure the queue cleans up
					   before checking for the released
					   state */

		/* ts A60904 : ticket 62, contribution by elmom */
		/* this is redundant with what is done in burn_wait_all() */
#ifdef Libburn_ticket_62_enable_redundant_asserT
		d = drive_array;
		count = burn_drive_count();
		for (i = 0; i < count; ++i, ++d)
			assert(d->released == 1);
#endif /* Libburn_ticket_62_enable_redundant_asserT */

		/* refresh the lib's drives */
		sg_enumerate();
		ata_enumerate();
		count = burn_drive_count();
		if (count)
			*drives =
				malloc(sizeof(struct burn_drive_info) * count);
		else
			*drives = NULL;
		*n_drives = scanned = found = num_scanned = 0;
	}

	for (i = 0; i < count; ++i) {
		if (scanned & (1 << i))
			continue;	/* already scanned the device */

		while (!drive_getcaps(&drive_array[i],
				      &(*drives)[num_scanned])) {
			sleep(1);
		}
		scanned |= 1 << i;
		found |= 1 << i;
		num_scanned++;
		(*n_drives)++;
	}

	if (num_scanned == count) {
		/* done scanning */
		scanning = 0;
		return 1;
	}
	return 0;
}


void burn_drive_info_free(struct burn_drive_info drive_infos[])
{
/* ts A60904 : ticket 62, contribution by elmom */
/* clarifying the meaning and the identity of the victim */

	/* ts A60904 : This looks a bit weird.
	   burn_drive_info is not the manager of burn_drive but only its
	   spokesperson. To my knowlege drive_infos from burn_drive_scan()
	   are not memorized globally. */
	if(drive_infos != NULL) /* and this NULL test is deadly necessary */
		free((void *) drive_infos);

	burn_drive_free_all();
}

/* Experimental API call */
int burn_drive_info_forget(struct burn_drive_info *info, int force)
{
	int occup;
	struct burn_drive *d;

	d = info->drive;
	occup = burn_drive_is_occupied(d);
/*
	fprintf(stderr, "libburn: experimental: occup == %d\n",occup);
*/
	if(occup <= -2)
		return 2;
	if(occup > 0)
		if(force < 1)
			return 0; 
	if(occup > 10)
		return 0;

	/* >>> do any drive calming here */;


	burn_drive_force_idle(d);
	if(occup > 0 && !burn_drive_is_released(d))
		burn_drive_release(d,0);
	burn_drive_free(d);
	return 1;
}

struct burn_disc *burn_drive_get_disc(struct burn_drive *d)
{
	d->disc->refcnt++;
	return d->disc;
}

void burn_drive_set_speed(struct burn_drive *d, int r, int w)
{
	d->set_speed(d, r, w);
}

int burn_msf_to_sectors(int m, int s, int f)
{
	return (m * 60 + s) * 75 + f;
}

void burn_sectors_to_msf(int sectors, int *m, int *s, int *f)
{
	*m = sectors / (60 * 75);
	*s = (sectors - *m * 60 * 75) / 75;
	*f = sectors - *m * 60 * 75 - *s * 75;
}

int burn_drive_get_read_speed(struct burn_drive *d)
{
	return d->mdata->max_read_speed;
}

int burn_drive_get_write_speed(struct burn_drive *d)
{
	return d->mdata->max_write_speed;
}


/* ts A51221 */
static char *enumeration_whitelist[BURN_DRIVE_WHITELIST_LEN];
static int enumeration_whitelist_top = -1;

/** Add a device to the list of permissible drives. As soon as some entry is in
    the whitelist all non-listed drives are banned from enumeration.
    @return 1 success, <=0 failure
*/
int burn_drive_add_whitelist(char *device_address)
{
	char *new_item;
	if(enumeration_whitelist_top+1 >= BURN_DRIVE_WHITELIST_LEN)
		return 0;
	enumeration_whitelist_top++;
	new_item = malloc(strlen(device_address) + 1);
	if (new_item == NULL)
		return -1;
	strcpy(new_item, device_address);
	enumeration_whitelist[enumeration_whitelist_top] = new_item;
	return 1;
}

/** Remove all drives from whitelist. This enables all possible drives. */
void burn_drive_clear_whitelist(void)
{
	int i;
	for (i = 0; i <= enumeration_whitelist_top; i++)
		free(enumeration_whitelist[i]);
	enumeration_whitelist_top = -1;
}

int burn_drive_is_banned(char *device_address)
{
	int i;
	if(enumeration_whitelist_top<0)
		return 0;
	for (i = 0; i <= enumeration_whitelist_top; i++) 
		if (strcmp(enumeration_whitelist[i], device_address) == 0)
			return 0;
	return 1;
}

/* ts A60823 */
/** Aquire a drive with known persistent address. 
*/
int burn_drive_scan_and_grab(struct burn_drive_info *drive_infos[], char* adr,
			     int load)
{
	unsigned int n_drives;
	int ret;

	burn_drive_clear_whitelist();
	burn_drive_add_whitelist(adr);
/*
	fprintf(stderr,"libburn: experimental: burn_drive_scan_and_grab(%s)\n",
		adr);
*/
	while (!burn_drive_scan(drive_infos, &n_drives))
		usleep(1002);
	if (n_drives <= 0)
		return 0;
/*
	fprintf(stderr, "libburn: experimental: n_drives == %d\n",n_drives);
*/

/* ts A60908 : seems we get rid of this :) */
#ifdef Libburn_grab_release_and_grab_agaiN
	if (load) {
		/* RIP-14.5 + LITE-ON 48125S produce a false status
		   if tray was unloaded */
 		/* Therefore the first grab is just for loading */
		ret= burn_drive_grab(drive_infos[0]->drive, 1);
		if (ret != 1) 
			return -1;
		burn_drive_release(drive_infos[0]->drive,0);
	}
#endif /* Libburn_grab_release_and_grab_agaiN */

	ret = burn_drive_grab(drive_infos[0]->drive, load);
	if (ret != 1)
		return -1;
	return 1;
}

/* ts A60823 */
/** Inquire the persistent address of the given drive. */
int burn_drive_get_adr(struct burn_drive_info *drive_info, char adr[])
{
	assert(strlen(drive_info->location) < BURN_DRIVE_ADR_LEN);
	strcpy(adr,drive_info->location);
	return 1;
}

/* ts A60922 ticket 33 */
/** Evaluate wether the given address would be enumerated by libburn */
int burn_drive_is_enumerable_adr(char *adr)
{
	return sg_is_enumerable_adr(adr);
}

/* ts A60922 ticket 33 */
/* Try to find an enumerated address with the given stat.st_rdev number */
int burn_drive_resolve_link(char *path, char adr[])
{
	int ret;
	char link_target[4096];


fprintf(stderr,"libburn experimental: burn_drive_resolve_link( %s )\n",path);

	ret = readlink(path, link_target, sizeof(link_target));
	if(ret == -1) {

fprintf(stderr,"libburn experimental: readlink( %s ) returns -1\n",path);

		return 0;
	}
	if(ret >= sizeof(link_target) - 1) {

fprintf(stderr,"libburn experimental: readlink( %s ) returns %d (too much)\n",path,ret);

		return -1;
	}
	link_target[ret] = 0;
	ret = burn_drive_convert_fs_adr(link_target, adr);

fprintf(stderr,"libburn experimental: burn_drive_convert_fs_adr( %s ) returns %d\n",link_target,ret);

	return ret;
}

/* ts A60922 ticket 33 */
/* Try to find an enumerated address with the given stat.st_rdev number */
int burn_drive_find_devno(dev_t devno, char adr[])
{
	char fname[4096];
	int i, ret = 0, first = 1;
	struct stat stbuf;

	while (1) {
		ret= sg_give_next_adr(&i, fname, sizeof(fname), first);
		if(ret <= 0)
	break;
		first = 0;
		ret = stat(fname, &stbuf);
		if(ret == -1)
	continue;
		if(devno != stbuf.st_rdev)
	continue;
		if(strlen(fname) >= BURN_DRIVE_ADR_LEN)
			return -1;

fprintf(stderr,"libburn experimental: burn_drive_find_devno( 0x%lX ) found %s\n", (long) devno, fname);

		strcpy(adr, fname);
		return 1;
	}
	return 0;
}

/* ts A60922 ticket 33 */
/* Try to find an enumerated address with the same host,channel,target,lun
   as path */
int burn_drive_find_scsi_adr(char *path, char adr[])
{
	char fname[4096];
	int i, ret = 0, first = 1;
	int host_no, channel_no, target_no, lun_no;
	int i_host_no, i_channel_no, i_target_no, i_lun_no;

	ret = sg_obtain_scsi_adr(path, &host_no, &channel_no, &target_no,
			         &lun_no);
	if(ret <= 0) {

fprintf(stderr,"libburn experimental: sg_obtain_scsi_adr( %s ) returns %d\n", path, ret);

		return 0;
	}

fprintf(stderr,"libburn experimental: burn_drive_find_scsi_adr( %s ) : %d,%d,%d,%d\n", path, host_no, channel_no, target_no, lun_no);


	while (1) {
		ret= sg_give_next_adr(&i, fname, sizeof(fname), first);
		if(ret <= 0)
	break;
		first = 0;
		ret = sg_obtain_scsi_adr(fname, &i_host_no, &i_channel_no,
				 &i_target_no, &i_lun_no);
		if(ret == -1)
	continue;
		if(i_host_no != host_no || i_channel_no != channel_no ||
		   i_target_no != target_no || i_lun_no != lun_no)
	continue;
		if(strlen(fname) >= BURN_DRIVE_ADR_LEN)
			return -1;

fprintf(stderr,"libburn experimental: burn_drive_find_scsi_adr( %s ) found %s\n", path, fname);

		strcpy(adr, fname);
		return 1;
	}
	return 0;
}


/* ts A60922 ticket 33 */
/** Try to convert a given existing filesystem address into a persistent drive
    address.  */
int burn_drive_convert_fs_adr(char *path, char adr[])
{
	int ret;
	struct stat stbuf;

fprintf(stderr,"libburn experimental: burn_drive_convert_fs_adr( %s )\n",path);

	if(burn_drive_is_enumerable_adr(path)) {
		if(strlen(path) >= BURN_DRIVE_ADR_LEN)
			return -1;

fprintf(stderr,"libburn experimental: burn_drive_is_enumerable_adr( %s ) is true\n",path);

		strcpy(adr, path);
		return 1;
	}

	if(lstat(path, &stbuf) == -1) {

fprintf(stderr,"libburn experimental: lstat( %s ) returns -1\n",path);

		return 0;
	}
	if((stbuf.st_mode & S_IFMT) == S_IFLNK) {
		ret = burn_drive_resolve_link(path, adr);
		if(ret > 0)
			return 1;
	}
	if((stbuf.st_mode&S_IFMT) == S_IFBLK ||
	   (stbuf.st_mode&S_IFMT) == S_IFCHR) {
		ret = burn_drive_find_devno(stbuf.st_rdev, adr);
		if(ret > 0)
			return 1;
		ret = burn_drive_find_scsi_adr(path, adr);
		if(ret > 0)
			return 1;
	}

fprintf(stderr,"libburn experimental: Nothing found for %s \n",path);

	return 0;
}

