/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __MMC
#define __MMC

struct burn_drive;
struct burn_write_opts;
struct command;
struct buffer;
struct cue_sheet;

/* MMC commands */

void mmc_read(struct burn_drive *);

/* ts A61009 : removed redundant parameter d in favor of o->drive */
/* void mmc_close_session(struct burn_drive *, struct burn_write_opts *); */
/* void mmc_close_disc(struct burn_drive *, struct burn_write_opts *); */
void mmc_close_session(struct burn_write_opts *o);
void mmc_close_disc(struct burn_write_opts *o);

void mmc_close(struct burn_drive *, int session, int track);
void mmc_get_event(struct burn_drive *);
int mmc_write(struct burn_drive *, int start, struct buffer *buf);
void mmc_write_12(struct burn_drive *d, int start, struct buffer *buf);
void mmc_sync_cache(struct burn_drive *);
void mmc_load(struct burn_drive *);
void mmc_eject(struct burn_drive *);
void mmc_erase(struct burn_drive *, int);
void mmc_read_toc(struct burn_drive *);
void mmc_read_disc_info(struct burn_drive *);
void mmc_read_atip(struct burn_drive *);
void mmc_read_sectors(struct burn_drive *,
		      int,
		      int, const struct burn_read_opts *, struct buffer *);
void mmc_set_speed(struct burn_drive *, int, int);
void mmc_read_lead_in(struct burn_drive *, struct buffer *);
void mmc_perform_opc(struct burn_drive *);
void mmc_get_configuration(struct burn_drive *);
int mmc_get_nwa(struct burn_drive *);
void mmc_send_cue_sheet(struct burn_drive *, struct cue_sheet *);
#endif /*__MMC*/
