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

static void enumerate_common(char *fname);

/* ts A51221 */
int burn_drive_is_banned(char *device_address);

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

void ata_enumerate(void)
{
	struct hd_driveid tm;
	int i, fd;
	char fname[10];

	for (i = 0; i < 26; i++) {
		sprintf(fname, "/dev/hd%c", 'a' + i);
		/* open O_RDWR so we don't think read only drives are
		   in some way useful
		 */
		/* ts A51221 */
		if (burn_drive_is_banned(fname))
			continue;
		fd = open(fname, O_RDWR | O_NONBLOCK);
		if (fd == -1)
			continue;

		/* found a drive */
		ioctl(fd, HDIO_GET_IDENTITY, &tm);

		/* not atapi */
		if (!(tm.config & 0x8000) || (tm.config & 0x4000)) {
			close(fd);
			continue;
		}

		/* if SG_IO fails on an atapi device, we should stop trying to 
		   use hd* devices */
		if (sgio_test(fd) == -1) {
			close(fd);
			return;
		}
		close(fd);
		enumerate_common(fname);
	}
}

void sg_enumerate(void)
{
	struct sg_scsi_id sid;
	int i, fd;
	char fname[10];

	for (i = 0; i < 32; i++) {
		sprintf(fname, "/dev/sg%d", i);
		/* open RDWR so we don't accidentally think read only drives
		   are in some way useful
		 */
		/* ts A51221 */
		if (burn_drive_is_banned(fname))
			continue;
		fd = open(fname, O_RDWR);
		if (fd == -1)
			continue;

		/* found a drive */
		ioctl(fd, SG_GET_SCSI_ID, &sid);
		close(fd);
		if (sid.scsi_type != TYPE_ROM)
			continue;

		enumerate_common(fname);
	}
}

static void enumerate_common(char *fname)
{
	struct burn_drive *t;
	struct burn_drive out;

	out.devname = burn_strdup(fname);
	out.fd = -1337;

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

/* try to get the drive info */
	if (sg_grab(t)) {
		burn_print(2, "getting drive info\n");
		t->getcaps(t);
		t->unlock(t);
		t->released = 1;
	} else {
		burn_print(2, "unable to grab new located drive\n");
	}

}

/*
	we use the sg reference count to decide whether we can use the
	drive or not.
	if refcount is not one, drive is open somewhere else.
*/
int sg_grab(struct burn_drive *d)
{
	int fd, count;

	fd = open(d->devname, O_RDWR | O_NONBLOCK);
	assert(fd != -1337);
	if (-1 != fd) {
/*		er = ioctl(fd, SG_GET_ACCESS_COUNT, &count);*/
		count = 1;
		if (1 == count) {
			d->fd = fd;
			fcntl(fd, F_SETOWN, getpid());
			d->released = 0;
			return 1;
		}
		burn_print(1, "could not acquire drive - already open\n");
		close(fd);
		return 0;
	}
	burn_print(1, "could not acquire drive\n");
	return 0;
}

/*
	non zero return means you still have the drive and it's not
	in a state to be released? (is that even possible?)
*/

int sg_release(struct burn_drive *d)
{
	if (d->fd < 1) {
		burn_print(1, "release an ungrabbed drive.  die\n");
		return 0;
	}
	close(d->fd);
	d->fd = -1337;
	return 0;
}

int sg_issue_command(struct burn_drive *d, struct command *c)
{
	int done = 0;
	int err;
	sg_io_hdr_t s;

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
