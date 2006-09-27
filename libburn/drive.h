/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __DRIVE
#define __DRIVE

#include "libburn.h"
#include "toc.h"
#include "structure.h"

struct burn_drive;
struct command;
struct mempage;

#define LEAD_IN 1
#define GAP 2
#define USER_DATA 3
#define LEAD_OUT 4
#define SYNC 5

#define SESSION_LEADOUT_ENTRY(d,s) (d)->toc->session[(s)].leadout_entry

#define CURRENT_SESSION_START(d) \
	burn_msf_to_lba(d->toc->session[d->currsession].start_m, \
	                   d->toc->session[d->currsession].start_s, \
	                   d->toc->session[d->currsession].start_f)

#define SESSION_END(d,s) \
	TOC_ENTRY_PLBA((d)->toc, SESSION_LEADOUT_ENTRY((d), (s)))

#define PREVIOUS_SESSION_END(d) \
	TOC_ENTRY_PLBA((d)->toc, SESSION_LEADOUT_ENTRY((d), (d)->currsession-1))

#define LAST_SESSION_END(d) \
	TOC_ENTRY_PLBA((d)->toc, \
	               SESSION_LEADOUT_ENTRY((d), (d)->toc->sessions-1))

struct burn_drive *burn_drive_register(struct burn_drive *);
int burn_drive_unregister(struct burn_drive *d);

unsigned int burn_drive_count(void);
void burn_wait_all(void);
int burn_sector_length_write(struct burn_drive *d);
int burn_track_control(struct burn_drive *d, int);
void burn_write_empty_sector(int fd);
void burn_write_empty_subcode(int fd);
void burn_drive_free(struct burn_drive *d);
void burn_drive_free_all(void);

int burn_drive_scan_sync(struct burn_drive_info *drives[],
			 unsigned int *n_drives);
void burn_disc_erase_sync(struct burn_drive *d, int fast);
int burn_drive_get_block_types(struct burn_drive *d,
			       enum burn_write_types write_type);

/* ts A60822 */
int burn_drive_is_open(struct burn_drive *d);

#endif /* __DRIVE */
