/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/*

This is the main operating system dependent SCSI part of libburn. It implements
the transport level aspects of SCSI control and command i/o.

Present implementation: GNU libcdio , for X/Open compliant operating systems


PORTING:

Porting libburn typically will consist of adding a new operating system case
to the following switcher files:
  os.h    Operating system specific libburn definitions and declarations.
  sg.c    Operating system dependent transport level modules.
and of deriving the following system specific files from existing examples:
  os-*.h  Included by os.h. You will need some general system knowledge
          about signals and knowledge about the storage object needs of your
          transport level module sg-*.c.

  sg-*.c  This source module. You will need special system knowledge about
          how to detect all potentially available drives, how to open them,
          eventually how to exclusively reserve them, how to perform
          SCSI transactions, how to inquire the (pseudo-)SCSI driver.
          You will not need to care about CD burning, MMC or other high-level
          SCSI aspects.

Said sg-*.c operations are defined by a public function interface, which has
to be implemented in a way that provides libburn with the desired services:

sg_initialize()         performs global initialization of the SCSI transport
                        adapter and eventually needed operating system
                        facilities. Checks for compatibility of supporting
                        software components.
 
sg_give_next_adr()      iterates over the set of potentially useful drive 
                        address strings.

scsi_enumerate_drives() brings all available, not-whitelist-banned, and
                        accessible drives into libburn's list of drives.

sg_drive_is_open()      tells wether libburn has the given drive in use.

sg_grab()               opens the drive for SCSI commands and ensures
                        undisturbed access.

sg_release()            closes a drive opened by sg_grab()

sg_issue_command()      sends a SCSI command to the drive, receives reply,
                        and evaluates wether the command succeeded or shall
                        be retried or finally failed.

sg_obtain_scsi_adr()    tries to obtain SCSI address parameters.

burn_os_stdio_capacity()  estimates the emulated media space of stdio-drives.

burn_os_open_track_src()  opens a disk file in a way that allows best
                        throughput with file reading and/or SCSI write command
                        transmission.

burn_os_alloc_buffer()  allocates a memory area that is suitable for file
                        descriptors issued by burn_os_open_track_src().
                        The buffer size may be rounded up for alignment
                        reasons.

burn_os_free_buffer()   delete a buffer obtained by burn_os_alloc_buffer().

Porting hints are marked by the text "PORTING:".
Send feedback to libburn-hackers@pykix.org .

*/


/** PORTING : ------- OS dependent headers and definitions ------ */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#ifdef Libburn_os_has_statvfS
#include <sys/statvfs.h>
#endif /* Libburn_os_has_stavtfS */


#include <cdio/cdio.h>
#include <cdio/mmc.h>


/* The waiting time before eventually retrying a failed SCSI command.
   Before each retry wait Libburn_sg_linux_retry_incR longer than with
   the previous one.
*/
#define Libburn_sg_libcdio_retry_usleeP 100000
#define Libburn_sg_libcdio_retry_incR   100000


/** PORTING : ------ libburn portable headers and definitions ----- */

#include "transport.h"
#include "drive.h"
#include "sg.h"
#include "spc.h"
/* collides with symbols of <cdio/mmc.h>
 #include "mmc.h"
*/
#include "sbc.h"
#include "debug.h"
#include "toc.h"
#include "util.h"

#include "libdax_msgs.h"
extern struct libdax_msgs *libdax_messenger;


/* is in portable part of libburn */
int burn_drive_is_banned(char *device_address);


/* Whether to log SCSI commands:
   bit0= log in /tmp/libburn_sg_command_log
   bit1= log to stderr
   bit2= flush every line
*/
extern int burn_sg_log_scsi;


/* ------------------------------------------------------------------------ */
/* PORTING:   Private definitions. Port only if needed by public functions. */
/*            (Public functions are listed below)                           */
/* ------------------------------------------------------------------------ */


/* Storage object is in libburn/init.c
   whether to strive for exclusive access to the drive
*/
extern int burn_sg_open_o_excl;


/* ------------------------------------------------------------------------ */
/* PORTING: Private functions. Port only if needed by public functions      */
/*          (Public functions are listed below)                             */
/* ------------------------------------------------------------------------ */


static int sg_close_drive(struct burn_drive * d)
{
	CdIo_t *p_cdio;

	if (d->p_cdio != NULL) {
		p_cdio = (CdIo_t *) d->p_cdio;
		cdio_destroy(p_cdio);
		d->p_cdio = NULL;
	}
	return 0;
}


static int sg_give_next_adr_raw(burn_drive_enumerator_t *idx,
				     char adr[], int adr_size, int initialize)
{
	if (initialize == 1) {
		idx->pos = idx->ppsz_cd_drives =
					cdio_get_devices(DRIVER_DEVICE);
		if (idx->ppsz_cd_drives == NULL)
			return 0;
	} else if (initialize == -1) {
		if (*(idx->ppsz_cd_drives) != NULL)
			cdio_free_device_list(idx->ppsz_cd_drives);
		idx->ppsz_cd_drives = NULL;
	}
	if (idx->pos == NULL)
		return 0;
	if (*(idx->pos) == NULL)
		return 0;
	if (strlen(*(idx->pos)) >= adr_size)
		return -1;
	strcpy(adr, *(idx->pos));
	(idx->pos)++;
	return 1;
}


/* Resolve eventual softlink, E.g. /dev/cdrom . */
static int sg_resolve_link(char *in_path, char target[], int target_size,
			   int flag)
{
	int i, max_link_depth = 100, ret;
	char path[4096], link_target[4096];
	struct stat stbuf;

	if (strlen(in_path) >= sizeof(path)) {
		if (strlen(in_path) >= target_size)
			return -1;
		strcpy(target, path);
		return 1;
	}
	strcpy(path, in_path);

	/* (burn_drive_resolve_link() relies on a completed drive list and
	    cannot be used here) */
	for (i= 0; i < max_link_depth; i++) {
		if (lstat(path, &stbuf) == -1) {
			strcpy(path, in_path);
	break; /* dead link */
		}
		if ((stbuf.st_mode & S_IFMT) != S_IFLNK)
	break; /* found target */
		ret = readlink(path, link_target, sizeof(link_target));
		if (ret == -1) {
			strcpy(path, in_path);
	break; /* unreadable link pointer */
		}
		strcpy(path, link_target);
	}
	if (i >= max_link_depth) /* endless link loop */
			strcpy(path, in_path);
	if (strlen(path) >= target_size)
		return -1;
	strcpy(target, path);
	return 1;
}


/* ----------------------------------------------------------------------- */
/* PORTING: Private functions which contain publicly needed functionality. */
/*          Their portable part must be performed. So it is probably best  */
/*          to replace the non-portable part and to call these functions   */
/*          in your port, too.                                             */
/* ----------------------------------------------------------------------- */


/** Wraps a detected drive into libburn structures and hands it over to
    libburn drive list.
*/
static void enumerate_common(char *fname, char *cdio_name,
				int bus_no, int host_no,
				int channel_no, int target_no, int lun_no)
{
	int ret;
	struct burn_drive out;

	/* General libburn drive setup */
	burn_setup_drive(&out, fname);

	/* This transport adapter uses SCSI-family commands and models
	   (seems the adapter would know better than its boss, if ever) */
	ret = burn_scsi_setup_drive(&out, bus_no, host_no, channel_no,
                                 target_no, lun_no, 0);
        if (ret <= 0)
                return;

	/* PORTING: ------------------- non portable part --------------- */

	/* Transport adapter is libcdio */
	/* Adapter specific handles and data */
	out.p_cdio = NULL;
	strcpy(out.libcdio_name, fname);
	if (strlen(cdio_name) < sizeof(out.libcdio_name))
		strcpy(out.libcdio_name, cdio_name);

	/* PORTING: ---------------- end of non portable part ------------ */

	/* Adapter specific functions with standardized names */
	out.grab = sg_grab;
	out.release = sg_release;
	out.drive_is_open = sg_drive_is_open;
	out.issue_command = sg_issue_command;
	/* Finally register drive and inquire drive information */
	burn_drive_finish_enum(&out);
}


/* ------------------------------------------------------------------------ */
/* PORTING:           Public functions. These MUST be ported.               */
/* ------------------------------------------------------------------------ */


/** Performs global initialization of the SCSI transport adapter and eventually
    needed operating system facilities. Checks for compatibility of supporting
    software components.
    @param msg   returns ids and/or error messages of eventual helpers
    @param flag  unused yet, submit 0
    @return      1 = success, <=0 = failure
*/ 
int sg_initialize(char msg[1024], int flag)
{
	char *version_text, *msg_pt;
	int cdio_ver;

	sprintf(msg, "sg-libcdio adapter v%d with libcdio version ",
		LIBCDIO_VERSION_NUM);

 #if LIBCDIO_VERSION_NUM < 83 

LIBBURN_MISCONFIGURATION = 0;
INTENTIONAL_ABORT_OF_COMPILATION__HEADERFILE_cdio_version_dot_h_TOO_OLD__NEED_LIBCDIO_VERSION_NUM_83 = 0;
LIBBURN_MISCONFIGURATION_ = 0;

 #else

	cdio_ver = libcdio_version_num;
	version_text = (char *) cdio_version_string;

 #endif /* ! LIBCDIO_VERSION_NUM < 83  */

	strncat(msg, version_text, 800);
	libdax_msgs_submit(libdax_messenger, -1, 0x00000002,
		LIBDAX_MSGS_SEV_DEBUG, LIBDAX_MSGS_PRIO_HIGH,
		msg , 0, 0);
	if (cdio_ver < LIBCDIO_VERSION_NUM) {
		strcat(msg, " ---> ");
		msg_pt = msg + strlen(msg);
		sprintf(msg_pt,
		    "libcdio TOO OLD: numeric version %d , need at least %d",
		    cdio_ver, LIBCDIO_VERSION_NUM);
		libdax_msgs_submit(libdax_messenger, -1,
			0x00000002,
			LIBDAX_MSGS_SEV_DEBUG, LIBDAX_MSGS_PRIO_HIGH,
			msg_pt, 0, 0);
		return 0;
	}
	return 1;
}


/** Returns the next index number and the next enumerated drive address.
    The enumeration has to cover all available and accessible drives. It is
    allowed to return addresses of drives which are not available but under
    some (even exotic) circumstances could be available. It is on the other
    hand allowed, only to hand out addresses which can really be used right
    in the moment of this call. (This implementation chooses the latter.)
    @param idx An opaque handle. Make no own theories about it.
    @param adr Takes the reply
    @param adr_size Gives maximum size of reply including final 0
    @param initialize  1 = start new,
                       0 = continue, use no other values for now
                      -1 = finish
    @return 1 = reply is a valid address , 0 = no further address available
           -1 = severe error (e.g. adr_size too small)
*/
int sg_give_next_adr(burn_drive_enumerator_t *idx,
		     char adr[], int adr_size, int initialize)
{
	int ret;
	char path[4096];

	ret = sg_give_next_adr_raw(idx, adr, adr_size, initialize);
	if (ret <= 0)
		return ret;
	if (strlen(adr) >= sizeof(path))
		return ret;
	strcpy(path, adr);
	ret = sg_resolve_link(path, adr, adr_size, 0);
	return ret;
}


/** Brings all available, not-whitelist-banned, and accessible drives into
    libburn's list of drives.
*/
int scsi_enumerate_drives(void)
{
	burn_drive_enumerator_t idx;
	int initialize = 1, ret, i_bus_no = -1;
        int i_host_no = -1, i_channel_no = -1, i_target_no = -1, i_lun_no = -1;
	char buf[4096], target[4096];

	while(1) {
		ret = sg_give_next_adr_raw(&idx, buf, sizeof(buf), initialize);
		initialize = 0;
		if (ret <= 0)
	break;
		ret = sg_resolve_link(buf, target, sizeof(target), 0);
		if (ret <= 0)
			strcpy(target, buf);
		if (burn_drive_is_banned(target))
	continue; 
		sg_obtain_scsi_adr(buf, &i_bus_no, &i_host_no,
				&i_channel_no, &i_target_no, &i_lun_no);
		enumerate_common(target, buf,
				i_bus_no, i_host_no, i_channel_no,
				i_target_no, i_lun_no);
	}
	sg_give_next_adr(&idx, buf, sizeof(buf), -1);
	return 1;
}


/** Tells whether libburn has the given drive in use or exclusively reserved.
    If it is "open" then libburn will eventually call sg_release() on it when
    it is time to give up usage resp. reservation.
*/
/** Published as burn_drive.drive_is_open() */
int sg_drive_is_open(struct burn_drive * d)
{
	return (d->p_cdio != NULL);
}


/** Opens the drive for SCSI commands and - if burn activities are prone
    to external interference on your system - obtains an exclusive access lock
    on the drive. (Note: this is not physical tray locking.)
    A drive that has been opened with sg_grab() will eventually be handed
    over to sg_release() for closing and unreserving. 
*/  
int sg_grab(struct burn_drive *d)
{
	CdIo_t *p_cdio;
	char *am;

	if (d->p_cdio != NULL) {
		d->released = 0;
		return 1;
	}
	if (d->libcdio_name[0] == 0) /* just to be sure it is initialized */
		strcpy(d->libcdio_name, d->devname);
	p_cdio = cdio_open_am(d->libcdio_name, DRIVER_DEVICE, 
			burn_sg_open_o_excl ?  "MMC_RDWR_EXCL" : "MMC_RDWR");

	if (p_cdio == NULL) {
		libdax_msgs_submit(libdax_messenger, d->global_index,
			0x00020003,
			LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
			"Could not grab drive", 0/*os_errno*/, 0);
		return 0;
	}
	am = (char *) cdio_get_arg(p_cdio, "access-mode");
        if (strncmp(am, "MMC_RDWR", 8) != 0) {
		libdax_msgs_submit(libdax_messenger, d->global_index,
			0x00020003,
			LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
			"libcdio provides no MMC_RDWR access mode", 0, 0);
		cdio_destroy(p_cdio);
		return 0;
        }

	d->p_cdio = p_cdio;
	d->released = 0;
	return 1;
}


/** PORTING: Is mainly about the call to sg_close_drive() and whether it
             implements the demanded functionality.
*/
/** Gives up the drive for SCSI commands and releases eventual access locks.
    (Note: this is not physical tray locking.) 
*/
int sg_release(struct burn_drive *d)
{
	if (d->p_cdio == NULL) {
		burn_print(1, "release an ungrabbed drive.  die\n");
		return 0;
	}
	sg_close_drive(d);
	return 0;
}


/** Sends a SCSI command to the drive, receives reply and evaluates wether
    the command succeeded or shall be retried or finally failed.
    Returned SCSI errors shall not lead to a return value indicating failure.
    The callers get notified by c->error. An SCSI failure which leads not to
    a retry shall be notified via scsi_notify_error().
    The Libburn_log_sg_commandS facility might be of help when problems with
    a drive have to be examined. It shall stay disabled for normal use.
    @return: 1 success , <=0 failure
*/
int sg_issue_command(struct burn_drive *d, struct command *c)
{
	int sense_valid = 0, i, usleep_time, timeout_ms;
	time_t start_time;
        driver_return_code_t i_status;
	unsigned int dxfer_len;
        static FILE *fp = NULL;
	mmc_cdb_t cdb = {{0, }};
	cdio_mmc_direction_t e_direction;
	CdIo_t *p_cdio;
	unsigned char sense[18], *sense_pt = NULL;

	c->error = 0;
	if (d->p_cdio == NULL) {
		return 0;
	}
	p_cdio = (CdIo_t *) d->p_cdio;
        if (burn_sg_log_scsi & 1) {
                if (fp == NULL) {
                        fp= fopen("/tmp/libburn_sg_command_log", "a");
                        fprintf(fp,
                            "\n-----------------------------------------\n");
                }
        }
        if (burn_sg_log_scsi & 3)
                scsi_log_cmd(c,fp,0);

	memcpy(cdb.field, c->opcode, c->oplen);
	if (c->dir == TO_DRIVE) {
		dxfer_len = c->page->bytes;
		e_direction = SCSI_MMC_DATA_WRITE;
	} else if (c->dir == FROM_DRIVE) {
		if (c->dxfer_len >= 0)
			dxfer_len = c->dxfer_len;
		else
			dxfer_len = BUFFER_SIZE;
		e_direction = SCSI_MMC_DATA_READ;
		/* touch page so we can use valgrind */
		memset(c->page->data, 0, BUFFER_SIZE);
	} else {
		dxfer_len = 0;
		e_direction = SCSI_MMC_DATA_NONE;
	}
		
	/* retry-loop */
	start_time = time(NULL);
	timeout_ms = 200000;
	for(i = 0; ; i++) {

		i_status = mmc_run_cmd(p_cdio, timeout_ms, &cdb, e_direction,
				 	dxfer_len, c->page->data);
		sense_valid = mmc_last_cmd_sense(p_cdio, &sense_pt);
		if (sense_valid >= 18)
			memcpy(sense, sense_pt, 18);
		if (sense_pt != NULL)
			free(sense_pt);

/* Regrettably mmc_run_cmd() does not clearly distinguish between transport
   failure and SCSI error reply.
   This reaction here would be for transport failure:

		if (i_status != 0 && i_status != DRIVER_OP_ERROR) {
			libdax_msgs_submit(libdax_messenger,
				d->global_index, 0x0002010c,
				LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
				"Failed to transfer command to drive",
				errno, 0);
			sg_close_drive(d);
			d->released = 1;
			d->busy = BURN_DRIVE_IDLE;
			c->error = 1;
			return -1;
		}
*/

		if (!sense_valid) {
			memset(sense, 0, 18);
			if (i_status != 0) { /* set dummy sense */
				/*LOGICAL UNIT NOT READY,CAUSE NOT REPORTABLE*/
				sense[2] = 0x02;
				sense[12] = 0x04;
			}
		} else
			sense[2] &= 15;
	
		if (i_status != 0 || (sense[2] || sense[12] || sense[13])) {
			if (!c->retry) {
				c->error = 1;
				goto ex;
			}
			switch (scsi_error(d, sense, 18)) {
			case RETRY:
				break;
			case FAIL:
				c->error = 1;
				goto ex;
			}
			/* 
			   Calming down retries and breaking up endless cycle
			*/
			usleep_time = Libburn_sg_libcdio_retry_usleeP +
					i * Libburn_sg_libcdio_retry_incR;
			if (time(NULL) + usleep_time / 1000000 - start_time >
			    timeout_ms / 1000 + 1) {
				c->error = 1;
				goto ex;
			}
			usleep(usleep_time);
		} else
			break; /* retry-loop */
	} /* end of retry-loop */

ex:;
	if (c->error)
		scsi_notify_error(d, c, sense, 18, 0);

	if (burn_sg_log_scsi & 3) 
		/* >>> Need own duration time measurement. Then remove bit1 */
		scsi_log_err(c, fp, sense, 0, (c->error != 0) | 2);
	return 1;
}


/** Tries to obtain SCSI address parameters.
    @return  1 is success , 0 is failure
*/
int sg_obtain_scsi_adr(char *path, int *bus_no, int *host_no, int *channel_no,
                       int *target_no, int *lun_no)
{

	/* >>> any chance to get them from libcdio ? */;

	*bus_no = *host_no = *channel_no = *target_no = *lun_no = -1;
	return (0);
}


/** Tells wether a text is a persistent address as listed by the enumeration
    functions.
*/
int sg_is_enumerable_adr(char* adr)
{
	burn_drive_enumerator_t idx;
	int initialize = 1, ret;
	char buf[64];

	while(1) {
		ret = sg_give_next_adr(&idx, buf, sizeof(buf), initialize);
		initialize = 0;
		if (ret <= 0)
	break;
		if (strcmp(adr, buf) == 0) {
			sg_give_next_adr(&idx, buf, sizeof(buf), -1);
			return 1;
		}
	}
	sg_give_next_adr(&idx, buf, sizeof(buf), -1);
	return (0);
}


/** Estimate the potential payload capacity of a file address.
    @param path  The address of the file to be examined. If it does not
                 exist yet, then the directory will be inquired.
    @param bytes The pointed value gets modified, but only if an estimation is
                 possible.
    @return      -2 = cannot perform necessary operations on file object
                 -1 = neither path nor dirname of path exist
                  0 = could not estimate size capacity of file object
                  1 = estimation has been made, bytes was set
*/
int burn_os_stdio_capacity(char *path, off_t *bytes)
{
	struct stat stbuf;

#ifdef Libburn_os_has_statvfS
	struct statvfs vfsbuf;
#endif

	char testpath[4096], *cpt;
	long blocks;
	off_t add_size = 0;

	testpath[0] = 0;
	blocks = *bytes / 512;
	if (stat(path, &stbuf) == -1) {
		strcpy(testpath, path);
		cpt = strrchr(testpath, '/');
		if(cpt == NULL)
			strcpy(testpath, ".");
		else if(cpt == testpath)
			testpath[1] = 0;
		else
			*cpt = 0;
		if (stat(testpath, &stbuf) == -1)
			return -1;

#ifdef Libburn_if_this_was_linuX

	} else if(S_ISBLK(stbuf.st_mode)) {
		fd = open(path, open_mode);
		if (fd == -1)
			return -2;
		ret = ioctl(fd, BLKGETSIZE, &blocks);
		close(fd);
		if (ret == -1)
			return -2;
		*bytes = ((off_t) blocks) * (off_t) 512;

#endif /* Libburn_if_this_was_linuX */

	} else if(S_ISREG(stbuf.st_mode)) {
		add_size = stbuf.st_blocks * (off_t) 512;
		strcpy(testpath, path);
	} else
		return 0;

	if (testpath[0]) {	

#ifdef Libburn_os_has_statvfS

		if (statvfs(testpath, &vfsbuf) == -1)
			return -2;
		*bytes = add_size + ((off_t) vfsbuf.f_bsize) *
						(off_t) vfsbuf.f_bavail;

#else /* Libburn_os_has_statvfS */

		return 0;

#endif /* ! Libburn_os_has_stavtfS */

	}
	return 1;
}


/* ts A91122 : an interface to open(O_DIRECT) or similar OS tricks. */

#ifdef Libburn_read_o_direcT

	/* No special O_DIRECT-like precautions are implemented here */

#endif /* Libburn_read_o_direcT */


int burn_os_open_track_src(char *path, int open_flags, int flag)
{
	int fd;

	fd = open(path, open_flags);
	return fd;
}


void *burn_os_alloc_buffer(size_t amount, int flag)
{
	void *buf = NULL;

	buf = calloc(1, amount);
	return buf;
}


int burn_os_free_buffer(void *buffer, size_t amount, int flag)
{
	if (buffer == NULL)
		return 0;
	free(buffer);
	return 1;
}
