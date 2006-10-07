/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/* scsi primary commands */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "transport.h"
#include "spc.h"
#include "mmc.h"
#include "sbc.h"
#include "drive.h"
#include "debug.h"
#include "options.h"

/* spc command set */
static unsigned char SPC_INQUIRY[] = { 0x12, 0, 0, 0, 255, 0 };

/*static char SPC_TEST[]={0,0,0,0,0,0};*/
static unsigned char SPC_PREVENT[] = { 0x1e, 0, 0, 0, 1, 0 };
static unsigned char SPC_ALLOW[] = { 0x1e, 0, 0, 0, 0, 0 };
static unsigned char SPC_MODE_SENSE[] = { 0x5a, 0, 0, 0, 0, 0, 0, 16, 0, 0 };
static unsigned char SPC_MODE_SELECT[] =
	{ 0x55, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char SPC_REQUEST_SENSE[] = { 0x03, 0, 0, 0, 18, 0 };
static unsigned char SPC_TEST_UNIT_READY[] = { 0x00, 0, 0, 0, 0, 0 };

int spc_test_unit_ready(struct burn_drive *d)
{
	struct command c;

	c.retry = 0;
	c.oplen = sizeof(SPC_TEST_UNIT_READY);
	memcpy(c.opcode, SPC_TEST_UNIT_READY, sizeof(SPC_TEST_UNIT_READY));
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
	if (c.error)
		return (c.sense[2] & 0xF) == 0;
	return 1;
}

void spc_request_sense(struct burn_drive *d, struct buffer *buf)
{
	struct command c;

	c.retry = 0;
	c.oplen = sizeof(SPC_REQUEST_SENSE);
	memcpy(c.opcode, SPC_REQUEST_SENSE, sizeof(SPC_REQUEST_SENSE));
	c.page = buf;
	c.page->sectors = 0;
	c.page->bytes = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);
}

int spc_get_erase_progress(struct burn_drive *d)
{
	struct buffer b;

	spc_request_sense(d, &b);
	return (b.data[16] << 8) | b.data[17];
}

void spc_inquiry(struct burn_drive *d)
{
	struct buffer buf;
	struct scsi_inquiry_data *id;
	struct command c;

	memcpy(c.opcode, SPC_INQUIRY, sizeof(SPC_INQUIRY));
	c.retry = 1;
	c.oplen = sizeof(SPC_INQUIRY);
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	id = (struct scsi_inquiry_data *)d->idata;
	id->vendor[8] = 0;
	id->product[16] = 0;
	id->revision[4] = 0;

	memcpy(id->vendor, c.page->data + 8, 8);
	memcpy(id->product, c.page->data + 16, 16);
	memcpy(id->revision, c.page->data + 32, 4);

	id->valid = 1;
	return;
}

void spc_prevent(struct burn_drive *d)
{
	struct command c;

	memcpy(c.opcode, SPC_PREVENT, sizeof(SPC_PREVENT));
	c.retry = 1;
	c.oplen = sizeof(SPC_PREVENT);
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}

void spc_allow(struct burn_drive *d)
{
	struct command c;

	memcpy(c.opcode, SPC_ALLOW, sizeof(SPC_ALLOW));
	c.retry = 1;
	c.oplen = sizeof(SPC_ALLOW);
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}

void spc_sense_caps(struct burn_drive *d)
{
	struct buffer buf;
	struct scsi_mode_data *m;
	int size;
	unsigned char *page;
	struct command c;

	memcpy(c.opcode, SPC_MODE_SENSE, sizeof(SPC_MODE_SENSE));
	c.retry = 1;
	c.oplen = sizeof(SPC_MODE_SENSE);
	c.opcode[2] = 0x2A;
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	size = c.page->data[0] * 256 + c.page->data[1];
	m = d->mdata;
	page = c.page->data + 8;

	m->buffer_size = page[12] * 256 + page[13];
	m->dvdram_read = page[2] & 32;
	m->dvdram_write = page[3] & 32;
	m->dvdr_read = page[2] & 16;
	m->dvdr_write = page[3] & 16;
	m->dvdrom_read = page[2] & 8;
	m->simulate = page[3] & 4;
	m->cdrw_read = page[2] & 2;
	m->cdrw_write = page[3] & 2;
	m->cdr_read = page[2] & 1;
	m->cdr_write = page[3] & 1;
	m->max_read_speed = page[8] * 256 + page[9];
	m->cur_read_speed = page[14] * 256 + page[15];
	m->max_write_speed = page[18] * 256 + page[19];
	m->cur_write_speed = page[20] * 256 + page[21];
	m->c2_pointers = page[5] & 16;
	m->valid = 1;
	m->underrun_proof = page[4] & 128;
}

void spc_sense_error_params(struct burn_drive *d)
{
	struct buffer buf;
	struct scsi_mode_data *m;
	int size;
	unsigned char *page;
	struct command c;

	memcpy(c.opcode, SPC_MODE_SENSE, sizeof(SPC_MODE_SENSE));
	c.retry = 1;
	c.oplen = sizeof(SPC_MODE_SENSE);
	c.opcode[2] = 0x01;
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	size = c.page->data[0] * 256 + c.page->data[1];
	m = d->mdata;
	page = c.page->data + 8;
	d->params.retries = page[3];
	m->retry_page_length = page[1];
	m->retry_page_valid = 1;
}

void spc_select_error_params(struct burn_drive *d,
			     const struct burn_read_opts *o)
{
	struct buffer buf;
	struct command c;

	memcpy(c.opcode, SPC_MODE_SELECT, sizeof(SPC_MODE_SELECT));
	c.retry = 1;
	c.oplen = sizeof(SPC_MODE_SELECT);
	c.opcode[8] = 8 + 2 + d->mdata->retry_page_length;
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;

	/* ts A61007 : moved up to only caller burn_disc_read() */
	/* a ssert(d->mdata->valid); */

	memset(c.page->data, 0, 8 + 2 + d->mdata->retry_page_length);
	c.page->bytes = 8 + 2 + d->mdata->retry_page_length;
	c.page->data[8] = 1;
	c.page->data[9] = d->mdata->retry_page_length;
	if (o->transfer_damaged_blocks)
		c.page->data[10] |= 32;
	if (o->report_recovered_errors)
		c.page->data[10] |= 4;
	if (!o->hardware_error_recovery)
		c.page->data[10] |= 1;
/*burn_print(1, "error parameter 0x%x\n", c->page->data[10]);*/
	c.page->data[11] = d->params.retries;
	c.dir = TO_DRIVE;
	d->issue_command(d, &c);
}

void spc_sense_write_params(struct burn_drive *d)
{
	struct buffer buf;
	struct scsi_mode_data *m;
	int size;
	unsigned char *page;
	struct command c;

	/* ts A61007 : Done in soft at only caller burn_drive_grab() */
	/* a ssert(d->mdata->cdr_write || d->mdata->cdrw_write ||
	       d->mdata->dvdr_write || d->mdata->dvdram_write); */

	memcpy(c.opcode, SPC_MODE_SENSE, sizeof(SPC_MODE_SENSE));
	c.retry = 1;
	c.oplen = sizeof(SPC_MODE_SENSE);
	c.opcode[2] = 0x05;
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	size = c.page->data[0] * 256 + c.page->data[1];
	m = d->mdata;
	page = c.page->data + 8;
	burn_print(1, "write page length 0x%x\n", page[1]);
	m->write_page_length = page[1];
	m->write_page_valid = 1;
	mmc_read_disc_info(d);
}

void spc_select_write_params(struct burn_drive *d,
			     const struct burn_write_opts *o)
{
	struct buffer buf;
	struct command c;
	int bufe, sim;

	/* ts A61007 : All current callers are safe. */
	/* a ssert(o->drive == d); */

	memcpy(c.opcode, SPC_MODE_SELECT, sizeof(SPC_MODE_SELECT));
	c.retry = 1;
	c.oplen = sizeof(SPC_MODE_SELECT);
	c.opcode[8] = 8 + 2 + d->mdata->write_page_length;
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;

	/* ts A61007 : moved up to burn_disc_write() */
	/* a ssert(d->mdata->valid); */

	memset(c.page->data, 0, 8 + 2 + d->mdata->write_page_length);
	c.page->bytes = 8 + 2 + d->mdata->write_page_length;
	c.page->data[8] = 5;
	c.page->data[9] = d->mdata->write_page_length;

	burn_print(12, "using write page length %d (valid %d)\n",
		   d->mdata->write_page_length, d->mdata->write_page_valid);
	bufe = o->underrun_proof;
	sim = o->simulate;
	c.page->data[10] = (bufe << 6)
		+ (sim << 4)
		+ o->write_type;
	c.page->data[11] = (o->multi << 6) | o->control;
	c.page->data[12] = spc_block_type(o->block_type);
	c.page->data[22] = 0;
	c.page->data[23] = 150;	/* audio pause length */
/*XXX need session format! */
	c.dir = TO_DRIVE;
	d->issue_command(d, &c);
}

void spc_getcaps(struct burn_drive *d)
{
	spc_inquiry(d);
	spc_sense_caps(d);
	spc_sense_error_params(d);
}

/*
only called when a blank is present, so we set type to blank
(on the last pass)

don't check totally stupid modes (raw/raw0)
some drives say they're ok, and they're not.
*/

void spc_probe_write_modes(struct burn_drive *d)
{
	struct buffer buf;
	int try_write_type = 1;
	int try_block_type = 0;
	int key, asc, ascq;
	struct command c;

	while (try_write_type != 4) {
		burn_print(9, "trying %d, %d\n", try_write_type,
			   try_block_type);
		memcpy(c.opcode, SPC_MODE_SELECT, sizeof(SPC_MODE_SELECT));
		c.retry = 1;
		c.oplen = sizeof(SPC_MODE_SELECT);
		c.opcode[8] = 8 + 2 + 0x32;
		c.page = &buf;

		memset(c.page->data, 0, 8 + 2 + 0x32);
		c.page->bytes = 8 + 2 + 0x32;

		c.page->data[8] = 5;
		c.page->data[9] = 0x32;
		c.page->data[10] = try_write_type;
		if (try_block_type > 4)
			c.page->data[11] = 4;
		else
			c.page->data[11] = 0;
		c.page->data[12] = try_block_type;
		c.page->data[23] = 150;
		c.dir = TO_DRIVE;
		d->issue_command(d, &c);

		key = c.sense[2];
		asc = c.sense[12];
		ascq = c.sense[13];

		if (key)
			burn_print(7, "%d not supported\n", try_block_type);
		else {
			burn_print(7, "%d:%d SUPPORTED MODE!\n",
				   try_write_type, try_block_type);
			if (try_write_type == 2)	/* sao */
				d->block_types[try_write_type] =
					BURN_BLOCK_SAO;
			else
				d->block_types[try_write_type] |=
					1 << try_block_type;
		}
		switch (try_block_type) {
		case 0:
		case 1:
		case 2:
			try_block_type++;
			break;
		case 3:
			try_block_type = 8;
			break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
			try_block_type++;
			break;
		case 13:
			try_block_type = 0;
			try_write_type++;
			break;
		default:
			return;
		}
	}
}

/** @return -1 = error */
int spc_block_type(enum burn_block_types b)
{
	switch (b) {
	case BURN_BLOCK_SAO:
		return 0;	/* ignored bitz */
	case BURN_BLOCK_RAW0:
		return 0;
	case BURN_BLOCK_RAW16:
		return 1;
	case BURN_BLOCK_RAW96P:
		return 2;
	case BURN_BLOCK_RAW96R:
		return 3;
	case BURN_BLOCK_MODE1:
		return 8;
	case BURN_BLOCK_MODE2R:
		return 9;
	case BURN_BLOCK_MODE2_PATHETIC:
		return 10;
	case BURN_BLOCK_MODE2_LAME:
		return 11;
	case BURN_BLOCK_MODE2_OBSCURE:
		return 12;
	case BURN_BLOCK_MODE2_OK:
		return 13;
	default:
		return -1;
	}
	/* ts A61007 : already prevented in burn_write_opts_set_write_type() */
	/* a ssert(0); */;
}
