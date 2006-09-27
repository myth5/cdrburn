/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <string.h>
#include <sys/poll.h>
#include <linux/hdreg.h>

#include "transport.h"
#include "drive.h"
#include "sg.h"
#include "spc.h"
#include "mmc.h"
#include "sbc.h"
#include "debug.h"
#include "toc.h"
#include "util.h"

#include "libdax_msgs.h"
extern struct libdax_msgs *libdax_messenger;

static void enumerate_common(char *fname, int host_no, int channel_no,
                             int target_no, int lun_no);

/* ts A51221 */
int burn_drive_is_banned(char *device_address);

/* ts A60813 : storage objects are in libburn/init.c
   wether to use O_EXCL
   wether to use O_NOBLOCK with open(2) on devices
   wether to take O_EXCL rejection as fatal error */
extern int burn_sg_open_o_excl;
extern int burn_sg_open_o_nonblock;
extern int burn_sg_open_abort_busy;


/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
int mmc_function_spy(char * text);


static int sgio_test(int fd)
{
	unsigned char test_ops[] = { 0, 0, 0, 0, 0, 0 };
	sg_io_hdr_t s;

	memset(&s, 0, sizeof(sg_io_hdr_t));
	s.interface_id = 'S';
	s.dxfer_direction = SG_DXFER_NONE;
	s.cmd_len = 6;
	s.cmdp = test_ops;
	s.timeout = 12345;
	return ioctl(fd, SG_IO, &s);
}


/* ts A60925 : ticket 74 */
int sg_close_drive_fd(char *fname, int driveno, int *fd, int sorry)
{
	int ret, os_errno, sevno= LIBDAX_MSGS_SEV_DEBUG;
	char msg[4096+100];

	if(*fd < 0)
		return(0);
	ret = close(*fd);
	*fd = -1337;
	if(ret != -1)
		return 1;
	os_errno= errno;

	if (fname != NULL)
		sprintf(msg, "Encountered error when closing drive '%s'",
			fname);
	else
		sprintf(msg, "Encountered error when closing drive");

	if (sorry)
		sevno = LIBDAX_MSGS_SEV_SORRY;
	libdax_msgs_submit(libdax_messenger, driveno, 0x00020002,
			sevno, LIBDAX_MSGS_PRIO_HIGH, msg, os_errno, 0);
	return 0;	
}


/* ts A60924 */
int sg_handle_busy_device(char *fname, int os_errno)
{
	char msg[4096];

	/* ts A60814 : i saw no way to do this more nicely */ 
	if (burn_sg_open_abort_busy) {
		fprintf(stderr,
	"\nlibburn: FATAL : Application triggered abort on busy device '%s'\n",
			fname);
		assert("drive busy" == "non fatal");
	}

	/* ts A60924 : now reporting to libdax_msgs */
	sprintf(msg, "Cannot open busy device '%s'", fname);
	libdax_msgs_submit(libdax_messenger, -1, 0x00020001,
			LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_LOW,
			msg, os_errno, 0);
	return 1;
}


/* ts A60922 ticket 33 */
/** Returns the next index number and the next enumerated drive address.
    @param idx An opaque number handle. Make no own theories about it.
    @param adr Takes the reply
    @param adr_size Gives maximum size of reply including final 0
    @param initialize  1 = start new, 0 = continue, use no other values for now
    @return 1 = reply is a valid address , 0 = no further address available
           -1 = severe error (e.g. adr_size too small)
*/
int sg_give_next_adr(int *idx, char adr[], int adr_size, int initialize)
{
	static int sg_limit = 32, ata_limit = 26;
	int baseno = 0;

	if (initialize  == 1)
		*idx = -1;
	(*idx)++;
	if (*idx >= sg_limit)
		goto next_ata;
	if (adr_size < 10)
		return -1;
	sprintf(adr, "/dev/sg%d", *idx);
	return 1;
next_ata:;
	baseno += sg_limit;
	if (*idx - baseno >= ata_limit)
		goto next_nothing;
	if (adr_size < 9)
		return -1;
	sprintf(adr, "/dev/hd%c", 'a' + (*idx - baseno));
	return 1;
next_nothing:;
	baseno += ata_limit;
	return 0;
}

int sg_is_enumerable_adr(char *adr)
{
	char fname[4096];
	int i, ret = 0, first = 1;

	while (1) {
		ret= sg_give_next_adr(&i, fname, sizeof(fname), first);
		if(ret <= 0)
	break;
/* <<<
		fprintf(stderr,"libburn experimental: idx %d : %s\n",i,fname);
*/
		first = 0;
		if (strcmp(adr, fname) == 0)
			return 1;

	}
	return(0);
}


/* ts A60926 */
int sg_release_siblings(int sibling_fds[], int *sibling_count)
{
	int i;
	char msg[81];

	for(i= 0; i < *sibling_count; i++)
		sg_close_drive_fd(NULL, -1, &(sibling_fds[i]), 0);
	if(*sibling_count > 0) {
		sprintf(msg, "Closed %d O_EXCL scsi siblings", *sibling_count);
		libdax_msgs_submit(libdax_messenger, -1, 0x00020007,
			LIBDAX_MSGS_SEV_NOTE, LIBDAX_MSGS_PRIO_HIGH, msg, 0,0);
	}
	*sibling_count = 0;
	return 1;
}


/* ts A60926 */
int sg_open_drive_fd(char *fname, int scan_mode)
{
	int open_mode = O_RDWR, fd;
	char msg[81];

	/* ts A60813
	   O_EXCL with block devices is an unpublished feature
	   of Linux kernels. Possibly introduced 2002.
	   Mentioned in "The Linux SCSI Generic (sg) HOWTO" */
	if(burn_sg_open_o_excl)
		open_mode |= O_EXCL;
	/* ts A60813
	   O_NONBLOCK was already hardcoded in ata_ but not in sg_.
	   There must be some reason for this. So O_NONBLOCK is
	   default mode for both now. Disable on own risk. */
	if(burn_sg_open_o_nonblock)
		open_mode |= O_NONBLOCK;

/* <<< debugging
	fprintf(stderr,
		"\nlibburn: experimental: o_excl= %d , o_nonblock= %d, abort_on_busy= %d\n",
	burn_sg_open_o_excl,burn_sg_open_o_nonblock,burn_sg_open_abort_busy);
	fprintf(stderr,
		"libburn: experimental: O_EXCL= %d , O_NONBLOCK= %d\n",
		!!(open_mode&O_EXCL),!!(open_mode&O_NONBLOCK));
*/
          
	fd = open(fname, open_mode);
	if (fd == -1) {
/* <<< debugging
		fprintf(stderr,
		"\nlibburn: experimental: fname= %s , errno= %d\n",
			fname,errno);
*/
		if (errno == EBUSY) {
			sg_handle_busy_device(fname, errno);
			return -1;
			
		}
		if (scan_mode)
			return -1;
		sprintf(msg, "Failed to open device '%s'",fname);
		libdax_msgs_submit(libdax_messenger, -1, 0x00020005,
				LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
				msg, errno, 0);
		return -1;
	}
	return fd;
}


/* ts A60926 */
int sg_open_scsi_siblings(char *path, int driveno,
			  int sibling_fds[], int *sibling_count,
			  int host_no, int channel_no, int id_no, int lun_no)
{
	int tld, i, ret, fd;
	int i_host_no = -1, i_channel_no = -1, i_target_no = -1, i_lun_no = -1;
	char msg[161], fname[81];

	static char tldev[][81]= {"/dev/sr%d", "/dev/scd%d", "/dev/st%d", ""};

	if(host_no < 0 || id_no < 0 || channel_no < 0 || lun_no < 0)
		return(2);
	if(*sibling_count > 0)
		sg_release_siblings(sibling_fds, sibling_count);
		
	for (tld = 0; tldev[tld][0] != 0; tld++) {
		for (i = 0; i < 32; i++) {
			sprintf(fname, tldev[tld], i);
			ret = sg_obtain_scsi_adr(fname, &i_host_no,
				&i_channel_no, &i_target_no, &i_lun_no);
			if (ret <= 0)
		continue;
			if (i_host_no != host_no || i_channel_no != channel_no)
		continue;
			if (i_target_no != id_no || i_lun_no != lun_no)
		continue;

			fd = sg_open_drive_fd(fname, 0);
			if (fd < 0)
				goto failed;

			if (*sibling_count>=LIBBURN_SG_MAX_SIBLINGS) {
				sprintf(msg, "Too many scsi siblings of '%s'",
					path);
				libdax_msgs_submit(libdax_messenger,
					driveno, 0x00020006,
					LIBDAX_MSGS_SEV_FATAL,
					LIBDAX_MSGS_PRIO_HIGH, msg, 0, 0);
				goto failed;
			}
			sprintf(msg, "Opened O_EXCL scsi sibling '%s' of '%s'",
				 fname, path);
			libdax_msgs_submit(libdax_messenger, driveno,
				0x00020004,
				LIBDAX_MSGS_SEV_NOTE, LIBDAX_MSGS_PRIO_HIGH,
				msg, 0, 0);
			sibling_fds[*sibling_count] = fd;
			(*sibling_count)++;
		}
	}
	return(1);
failed:;
	sg_release_siblings(sibling_fds, sibling_count);
	return 0;
}


/* ts A60926 */
int sg_close_drive(struct burn_drive *d)
{
	int ret;

	if (!burn_drive_is_open(d))
		return 0;
	sg_release_siblings(d->sibling_fds, &(d->sibling_count));
	ret = sg_close_drive_fd(d->devname, d->global_index, &(d->fd), 0);
	return ret;
}

void ata_enumerate(void)
{
	struct hd_driveid tm;
	int i, fd;
	char fname[10];


#ifdef No_sg_open_drive_fD

	/* ts A60813 */
	int open_mode = O_RDWR;

	/* ts A60813
	   O_EXCL with block devices is an unpublished feature
	   of Linux kernels. Possibly introduced 2002.
	   Mentioned in "The Linux SCSI Generic (sg) HOWTO" */
	if(burn_sg_open_o_excl)
		open_mode |= O_EXCL;
	/* ts A60813
	   O_NONBLOCK was already hardcoded in ata_ but not in sg_.
	   There must be some reason for this. So O_NONBLOCK is
	   default mode for both now. Disable on own risk. */
	if(burn_sg_open_o_nonblock)
		open_mode |= O_NONBLOCK;

#endif /* No_sg_open_drive_fD */


	for (i = 0; i < 26; i++) {
		sprintf(fname, "/dev/hd%c", 'a' + i);
		/* open O_RDWR so we don't think read only drives are
		   in some way useful
		 */
		/* ts A51221 */
		if (burn_drive_is_banned(fname))
			continue;

#ifdef No_sg_open_drive_fD
		fd = open(fname, open_mode);
		if (fd == -1) {
/* <<< debugging
			fprintf(stderr,
			"\nlibburn: experimental: fname= %s , errno= %d\n",
				fname,errno);
*/
			if (errno == EBUSY) 
				sg_handle_busy_device(fname, errno);
			continue;
		}
#else
		fd = sg_open_drive_fd(fname, 1);
		if (fd == -1)
			continue;
#endif /* ! No_sg_open_drive_fD */

		/* found a drive */
		ioctl(fd, HDIO_GET_IDENTITY, &tm);

		/* not atapi */
		if (!(tm.config & 0x8000) || (tm.config & 0x4000)) {
			sg_close_drive_fd(fname, -1, &fd, 0);
			continue;
		}

		/* if SG_IO fails on an atapi device, we should stop trying to 
		   use hd* devices */
		if (sgio_test(fd) == -1) {
			sg_close_drive_fd(fname, -1, &fd, 0);
			return;
		}
		if (sg_close_drive_fd(fname, -1, &fd, 1) <= 0)
			continue;
		enumerate_common(fname, -1, -1, -1, -1);
	}
}

void sg_enumerate(void)
{
	struct sg_scsi_id sid;
	int i, fd, sibling_fds[LIBBURN_SG_MAX_SIBLINGS], sibling_count= 0, ret;
	char fname[10];


#ifdef No_sg_open_drive_fD

	/* ts A60813 */
	int open_mode = O_RDWR;

	/* ts A60813
	   O_EXCL with block devices is an unpublished feature
	   of Linux kernels. Possibly introduced 2002.
	   Mentioned in "The Linux SCSI Generic (sg) HOWTO" */
	if(burn_sg_open_o_excl)
		open_mode |= O_EXCL;
	/* ts A60813
	   O_NONBLOCK was not hardcoded in sg_ but was in ata_.
	   I myself test mainly sg_ and it seems to be ok with 
	   O_NONBLOCK. So it should stay default mode. */
	if(burn_sg_open_o_nonblock)
		open_mode |= O_NONBLOCK;

/* <<< debugging
	fprintf(stderr,
		"\nlibburn: experimental: o_excl= %d , o_nonblock= %d, abort_on_busy= %d\n",
	burn_sg_open_o_excl,burn_sg_open_o_nonblock,burn_sg_open_abort_busy);
	fprintf(stderr,
		"libburn: experimental: O_EXCL= %d , O_NONBLOCK= %d\n",
		!!(open_mode&O_EXCL),!!(open_mode&O_NONBLOCK));
          
*/

#endif /* No_sg_open_drive_fD */


	for (i = 0; i < 32; i++) {
		sprintf(fname, "/dev/sg%d", i);
		/* open RDWR so we don't accidentally think read only drives
		   are in some way useful
		 */
		/* ts A51221 */
		if (burn_drive_is_banned(fname))
	continue;

#ifdef No_sg_open_drive_fD

		fd = open(fname, open_mode);

		if (fd == -1) {
/* <<< debugging
			fprintf(stderr,
			"\n cdrskin: experimental: fname= %s , errno= %d\n",
				fname,errno);
*/
			if (errno == EBUSY) 
				sg_handle_busy_device(fname, errno);
	continue;
		}
#else

		fd = sg_open_drive_fd(fname, 1);
		if (fd == -1)
			continue;

#endif /* ! No_sg_open_drive_fD */

		/* found a drive */
		ioctl(fd, SG_GET_SCSI_ID, &sid);
		if (sg_close_drive_fd(fname, -1, &fd, 
				sid.scsi_type == TYPE_ROM ) <= 0)
	continue;
		if (sid.scsi_type != TYPE_ROM)
	continue;

		/* ts A60927 : trying to do locking with growisofs */
		if(burn_sg_open_o_excl>1) {
			ret = sg_open_scsi_siblings(
					fname, -1, sibling_fds, &sibling_count,
					sid.host_no, sid.channel,
					sid.scsi_id, sid.lun);
			if (ret<=0) {
				sg_handle_busy_device(fname, 0);
	continue;
			}
			/* the final occupation will be done in sg_grab() */
			sg_release_siblings(sibling_fds, &sibling_count);
		}

		enumerate_common(fname, sid.host_no, sid.channel, 
				sid.scsi_id, sid.lun);
	}
}
/* ts A60923 : introduced new SCSI parameters */
static void enumerate_common(char *fname, int host_no, int channel_no, 
			     int target_no, int lun_no)
{
	int i;
	struct burn_drive *t;
	struct burn_drive out;

	/* ts A60923 */
	out.host = host_no;
	out.id = target_no;
	out.channel = channel_no;
	out.lun = lun_no;

	out.devname = burn_strdup(fname);
	out.fd = -1337;
	out.sibling_count = 0;
	for(i= 0; i<LIBBURN_SG_MAX_SIBLINGS; i++)
		out.sibling_fds[i] = -1337;

	out.grab = sg_grab;
	out.release = sg_release;
	out.issue_command = sg_issue_command;
	out.getcaps = spc_getcaps;
	out.released = 1;
	out.status = BURN_DISC_UNREADY;

	out.eject = sbc_eject;
	out.load = sbc_load;
	out.lock = spc_prevent;
	out.unlock = spc_allow;
	out.read_disc_info = spc_sense_write_params;
	out.get_erase_progress = spc_get_erase_progress;
	out.test_unit_ready = spc_test_unit_ready;
	out.probe_write_modes = spc_probe_write_modes;
	out.read_toc = mmc_read_toc;
	out.write = mmc_write;
	out.erase = mmc_erase;
	out.read_sectors = mmc_read_sectors;
	out.perform_opc = mmc_perform_opc;
	out.set_speed = mmc_set_speed;
	out.send_parameters = spc_select_error_params;
	out.send_write_parameters = spc_select_write_params;
	out.send_cue_sheet = mmc_send_cue_sheet;
	out.sync_cache = mmc_sync_cache;
	out.get_nwa = mmc_get_nwa;
	out.close_disc = mmc_close_disc;
	out.close_session = mmc_close_session;
	out.idata = malloc(sizeof(struct scsi_inquiry_data));
	out.idata->valid = 0;
	out.mdata = malloc(sizeof(struct scsi_mode_data));
	out.mdata->valid = 0;
	memset(&out.params, 0, sizeof(struct params));
	t = burn_drive_register(&out);

/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
	mmc_function_spy("enumerate_common : -------- doing grab");

/* try to get the drive info */
	if (sg_grab(t)) {
		burn_print(2, "getting drive info\n");
		t->getcaps(t);
		t->unlock(t);
		t->released = 1;
	} else {
		burn_print(2, "unable to grab new located drive\n");
		burn_drive_unregister(t);
	}

/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
	mmc_function_spy("enumerate_common : ----- would release ");

}

/*
	we use the sg reference count to decide whether we can use the
	drive or not.
	if refcount is not one, drive is open somewhere else.

	ts A60813: this test is too late. O_EXCL is the stronger solution.
	After all the test was disabled already in icculus.org/burn CVS.
*/
int sg_grab(struct burn_drive *d)
{
	int fd, count, os_errno= 0, ret;

	/* ts A60813 */
	int open_mode = O_RDWR;

/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
	mmc_function_spy("sg_grab");


	/* ts A60813
	   O_EXCL with block devices is an unpublished feature
	   of Linux kernels. Possibly introduced 2002.
	   It can only be used if libburn stops opening several
	   file descriptor on the same block device.
	   See comment below */
	if(burn_sg_open_o_excl)
		open_mode |= O_EXCL;

	/* ts A60813
	   O_NONBLOCK was hardcoded here. So it should stay default mode. */
	if(burn_sg_open_o_nonblock)
		open_mode |= O_NONBLOCK;

	/* ts A60813
	   After enumeration the drive fd is probably still open.
	   -1337 is the initial value of burn_drive.fd and the value after
	   relase of drive. Unclear why not the official error return
	   value -1 of open(2) war used. */
				/* ts A60822: was  if(d->fd == -1337) { */
	if(! burn_drive_is_open(d)) {

		/* ts A60821
   		<<< debug: for tracing calls which might use open drive fds */
		mmc_function_spy("sg_grab ----------- opening");

		/* ts A60926 */
		if(burn_sg_open_o_excl>1) {
			fd = -1;
			ret = sg_open_scsi_siblings(d->devname,
					d->global_index,d->sibling_fds,
					&(d->sibling_count),
					d->host, d->channel, d->id, d->lun);
			if(ret <= 0)
				goto drive_is_in_use;
		}

		fd = open(d->devname, open_mode);
		os_errno = errno;
	} else
		fd= d->fd;

	assert(fd != -1337);
	if (-1 != fd) {

		/* ts A60814:
		   according to my experiments this test would work now ! */

		/* ts A60926 : this was disabled */
		/* Tests with growisofs on kernel 2.4.21 yielded that this
		   does not help against blocking on busy drives.
		*/
/* <<< the old dummy */
/*              er = ioctl(fd, SG_GET_ACCESS_COUNT, &count);*/
		count = 1;

		if (1 == count) {
			d->fd = fd;
			fcntl(fd, F_SETOWN, getpid());
			d->released = 0;
			return 1;
		}

drive_is_in_use:;
		libdax_msgs_submit(libdax_messenger, d->global_index,
			0x00020003,
			LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
			"Could not grab drive - already in use", 0, 0);
		sg_close_drive(d);
		d->fd = -1337;
		return 0;
	}
	libdax_msgs_submit(libdax_messenger, d->global_index, 0x00020003,
			LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
			"Could not grab drive", os_errno, 0);
	return 0;
}

/*
	non zero return means you still have the drive and it's not
	in a state to be released? (is that even possible?)
*/

int sg_release(struct burn_drive *d)
{
	/* ts A60821
   	<<< debug: for tracing calls which might use open drive fds */
	mmc_function_spy("sg_release");

	if (d->fd < 1) {
		burn_print(1, "release an ungrabbed drive.  die\n");
		return 0;
	}

	/* ts A60821
   	<<< debug: for tracing calls which might use open drive fds */
	mmc_function_spy("sg_release ----------- closing");

	sg_close_drive(d);
	return 0;
}

int sg_issue_command(struct burn_drive *d, struct command *c)
{
	int done = 0;
	int err;
	sg_io_hdr_t s;

	
/* ts A60821
   <<< debug: for tracing calls which might use open drive fds */
	char buf[161];
	sprintf(buf,"sg_issue_command   d->fd= %d  d->released= %d\n",
		d->fd,d->released);
	mmc_function_spy(buf);


	c->error = 0;
/*
this is valid during the mode probe in scan
	if (d->fd < 1 || d->released) {
		burn_print(1,
			      "command issued on ungrabbed drive, chaos.\n");
		burn_print(1, "fd = %d, released = %d\n", d->fd,
			      d->released);
	}
*/
	memset(&s, 0, sizeof(sg_io_hdr_t));

	s.interface_id = 'S';

	if (c->dir == TO_DRIVE)
		s.dxfer_direction = SG_DXFER_TO_DEV;
	else if (c->dir == FROM_DRIVE)
		s.dxfer_direction = SG_DXFER_FROM_DEV;
	else if (c->dir == NO_TRANSFER) {
		s.dxfer_direction = SG_DXFER_NONE;
		assert(!c->page);
	}
	s.cmd_len = c->oplen;
	s.cmdp = c->opcode;
	s.mx_sb_len = 32;
	s.sbp = c->sense;
	memset(c->sense, 0, sizeof(c->sense));
	s.timeout = 200000;
	if (c->page) {
		s.dxferp = c->page->data;
		if (c->dir == FROM_DRIVE) {
			s.dxfer_len = BUFFER_SIZE;
/* touch page so we can use valgrind */
			memset(c->page->data, 0, BUFFER_SIZE);
		} else {
			assert(c->page->bytes > 0);
			s.dxfer_len = c->page->bytes;
		}
	} else {
		s.dxferp = NULL;
		s.dxfer_len = 0;
	}
	s.usr_ptr = c;

	do {
		err = ioctl(d->fd, SG_IO, &s);
		assert(err != -1);
		if (s.sb_len_wr) {
			if (!c->retry) {
				c->error = 1;
				return 1;
			}
			switch (scsi_error(d, s.sbp, s.sb_len_wr)) {
			case RETRY:
				done = 0;
				break;
			case FAIL:
				done = 1;
				c->error = 1;
				break;
			}
		} else {
			done = 1;
		}
	} while (!done);
	return 1;
}

enum response scsi_error(struct burn_drive *d, unsigned char *sense,
			 int senselen)
{
	int key, asc, ascq;

	senselen = senselen;
	key = sense[2];
	asc = sense[12];
	ascq = sense[13];

	burn_print(12, "CONDITION: 0x%x 0x%x 0x%x on %s %s\n",
		   key, asc, ascq, d->idata->vendor, d->idata->product);

	switch (asc) {
	case 0:
		burn_print(12, "NO ERROR!\n");
		return RETRY;

	case 2:
		burn_print(1, "not ready\n");
		return RETRY;
	case 4:
		burn_print(1,
			   "logical unit is in the process of becoming ready\n");
		return RETRY;
	case 0x20:
		if (key == 5)
			burn_print(1, "bad opcode\n");
		return FAIL;
	case 0x21:
		burn_print(1, "invalid address or something\n");
		return FAIL;
	case 0x24:
		if (key == 5)
			burn_print(1, "invalid field in cdb\n");
		else
			break;
		return FAIL;
	case 0x26:
		if ( key == 5 )
			burn_print( 1, "invalid field in parameter list\n" );
		return FAIL;
	case 0x28:
		if (key == 6)
			burn_print(1,
				   "Not ready to ready change, medium may have changed\n");
		else
			break;
		return RETRY;
	case 0x3A:
		burn_print(12, "Medium not present in %s %s\n",
			   d->idata->vendor, d->idata->product);

		d->status = BURN_DISC_EMPTY;
		return FAIL;
	}
	burn_print(1, "unknown failure\n");
	burn_print(1, "key:0x%x, asc:0x%x, ascq:0x%x\n", key, asc, ascq);
	return FAIL;
}

/* ts A60922 */
/** Try to obtain SCSI address parameters.
    @return  1 is success , 0 is failure
*/
int sg_obtain_scsi_adr(char *path, int *host_no, int *channel_no,
                       int *target_no, int *lun_no)
{
	int fd, ret;
	struct my_scsi_idlun {
		int x;
		int host_unique_id;
	};
 	struct my_scsi_idlun idlun;

	if (strncmp(path, "/dev/hd", 7) == 0 
	    && path[7] >= 'a' && path[7] <= 'z' && path[8] == 0)
		return 0; /* on RIP 14 all hdx return SCSI adr 0,0,0,0 */

	fd = open(path, O_RDONLY | O_NONBLOCK);
	if(fd < 0)
		return 0;

	/* http://www.tldp.org/HOWTO/SCSI-Generic-HOWTO/scsi_g_idlun.html */
	ret= ioctl(fd, SCSI_IOCTL_GET_IDLUN, &idlun);

	sg_close_drive_fd(path, -1, &fd, 0);
	if (ret == -1)
		return(0);
	*host_no= (idlun.x>>24)&255;
	*channel_no= (idlun.x>>16)&255;
	*target_no= (idlun.x)&255;
	*lun_no= (idlun.x>>8)&255;
	return 1;
}
