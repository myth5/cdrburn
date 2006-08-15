/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __SG
#define __SG

struct burn_drive;
struct command;

enum response
{ RETRY, FAIL };

void sg_enumerate(void);
void ata_enumerate(void);
int sg_grab(struct burn_drive *);
int sg_release(struct burn_drive *);
int sg_issue_command(struct burn_drive *, struct command *);
enum response scsi_error(struct burn_drive *, unsigned char *, int);

#endif /* __SG */
