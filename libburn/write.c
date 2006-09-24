/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "error.h"
#include "sector.h"
#include "libburn.h"
#include "drive.h"
#include "transport.h"
#include "message.h"
#include "crc.h"
#include "debug.h"
#include "init.h"
#include "lec.h"
#include "toc.h"
#include "util.h"
#include "sg.h"
#include "write.h"
#include "options.h"

static int type_to_ctrl(int mode)
{
	int ctrl = 0;

	int data = BURN_MODE2 | BURN_MODE1 | BURN_MODE0;

	if (mode & data) {
		ctrl |= 4;
	} else if (mode & BURN_AUDIO) {
		if (mode & BURN_4CH)
			ctrl |= 8;
		if (mode & BURN_PREEMPHASIS)
			ctrl |= 1;
	} else
		assert(0);

	if (mode & BURN_COPY)
		ctrl |= 2;

	return ctrl;
}

/* only the ctrl nibble is set here (not adr) */
static void type_to_form(int mode, unsigned char *ctladr, int *form)
{
	*ctladr = type_to_ctrl(mode) << 4;

	if (mode & BURN_AUDIO)
		*form = 0;
	if (mode & BURN_MODE0)
		assert(0);
	if (mode & BURN_MODE1)
		*form = 0x10;
	if (mode & BURN_MODE2)
		assert(0);	/* XXX someone's gonna want this sometime */
	if (mode & BURN_MODE_RAW)
		*form = 0;
	if (mode & BURN_SUBCODE_P16)	/* must be expanded to R96 */
		*form |= 0x40;
	if (mode & BURN_SUBCODE_P96)
		*form |= 0xC0;
	if (mode & BURN_SUBCODE_R96)
		*form |= 0x40;
}

int burn_write_flush(struct burn_write_opts *o)
{
	struct burn_drive *d = o->drive;

	if (d->buffer->bytes && !d->cancel) {
		int err;
		err = d->write(d, d->nwa, d->buffer);
		if (err == BE_CANCELLED)
			return 0;
		d->nwa += d->buffer->sectors;
	}
	d->sync_cache(d);
	return 1;
}


/* ts A60819:
   This is unused since about Feb 2006, icculus.org/burn CVS.
   The compiler complains. We shall please our compiler.
*/
#ifdef Libburn_write_with_function_print_cuE

static void print_cue(struct cue_sheet *sheet)
{
	int i;
	unsigned char *unit;

	printf("\n");
	printf("ctladr|trno|indx|form|scms|  msf\n");
	printf("------+----+----+----+----+--------\n");
	for (i = 0; i < sheet->count; i++) {
		unit = sheet->data + 8 * i;
		printf(" %1X  %1X | %02X | %02X | %02X | %02X |",
		       (unit[0] & 0xf0) >> 4, unit[0] & 0xf, unit[1], unit[2],
		       unit[3], unit[4]);
		printf("%02X:%02X:%02X\n", unit[5], unit[6], unit[7]);
	}
}

#endif /* Libburn_write_with_print_cuE */


static void add_cue(struct cue_sheet *sheet, unsigned char ctladr,
		    unsigned char tno, unsigned char indx,
		    unsigned char form, unsigned char scms, int lba)
{
	unsigned char *unit;
	unsigned char *ptr;
	int m, s, f;

	burn_lba_to_msf(lba, &m, &s, &f);

	sheet->count++;
	ptr = realloc(sheet->data, sheet->count * 8);
	assert(ptr);
	sheet->data = ptr;
	unit = sheet->data + (sheet->count - 1) * 8;
	unit[0] = ctladr;
	unit[1] = tno;
	unit[2] = indx;
	unit[3] = form;
	unit[4] = scms;
	unit[5] = m;
	unit[6] = s;
	unit[7] = f;
}

struct cue_sheet *burn_create_toc_entries(struct burn_write_opts *o,
					  struct burn_session *session)
{
	int i, m, s, f, form, pform, runtime = -150;
	unsigned char ctladr;
	struct burn_drive *d;
	struct burn_toc_entry *e;
	struct cue_sheet *sheet;
	struct burn_track **tar = session->track;
	int ntr = session->tracks;
	int rem = 0;

	d = o->drive;

	sheet = malloc(sizeof(struct cue_sheet));
	sheet->data = NULL;
	sheet->count = 0;

	type_to_form(tar[0]->mode, &ctladr, &form);
	add_cue(sheet, ctladr | 1, 0, 0, 1, 0, runtime);
	add_cue(sheet, ctladr | 1, 1, 0, form, 0, runtime);
	runtime += 150;

	burn_print(1, "toc for %d tracks:\n", ntr);
	d->toc_entries = ntr + 3;
	assert(d->toc_entry == NULL);
	d->toc_entry = malloc(d->toc_entries * sizeof(struct burn_toc_entry));
	e = d->toc_entry;
	memset((void *)e, 0, d->toc_entries * sizeof(struct burn_toc_entry));
	e[0].point = 0xA0;
	if (tar[0]->mode & BURN_AUDIO)
		e[0].control = TOC_CONTROL_AUDIO;
	else
		e[0].control = TOC_CONTROL_DATA;
	e[0].pmin = 1;
	e[0].psec = o->format;
	e[0].adr = 1;
	e[1].point = 0xA1;
	e[1].pmin = ntr;
	e[1].adr = 1;
	if (tar[ntr - 1]->mode & BURN_AUDIO)
		e[1].control = TOC_CONTROL_AUDIO;
	else
		e[1].control = TOC_CONTROL_DATA;
	e[2].point = 0xA2;
	e[2].control = e[1].control;
	e[2].adr = 1;

	tar[0]->pregap2 = 1;
	pform = form;
	for (i = 0; i < ntr; i++) {
		type_to_form(tar[i]->mode, &ctladr, &form);
		if (pform != form) {
			add_cue(sheet, ctladr | 1, i + 1, 0, form, 0, runtime);
			runtime += 150;
/* XXX fix pregap interval 1 for data tracks */
/* ts A60813 silence righteous compiler warning about C++ style comments
   This is possibly not a comment but rather a trace of Derek Foreman
   experiments. Thus not to be beautified - but to be preserved rectified.
/ /                      if (!(form & BURN_AUDIO))
/ /                              tar[i]->pregap1 = 1;
*/
			tar[i]->pregap2 = 1;
		}
/* XXX HERE IS WHERE WE DO INDICES IN THE CUE SHEET */
/* XXX and we should make sure the gaps conform to ecma-130... */
		tar[i]->entry = &e[3 + i];
		e[3 + i].point = i + 1;
		burn_lba_to_msf(runtime, &m, &s, &f);
		e[3 + i].pmin = m;
		e[3 + i].psec = s;
		e[3 + i].pframe = f;
		e[3 + i].adr = 1;
		e[3 + i].control = type_to_ctrl(tar[i]->mode);
		burn_print(1, "track %d control %d\n", tar[i]->mode,
			   e[3 + i].control);
		add_cue(sheet, ctladr | 1, i + 1, 1, form, 0, runtime);
		runtime += burn_track_get_sectors(tar[i]);
/* if we're padding, we'll clear any current shortage.
   if we're not, we'll slip toc entries by a sector every time our
   shortage is more than a sector
XXX this is untested :)
*/
		if (!tar[i]->pad) {
			rem += burn_track_get_shortage(tar[i]);
			if (i +1 != ntr)
				tar[i]->source->next = tar[i+1]->source;
		} else if (rem) {
			rem = 0;
			runtime++;
		}
		if (rem > burn_sector_length(tar[i]->mode)) {
			rem -= burn_sector_length(tar[i]->mode);
			runtime--;
		}
		pform = form;
	}
	burn_lba_to_msf(runtime, &m, &s, &f);
	e[2].pmin = m;
	e[2].psec = s;
	e[2].pframe = f;
	burn_print(1, "run time is %d (%d:%d:%d)\n", runtime, m, s, f);
	for (i = 0; i < d->toc_entries; i++)
		burn_print(1, "point %d (%02d:%02d:%02d)\n",
			   d->toc_entry[i].point, d->toc_entry[i].pmin,
			   d->toc_entry[i].psec, d->toc_entry[i].pframe);
	add_cue(sheet, ctladr | 1, 0xAA, 1, 1, 0, runtime);
	return sheet;
}

int burn_sector_length(int tracktype)
{
	if (tracktype & BURN_AUDIO)
		return 2352;
	if (tracktype & BURN_MODE_RAW)
		return 2352;
	if (tracktype & BURN_MODE1)
		return 2048;
	assert(0);
	return 12345;
}

int burn_subcode_length(int tracktype)
{
	if (tracktype & BURN_SUBCODE_P16)
		return 16;
	if ((tracktype & BURN_SUBCODE_P96) || (tracktype & BURN_SUBCODE_R96))
		return 96;
	return 0;
}

int burn_write_leadin(struct burn_write_opts *o,
		       struct burn_session *s, int first)
{
	struct burn_drive *d = o->drive;
	int count;

	d->busy = BURN_DRIVE_WRITING_LEADIN;

	burn_print(5, first ? "    first leadin\n" : "    leadin\n");

	if (first)
		count = 0 - d->alba - 150;
	else
		count = 4500;

	d->progress.start_sector = d->alba;
	d->progress.sectors = count;
	d->progress.sector = 0;

	while (count != 0) {
		if (!sector_toc(o, s->track[0]->mode))
			return 0;
		count--;
		d->progress.sector++;
	}
	d->busy = BURN_DRIVE_WRITING;
	return 1;
}

int burn_write_leadout(struct burn_write_opts *o,
			int first, unsigned char control, int mode)
{
	struct burn_drive *d = o->drive;
	int count;

	d->busy = BURN_DRIVE_WRITING_LEADOUT;

	d->rlba = -150;
	burn_print(5, first ? "    first leadout\n" : "    leadout\n");
	if (first)
		count = 6750;
	else
		count = 2250;
	d->progress.start_sector = d->alba;
	d->progress.sectors = count;
	d->progress.sector = 0;

	while (count != 0) {
		if (!sector_lout(o, control, mode))
			return 0;
		count--;
		d->progress.sector++;
	}
	d->busy = BURN_DRIVE_WRITING;
	return 1;
}

int burn_write_session(struct burn_write_opts *o, struct burn_session *s)
{
	struct burn_drive *d = o->drive;
	struct burn_track *prev = NULL, *next = NULL;
	int i;

	d->rlba = 0;
	burn_print(1, "    writing a session\n");
	for (i = 0; i < s->tracks; i++) {
		if (i > 0)
			prev = s->track[i - 1];
		if (i + 1 < s->tracks)
			next = s->track[i + 1];
		else
			next = NULL;

		if (!burn_write_track(o, s, i))
			return 0;
	}
	return 1;
}

int burn_write_track(struct burn_write_opts *o, struct burn_session *s,
		      int tnum)
{
	struct burn_track *t = s->track[tnum];
	struct burn_drive *d = o->drive;
	int i, tmp = 0;
	int sectors;

	d->rlba = -150;

/* XXX for tao, we don't want the pregaps  but still want post? */
	if (o->write_type != BURN_WRITE_TAO) {
		if (t->pregap1)
			d->rlba += 75;
		if (t->pregap2)
			d->rlba += 150;

		if (t->pregap1) {
			struct burn_track *pt = s->track[tnum - 1];

			if (tnum == 0) {
				printf("first track should not have a pregap1\n");
				pt = t;
			}
			for (i = 0; i < 75; i++)
				if (!sector_pregap(o, t->entry->point,
					           pt->entry->control, pt->mode))
					return 0;
		}
		if (t->pregap2)
			for (i = 0; i < 150; i++)
				if (!sector_pregap(o, t->entry->point,
					           t->entry->control, t->mode))
					return 0;
	} else {
		o->control = t->entry->control;
		d->send_write_parameters(d, o);
	}

/* user data */
	sectors = burn_track_get_sectors(t);

	/* Update progress */
	d->progress.start_sector = d->nwa;
	d->progress.sectors = sectors;
	d->progress.sector = 0;

	/* ts A60831: added tnum-line, extended print message on proposal
           by bonfire-app@wanadoo.fr in http://libburn.pykix.org/ticket/58 */
        d->progress.track = tnum;
        burn_print(12, "track %d is %d sectors long\n", tnum, sectors);

	if (tnum == s->tracks)
		tmp = sectors > 150 ? 150 : sectors;

	for (i = 0; i < sectors - tmp; i++) {
		if (!sector_data(o, t, 0))
			return 0;

		/* update current progress */
		d->progress.sector++;
	}
	for (; i < sectors; i++) {
		burn_print(1, "last track, leadout prep\n");
		if (!sector_data(o, t, 1))
			return 0;

		/* update progress */
		d->progress.sector++;
	}

	if (t->postgap)
		for (i = 0; i < 150; i++)
			if (!sector_postgap(o, t->entry->point, t->entry->control,
				            t->mode))
				return 0;
	i = t->offset;
	if (o->write_type == BURN_WRITE_SAO) {
		if (d->buffer->bytes) {
			int err;
			err = d->write(d, d->nwa, d->buffer);
			if (err == BE_CANCELLED)
				return 0;
			d->nwa += d->buffer->sectors;
			d->buffer->bytes = 0;
			d->buffer->sectors = 0;
		}
	}
	if (o->write_type == BURN_WRITE_TAO)
		if (!burn_write_flush(o))
			return 0;
	return 1;
}

void burn_disc_write_sync(struct burn_write_opts *o, struct burn_disc *disc)
{
	struct cue_sheet *sheet;
	struct burn_drive *d = o->drive;
	struct buffer buf;
	struct burn_track *lt;
	int first = 1, i;
	int res;

/* ts A60924 : libburn/message.c gets obsoleted
	burn_message_clear_queue();
*/

	burn_print(1, "sync write of %d sessions\n", disc->sessions);
	d->buffer = &buf;
	memset(d->buffer, 0, sizeof(struct buffer));

	d->rlba = -150;

	d->toc_temp = 9;

/* Apparently some drives require this command to be sent, and a few drives
return crap.  so we send the command, then ignore the result.
*/
	res = d->get_nwa(d);
/*	printf("ignored nwa: %d\n", res);*/

	d->alba = d->start_lba;
	d->nwa = d->alba;

	if (o->write_type != BURN_WRITE_TAO)
		d->send_write_parameters(d, o);

	/* init progress before showing the state */
	d->progress.session = 0;
	d->progress.sessions = disc->sessions;
	d->progress.track = 0;
	d->progress.tracks = disc->session[0]->tracks;
	/* TODO: handle indices */
	d->progress.index = 0;
	d->progress.indices = disc->session[0]->track[0]->indices;
	/* TODO: handle multissession discs */
	/* XXX: sectors are only set during write track */
	d->progress.start_sector = 0;
	d->progress.sectors = 0;
	d->progress.sector = 0;

	d->busy = BURN_DRIVE_WRITING;

	for (i = 0; i < disc->sessions; i++) {
		/* update progress */
		d->progress.session = i;
		d->progress.tracks = disc->session[i]->tracks;

		sheet = burn_create_toc_entries(o, disc->session[i]);
/*		print_cue(sheet);*/
		if (o->write_type == BURN_WRITE_SAO)
			d->send_cue_sheet(d, sheet);
		free(sheet);

		if (o->write_type == BURN_WRITE_RAW) {
			if (!burn_write_leadin(o, disc->session[i], first))
				goto fail;
		} else {
			if (first) {
				d->nwa = -150;
				d->alba = -150;
			} else {
				d->nwa += 4500;
				d->alba += 4500;
			}
		}
		if (!burn_write_session(o, disc->session[i]))
			goto fail;

		lt = disc->session[i]->track[disc->session[i]->tracks - 1];
		if (o->write_type == BURN_WRITE_RAW) {
			if (!burn_write_leadout(o, first, lt->entry->control,
			                        lt->mode))
				goto fail;
		} else {
			if (!burn_write_flush(o))
				goto fail;
			d->nwa += first ? 6750 : 2250;
			d->alba += first ? 6750 : 2250;
		}
		if (first)
			first = 0;

		/* XXX: currently signs an end of session */
		d->progress.sector = 0;
		d->progress.start_sector = 0;
		d->progress.sectors = 0;
	}
	if (o->write_type != BURN_WRITE_SAO)
		if (!burn_write_flush(o))
			goto fail;
	sleep(1);

	burn_print(1, "done\n");
	d->busy = BURN_DRIVE_IDLE;

fail:
	d->sync_cache(d);
	burn_print(1, "done - failed\n");
	d->busy = BURN_DRIVE_IDLE;
}
