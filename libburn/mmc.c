/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "error.h"
#include "sector.h"
#include "libburn.h"
#include "transport.h"
#include "mmc.h"
#include "spc.h"
#include "drive.h"
#include "debug.h"
#include "toc.h"
#include "structure.h"
#include "options.h"

static unsigned char MMC_GET_TOC[] = { 0x43, 2, 2, 0, 0, 0, 0, 16, 0, 0 };
static unsigned char MMC_GET_ATIP[] = { 0x43, 2, 4, 0, 0, 0, 0, 16, 0, 0 };
static unsigned char MMC_GET_DISC_INFO[] =
	{ 0x51, 0, 0, 0, 0, 0, 0, 16, 0, 0 };
static unsigned char MMC_READ_CD[] = { 0xBE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_ERASE[] = { 0xA1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_SEND_OPC[] = { 0x54, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_SET_SPEED[] =
	{ 0xBB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_WRITE_12[] =
	{ 0xAA, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_WRITE_10[] = { 0x2A, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_GET_CONFIGURATION[] =
	{ 0x46, 0, 0, 0, 0, 0, 16, 0, 0 };
static unsigned char MMC_SYNC_CACHE[] = { 0x35, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_GET_EVENT[] = { 0x4A, 1, 0, 0, 16, 0, 0, 0, 8, 0 };
static unsigned char MMC_CLOSE[] = { 0x5B, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned char MMC_TRACK_INFO[] = { 0x52, 0, 0, 0, 0, 0, 0, 16, 0, 0 };
static unsigned char MMC_SEND_CUE_SHEET[] =
	{ 0x5D, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void mmc_send_cue_sheet(struct burn_drive *d, struct cue_sheet *s)
{
	struct buffer buf;
	struct command c;

	c.retry = 1;
	c.oplen = sizeof(MMC_SEND_CUE_SHEET);
	memcpy(c.opcode, MMC_SEND_CUE_SHEET, sizeof(MMC_SEND_CUE_SHEET));
	c.page = &buf;
	c.page->bytes = s->count * 8;
	c.page->sectors = 0;
	c.opcode[6] = (c.page->bytes >> 16) & 0xFF;
	c.opcode[7] = (c.page->bytes >> 8) & 0xFF;
	c.opcode[8] = c.page->bytes & 0xFF;
	c.dir = TO_DRIVE;
	memcpy(c.page->data, s->data, c.page->bytes);
	d->issue_command(d, &c);
}

int mmc_get_nwa(struct burn_drive *d)
{
	struct buffer buf;
	struct command c;
	unsigned char *data;

	c.retry = 1;
	c.oplen = sizeof(MMC_TRACK_INFO);
	memcpy(c.opcode, MMC_TRACK_INFO, sizeof(MMC_TRACK_INFO));
	c.opcode[1] = 1;
	c.opcode[5] = 0xFF;
	c.page = &buf;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);
	data = c.page->data;
	return (data[12] << 24) + (data[13] << 16)
		+ (data[14] << 8) + data[15];
}

void mmc_close_disc(struct burn_drive *d, struct burn_write_opts *o)
{
	assert(o->drive == d);
	o->multi = 0;
	spc_select_write_params(d, o);
	mmc_close(d, 1, 0);
}

void mmc_close_session(struct burn_drive *d, struct burn_write_opts *o)
{
	assert(o->drive == d);
	o->multi = 3;
	spc_select_write_params(d, o);
	mmc_close(d, 1, 0);
}

void mmc_close(struct burn_drive *d, int session, int track)
{
	struct command c;

	c.retry = 1;
	c.oplen = sizeof(MMC_CLOSE);
	memcpy(c.opcode, MMC_CLOSE, sizeof(MMC_CLOSE));
	c.opcode[2] = session | !!track;
	c.opcode[4] = track >> 8;
	c.opcode[5] = track & 0xFF;
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}

void mmc_get_event(struct burn_drive *d)
{
	struct buffer buf;
	struct command c;

	c.retry = 1;
	c.oplen = sizeof(MMC_GET_EVENT);
	memcpy(c.opcode, MMC_GET_EVENT, sizeof(MMC_GET_EVENT));
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);
	burn_print(12, "0x%x:0x%x:0x%x:0x%x\n",
		   c.page->data[0], c.page->data[1], c.page->data[2],
		   c.page->data[3]);
	burn_print(12, "event: %d:%d:%d:%d\n", c.page->data[4],
		   c.page->data[5], c.page->data[6], c.page->data[7]);
}

void mmc_write_12(struct burn_drive *d, int start, struct buffer *buf)
{
	struct command c;
	int len;

	len = buf->sectors;
	assert(buf->bytes >= buf->sectors);	/* can be == at 0... */
	burn_print(100, "trying to write %d at %d\n", len, start);
	memcpy(c.opcode, MMC_WRITE_12, sizeof(MMC_WRITE_12));
	c.retry = 1;
	c.oplen = sizeof(MMC_WRITE_12);
	c.opcode[2] = start >> 24;
	c.opcode[3] = (start >> 16) & 0xFF;
	c.opcode[4] = (start >> 8) & 0xFF;
	c.opcode[5] = start & 0xFF;
	c.opcode[6] = len >> 24;
	c.opcode[7] = (len >> 16) & 0xFF;
	c.opcode[8] = (len >> 8) & 0xFF;
	c.opcode[9] = len & 0xFF;
	c.page = buf;
	c.dir = TO_DRIVE;

	d->issue_command(d, &c);
}

int mmc_write(struct burn_drive *d, int start, struct buffer *buf)
{
	int cancelled;
	struct command c;
	int len;

	pthread_mutex_lock(&d->access_lock);
	cancelled = d->cancel;
	pthread_mutex_unlock(&d->access_lock);

	if (cancelled)
		return BE_CANCELLED;

	len = buf->sectors;
	assert(buf->bytes >= buf->sectors);	/* can be == at 0... */
	burn_print(100, "trying to write %d at %d\n", len, start);
	memcpy(c.opcode, MMC_WRITE_10, sizeof(MMC_WRITE_10));
	c.retry = 1;
	c.oplen = sizeof(MMC_WRITE_10);
	c.opcode[2] = start >> 24;
	c.opcode[3] = (start >> 16) & 0xFF;
	c.opcode[4] = (start >> 8) & 0xFF;
	c.opcode[5] = start & 0xFF;
	c.opcode[6] = 0;
	c.opcode[7] = (len >> 8) & 0xFF;
	c.opcode[8] = len & 0xFF;
	c.page = buf;
	c.dir = TO_DRIVE;
/*
	burn_print(12, "%d, %d, %d, %d - ", c->opcode[2], c->opcode[3], c->opcode[4], c->opcode[5]);
	burn_print(12, "%d, %d, %d, %d\n", c->opcode[6], c->opcode[7], c->opcode[8], c->opcode[9]);
*/

/*	write(fileno(stderr), c.page->data, c.page->bytes);*/

	d->issue_command(d, &c);
	return 0;
}

void mmc_read_toc(struct burn_drive *d)
{
/* read full toc, all sessions, in m/s/f form, 4k buffer */
	struct burn_track *track;
	struct burn_session *session;
	struct buffer buf;
	struct command c;
	int dlen;
	int i;
	unsigned char *tdata;

	memcpy(c.opcode, MMC_GET_TOC, sizeof(MMC_GET_TOC));
	c.retry = 1;
	c.oplen = sizeof(MMC_GET_TOC);
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	if (c.error) {
		d->busy = BURN_DRIVE_IDLE;
		return;
	}

	dlen = c.page->data[0] * 256 + c.page->data[1];
	d->toc_entries = (dlen - 2) / 11;
/*
	some drives fail this check.

	assert(((dlen - 2) % 11) == 0);
*/
	d->toc_entry = malloc(d->toc_entries * sizeof(struct burn_toc_entry));
	tdata = c.page->data + 4;

	burn_print(12, "TOC:\n");

	d->disc = burn_disc_create();

	for (i = 0; i < c.page->data[3]; i++) {
		session = burn_session_create();
		burn_disc_add_session(d->disc, session, BURN_POS_END);
		burn_session_free(session);
	}
	for (i = 0; i < d->toc_entries; i++, tdata += 11) {
		burn_print(12, "S %d, PT %d, TNO %d : ", tdata[0], tdata[3],
			   tdata[2]);
		burn_print(12, "(%d:%d:%d)", tdata[8], tdata[9], tdata[10]);
		burn_print(12, "A(%d:%d:%d)", tdata[4], tdata[5], tdata[6]);
		burn_print(12, " - control %d, adr %d\n", tdata[1] & 0xF,
			   tdata[1] >> 4);

		if (tdata[3] == 1) {
			if (burn_msf_to_lba(tdata[8], tdata[9], tdata[10])) {
				d->disc->session[0]->hidefirst = 1;
				track = burn_track_create();
				burn_session_add_track(d->disc->
						       session[tdata[0] - 1],
						       track, BURN_POS_END);
				burn_track_free(track);

			}
		}
		if (tdata[3] < 100) {
			track = burn_track_create();
			burn_session_add_track(d->disc->session[tdata[0] - 1],
					       track, BURN_POS_END);
			track->entry = &d->toc_entry[i];
			burn_track_free(track);
		}
		d->toc_entry[i].session = tdata[0];
		d->toc_entry[i].adr = tdata[1] >> 4;
		d->toc_entry[i].control = tdata[1] & 0xF;
		d->toc_entry[i].tno = tdata[2];
		d->toc_entry[i].point = tdata[3];
		d->toc_entry[i].min = tdata[4];
		d->toc_entry[i].sec = tdata[5];
		d->toc_entry[i].frame = tdata[6];
		d->toc_entry[i].zero = tdata[7];
		d->toc_entry[i].pmin = tdata[8];
		d->toc_entry[i].psec = tdata[9];
		d->toc_entry[i].pframe = tdata[10];
		if (tdata[3] == 0xA0)
			d->disc->session[tdata[0] - 1]->firsttrack = tdata[8];
		if (tdata[3] == 0xA1)
			d->disc->session[tdata[0] - 1]->lasttrack = tdata[8];
		if (tdata[3] == 0xA2)
			d->disc->session[tdata[0] - 1]->leadout_entry =
				&d->toc_entry[i];
	}
	if (d->status != BURN_DISC_APPENDABLE)
		d->status = BURN_DISC_FULL;
	toc_find_modes(d);
}

void mmc_read_disc_info(struct burn_drive *d)
{
	struct buffer buf;
	unsigned char *data;
	struct command c;

	memcpy(c.opcode, MMC_GET_DISC_INFO, sizeof(MMC_GET_DISC_INFO));
	c.retry = 1;
	c.oplen = sizeof(MMC_GET_DISC_INFO);
	c.page = &buf;
	c.page->sectors = 0;
	c.page->bytes = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	if (c.error) {
		d->busy = BURN_DRIVE_IDLE;
		return;
	}

	data = c.page->data;
	d->erasable = !!(data[2] & 16);
	switch (data[2] & 3) {
	case 0:
		d->toc_entries = 0;
		d->start_lba = burn_msf_to_lba(data[17], data[18], data[19]);
		d->end_lba = burn_msf_to_lba(data[21], data[22], data[23]);
		d->status = BURN_DISC_BLANK;
		break;
	case 1:
		d->status = BURN_DISC_APPENDABLE;
	case 2:
		mmc_read_toc(d);
		break;
	}
}

void mmc_read_atip(struct burn_drive *d)
{
	struct buffer buf;
	struct command c;

	memcpy(c.opcode, MMC_GET_ATIP, sizeof(MMC_GET_ATIP));
	c.retry = 1;
	c.oplen = sizeof(MMC_GET_ATIP);
	c.page = &buf;
	c.page->bytes = 0;
	c.page->sectors = 0;

	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);
	burn_print(1, "atip shit for you\n");
}

void mmc_read_sectors(struct burn_drive *d,
		      int start,
		      int len,
		      const struct burn_read_opts *o, struct buffer *buf)
{
	int temp;
	int errorblock, req;
	struct command c;

	assert(len >= 0);
/* if the drive isn't busy, why the hell are we here? */
	assert(d->busy);
	burn_print(12, "reading %d from %d\n", len, start);
	memcpy(c.opcode, MMC_READ_CD, sizeof(MMC_READ_CD));
	c.retry = 1;
	c.oplen = sizeof(MMC_READ_CD);
	temp = start;
	c.opcode[5] = temp & 0xFF;
	temp >>= 8;
	c.opcode[4] = temp & 0xFF;
	temp >>= 8;
	c.opcode[3] = temp & 0xFF;
	temp >>= 8;
	c.opcode[2] = temp & 0xFF;
	c.opcode[8] = len & 0xFF;
	len >>= 8;
	c.opcode[7] = len & 0xFF;
	len >>= 8;
	c.opcode[6] = len & 0xFF;
	req = 0xF8;
	if (d->busy == BURN_DRIVE_GRABBING || o->report_recovered_errors)
		req |= 2;

	c.opcode[10] = 0;
/* always read the subcode, throw it away later, since we don't know
   what we're really reading
*/
	if (d->busy == BURN_DRIVE_GRABBING || (o->subcodes_audio)
	    || (o->subcodes_data))
		c.opcode[10] = 1;

	c.opcode[9] = req;
	c.page = buf;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	if (c.error) {
		burn_print(12, "got an error over here\n");
		burn_print(12, "%d, %d, %d, %d\n", c.sense[3], c.sense[4],
			   c.sense[5], c.sense[6]);
		errorblock =
			(c.sense[3] << 24) + (c.sense[4] << 16) +
			(c.sense[5] << 8) + c.sense[6];
		c.page->sectors = errorblock - start + 1;
		burn_print(1, "error on block %d\n", errorblock);
		burn_print(12, "error on block %d\n", errorblock);
		burn_print(12, "returning %d sectors\n", c.page->sectors);
	}
}

void mmc_erase(struct burn_drive *d, int fast)
{
	struct command c;

	memcpy(c.opcode, MMC_ERASE, sizeof(MMC_ERASE));
	c.opcode[1] = 16;	/* IMMED set to 1 */
	c.opcode[1] |= !!fast;
	c.retry = 1;
	c.oplen = sizeof(MMC_ERASE);
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}

void mmc_read_lead_in(struct burn_drive *d, struct buffer *buf)
{
	int len;
	struct command c;

	len = buf->sectors;
	memcpy(c.opcode, MMC_READ_CD, sizeof(MMC_READ_CD));
	c.retry = 1;
	c.oplen = sizeof(MMC_READ_CD);
	c.opcode[5] = 0;
	c.opcode[4] = 0;
	c.opcode[3] = 0;
	c.opcode[2] = 0xF0;
	c.opcode[8] = 1;
	c.opcode[7] = 0;
	c.opcode[6] = 0;
	c.opcode[9] = 0;
	c.opcode[10] = 2;
	c.page = buf;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);
}

void mmc_perform_opc(struct burn_drive *d)
{
	struct command c;

	memcpy(c.opcode, MMC_SEND_OPC, sizeof(MMC_SEND_OPC));
	c.retry = 1;
	c.oplen = sizeof(MMC_SEND_OPC);
	c.opcode[1] = 1;
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}

void mmc_set_speed(struct burn_drive *d, int r, int w)
{
	struct command c;

	memcpy(c.opcode, MMC_SET_SPEED, sizeof(MMC_SET_SPEED));
	c.retry = 1;
	c.oplen = sizeof(MMC_SET_SPEED);
	c.opcode[2] = r >> 8;
	c.opcode[3] = r & 0xFF;
	c.opcode[4] = w >> 8;
	c.opcode[5] = w & 0xFF;
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}

void mmc_get_configuration(struct burn_drive *d)
{
	struct buffer buf;
	int len;
	struct command c;

	memcpy(c.opcode, MMC_GET_CONFIGURATION, sizeof(MMC_GET_CONFIGURATION));
	c.retry = 1;
	c.oplen = sizeof(MMC_GET_CONFIGURATION);
	c.page = &buf;
	c.page->sectors = 0;
	c.page->bytes = 0;
	c.dir = FROM_DRIVE;
	d->issue_command(d, &c);

	burn_print(1, "got it back\n");
	len = (c.page->data[0] << 24)
		+ (c.page->data[1] << 16)
		+ (c.page->data[2] << 8)
		+ c.page->data[3];
	burn_print(1, "all %d bytes of it\n", len);
	burn_print(1, "%d, %d, %d, %d\n",
		   c.page->data[0],
		   c.page->data[1], c.page->data[2], c.page->data[3]);
}

void mmc_sync_cache(struct burn_drive *d)
{
	struct command c;

	memcpy(c.opcode, MMC_SYNC_CACHE, sizeof(MMC_SYNC_CACHE));
	c.retry = 1;
	c.oplen = sizeof(MMC_SYNC_CACHE);
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}
