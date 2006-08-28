/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

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

void burn_drive_free(void)
{
	int i;
	struct burn_drive *d;

	for (i = 0; i < drivetop + 1; i++) {
		d = &drive_array[i];
		if (burn_drive_is_open(d))
			close(d->fd);
		free((void *)d->idata);
		free((void *)d->mdata);
		free((void *)d->toc_entry);
		free(d->devname);
	}
	drivetop = -1;
	memset(drive_array, 0, sizeof(drive_array));
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
		d->read_disc_info(d);
	} else
		d->read_toc(d);
	d->busy = BURN_DRIVE_IDLE;
	return 1;
}

struct burn_drive *burn_drive_register(struct burn_drive *d)
{
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
	memcpy(&drive_array[drivetop + 1], d, sizeof(struct burn_drive));
	pthread_mutex_init(&drive_array[drivetop + 1].access_lock, NULL);
	return &drive_array[++drivetop];
}

void burn_drive_release(struct burn_drive *d, int le)
{
	if (d->released)
		burn_print(1, "second release on drive!\n");
	assert(!d->busy);
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
	while (!d->test_unit_ready(d) &&
	       (d->progress.sector = d->get_erase_progress(d)) > 0)
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
	static int scanning = 0, scanned, found;
	static unsigned num_scanned, count;
	unsigned int i;
	struct burn_drive *d;

	assert(burn_running);

	if (!scanning) {
		scanning = 1;
		/* make sure the drives aren't in use */
		burn_wait_all();	/* make sure the queue cleans up
					   before checking for the released
					   state */

		d = drive_array;
		count = burn_drive_count();
		for (i = 0; i < count; ++i, ++d)
			assert(d->released == 1);

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

void burn_drive_info_free(struct burn_drive_info *info)
{
	free(info);
	burn_drive_free();
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

/* ts A60823 */
/** Aquire a drive with known persistent address. 
*/
int burn_drive_scan_and_grab(struct burn_drive_info *drives[], char* adr,
			     int load)
{
	unsigned int n_drives;
	int ret;

	burn_drive_clear_whitelist();
	burn_drive_add_whitelist(adr);
	while (!burn_drive_scan(drives, &n_drives));
	if (n_drives <= 0)
		return 0;
	if (load) {
		/* RIP-14.5 + LITE-ON 48125S produce a false status
		   if tray was unloaded */
 		/* Therefore the first grab is just for loading */
		ret= burn_drive_grab(drives[0]->drive, 1);
		if (ret != 1) 
			return -1;
		burn_drive_release(drives[0]->drive,0);
	}
	ret = burn_drive_grab(drives[0]->drive, load);
	if (ret != 1)
		return -1;
	return 1;
}

/* ts A60823 */
/** Inquire the persistent address of the given drive. */
int burn_drive_get_adr(struct burn_drive_info *drive, char adr[])
{
	assert(strlen(drive->location) < BURN_DRIVE_ADR_LEN);
	strcpy(adr,drive->location);
	return 1;
}

