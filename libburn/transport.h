/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __TRANSPORT
#define __TRANSPORT

#include "libburn.h"

#include <pthread.h>
/* sg data structures */
#include <sys/types.h>

#ifdef __FreeBSD__

#define BUFFER_SIZE 65536/2

#else /* __FreeBSD__ */

#define BUFFER_SIZE 65536

#endif /* ! __FreeBSD__ */

enum transfer_direction
{ TO_DRIVE, FROM_DRIVE, NO_TRANSFER };

/* end of sg data structures */

/* generic 'drive' data structures */

struct cue_sheet
{
	int count;
	unsigned char *data;
};

struct params
{
	int speed;
	int retries;
};

struct buffer
{
	unsigned char data[BUFFER_SIZE];
	int sectors;
	int bytes;
};

struct command
{
	unsigned char opcode[16];
	int oplen;
	int dir;
	unsigned char sense[128];
	int error;
	int retry;
	struct buffer *page;
};

struct burn_scsi_inquiry_data
{
	char vendor[9];
	char product[17];
	char revision[5];
	int valid;
};

struct scsi_mode_data
{
	int buffer_size;
	int dvdram_read;
	int dvdram_write;
	int dvdr_read;
	int dvdr_write;
	int dvdrom_read;
	int cdrw_read;
	int cdrw_write;
	int cdr_read;
	int cdr_write;
	int simulate;
	int max_read_speed;
	int max_write_speed;

	/* ts A61021 */
	int min_write_speed;

	int cur_read_speed;
	int cur_write_speed;
	int retry_page_length;
	int retry_page_valid;
	int write_page_length;
	int write_page_valid;
	int c2_pointers;
	int valid;
	int underrun_proof;
};


#define LIBBURN_SG_MAX_SIBLINGS 16

/** Gets initialized in enumerate_common() and burn_drive_register() */
struct burn_drive
{
	int bus_no;
	int host;
	int id;
	int channel;
	int lun;
	char *devname;

#if defined(__FreeBSD__)
	struct cam_device* cam;
#else
	int fd;

	/* ts A60926 : trying to lock against growisofs /dev/srN, /dev/scdN */
	int sibling_count;
	int sibling_fds[LIBBURN_SG_MAX_SIBLINGS];
#endif

	/* ts A60904 : ticket 62, contribution by elmom */
	/**
	    Tells the index in scanned burn_drive_info array.
	    -1 if fallen victim to burn_drive_info_forget()
	*/
	int global_index;

	pthread_mutex_t access_lock;

	enum burn_disc_status status;
	int erasable;
	volatile int released;
	int nwa;		/* next writeable address */
	int alba;		/* absolute lba */
	int rlba;		/* relative lba in section */
	int start_lba;
	int end_lba;
	int toc_temp;
	struct burn_disc *disc;	/* disc structure */
	int block_types[4];
	struct buffer *buffer;
	struct burn_progress progress;

	volatile int cancel;
	volatile enum burn_drive_status busy;
/* transport functions */
	int (*grab) (struct burn_drive *);
	int (*release) (struct burn_drive *);

	/* ts A61021 */
	int (*drive_is_open) (struct burn_drive *);

	int (*issue_command) (struct burn_drive *, struct command *);

/* lower level functions */
	void (*erase) (struct burn_drive *, int);
	void (*getcaps) (struct burn_drive *);

	/* ts A61021 */
	void (*read_atip) (struct burn_drive *);

	int (*write) (struct burn_drive *, int, struct buffer *);
	void (*read_toc) (struct burn_drive *);
	void (*lock) (struct burn_drive *);
	void (*unlock) (struct burn_drive *);
	void (*eject) (struct burn_drive *);
	void (*load) (struct burn_drive *);
	void (*read_disc_info) (struct burn_drive *);
	void (*read_sectors) (struct burn_drive *,
			      int start,
			      int len,
			      const struct burn_read_opts *, struct buffer *);
	void (*perform_opc) (struct burn_drive *);
	void (*set_speed) (struct burn_drive *, int, int);
	void (*send_parameters) (struct burn_drive *,
				 const struct burn_read_opts *);
	void (*send_write_parameters) (struct burn_drive *,
				       const struct burn_write_opts *);
	void (*send_cue_sheet) (struct burn_drive *, struct cue_sheet *);
	void (*sync_cache) (struct burn_drive *);
	int (*get_erase_progress) (struct burn_drive *);
	int (*get_nwa) (struct burn_drive *);

	/* ts A61009 : removed d in favor of o->drive */
	/* void (*close_disc) (struct burn_drive * d,
				 struct burn_write_opts * o);
	   void (*close_session) (struct burn_drive * d,
			       struct burn_write_opts * o);
	*/
	void (*close_disc) (struct burn_write_opts * o);
	void (*close_session) ( struct burn_write_opts * o);

	int (*test_unit_ready) (struct burn_drive * d);
	void (*probe_write_modes) (struct burn_drive * d);
	struct params params;
	struct burn_scsi_inquiry_data *idata;
	struct scsi_mode_data *mdata;
	int toc_entries;
	struct burn_toc_entry *toc_entry;
};

/* end of generic 'drive' data structures */

#endif /* __TRANSPORT */
