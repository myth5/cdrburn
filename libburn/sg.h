/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __SG
#define __SG

struct burn_drive;
struct command;

enum response
{ RETRY, FAIL };

/* ts A60925 : ticket 74 */
int sg_close_drive_fd(char *fname, int driveno, int *fd, int sorry);

/* ts A60922 ticket 33 */
int sg_give_next_adr(int *idx, char adr[], int adr_size, int initialize);
int sg_is_enumerable_adr(char *adr);
int sg_obtain_scsi_adr(char *path, int *host_no, int *channel_no,
                       int *target_no, int *lun_no);

void sg_enumerate(void);
void ata_enumerate(void);
int sg_grab(struct burn_drive *);
int sg_release(struct burn_drive *);
int sg_issue_command(struct burn_drive *, struct command *);
enum response scsi_error(struct burn_drive *, unsigned char *, int);

#endif /* __SG */
