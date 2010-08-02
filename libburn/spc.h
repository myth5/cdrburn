/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/* Copyright (c) 2004 - 2006 Derek Foreman, Ben Jansens
   Copyright (c) 2006 - 2010 Thomas Schmitt <scdbackup@gmx.net>
   Provided under GPL version 2 or later.
*/


#ifndef __SPC
#define __SPC

#include "libburn.h"

void spc_inquiry(struct burn_drive *);
void spc_prevent(struct burn_drive *);
void spc_allow(struct burn_drive *);
void spc_sense_caps(struct burn_drive *);
void spc_sense_error_params(struct burn_drive *);
void spc_select_error_params(struct burn_drive *,
			     const struct burn_read_opts *);
void spc_getcaps(struct burn_drive *d);
void spc_sense_write_params(struct burn_drive *);
void spc_select_write_params(struct burn_drive *,
			     const struct burn_write_opts *);
void spc_probe_write_modes(struct burn_drive *);
void spc_request_sense(struct burn_drive *d, struct buffer *buf);
int spc_block_type(enum burn_block_types b);
int spc_get_erase_progress(struct burn_drive *d);

/* ts A70315 : test_unit_ready with result parameters */
int spc_test_unit_ready_r(struct burn_drive *d, int *key, int *asc, int *ascq);

int spc_test_unit_ready(struct burn_drive *d);

/* ts A70315 */
/** Wait until the drive state becomes clear in or until max_sec elapsed */
int spc_wait_unit_attention(struct burn_drive *d, int max_sec, char *cmd_text,
				int flag);

/* ts A61021 : the spc specific part of sg.c:enumerate_common()
*/
int spc_setup_drive(struct burn_drive *d);

/* ts A61021 : the general SCSI specific part of sg.c:enumerate_common()
   @param flag Bitfield for control purposes
               bit0= do not setup spc/sbc/mmc
*/
int burn_scsi_setup_drive(struct burn_drive *d, int bus_no, int host_no,
			int channel_no, int target_no, int lun_no, int flag);

/* ts A61115 moved from sg-*.h */
enum response { RETRY, FAIL };
enum response scsi_error(struct burn_drive *, unsigned char *, int);

/* ts A61122 */
enum response scsi_error_msg(struct burn_drive *d, unsigned char *sense,
                             int senselen, char msg[161],
                             int *key, int *asc, int *ascq);

/* ts A61030 */
/* @param flag bit0=do report conditions which are considered not an error */
int scsi_notify_error(struct burn_drive *, struct command *c,
			unsigned char *sense, int senselen, int flag);

/* ts A70519 */
int scsi_init_command(struct command *c, unsigned char *opcode, int oplen);

/* ts A91106 */
int scsi_show_cmd_text(struct command *c, void *fp, int flag);

/* ts A91106 */
int scsi_show_cmd_reply(struct command *c, void *fp, int flag);

/* ts A91218 (former sg_log_cmd ts A70518) */
/** Logs command (before execution) */
int scsi_log_cmd(struct command *c, void *fp, int flag);

/* ts A91221 (former sg_log_err ts A91108) */
/** Logs outcome of a sg command. */
int scsi_log_err(struct command *c, void *fp, unsigned char sense[18], 
                 int sense_len, int duration, int flag);

/* ts B00728 */
int spc_decode_sense(unsigned char *sense, int senselen,
                     int *key, int *asc, int *ascq);


#endif /*__SPC*/
