/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef LIBBURN_H
#define LIBBURN_H

/* Needed for  off_t  which is the (POSIX-ly) appropriate type for
   expressing a file or stream size.

   XXX we should enforce 64-bitness for off_t
*/
#include <sys/types.h>

#ifndef DOXYGEN

#if defined(__cplusplus)
#define BURN_BEGIN_DECLS \
	namespace burn { \
		extern "C" {
#define BURN_END_DECLS \
		} \
	}
#else
#define BURN_BEGIN_DECLS
#define BURN_END_DECLS
#endif

BURN_BEGIN_DECLS

#endif

/** References a physical drive in the system */
struct burn_drive;

/** References a whole disc */
struct burn_disc;

/** References a single session on a disc */
struct burn_session;

/** References a single track on a disc */
struct burn_track;

/** Session format for normal audio or data discs */
#define BURN_CDROM	0
/** Session format for obsolete CD-I discs */
#define BURN_CDI	0x10
/** Session format for CDROM-XA discs */
#define BURN_CDXA	0x20

#define BURN_POS_END 100

/** Mask for mode bits */
#define BURN_MODE_BITS 127

/** Track mode - mode 0 data
    0 bytes of user data.  it's all 0s.  mode 0.  get it?  HAH
*/
#define BURN_MODE0		(1 << 0)
/** Track mode - mode "raw" - all 2352 bytes supplied by app
    FOR DATA TRACKS ONLY!
*/
#define BURN_MODE_RAW		(1 << 1)
/** Track mode - mode 1 data
    2048 bytes user data, and all the LEC money can buy
*/
#define BURN_MODE1		(1 << 2)
/** Track mode - mode 2 data
    defaults to formless, 2336 bytes of user data, unprotected
    | with a data form if required.
*/
#define BURN_MODE2		(1 << 3)
/** Track mode modifier - Form 1, | with MODE2 for reasonable results
    2048 bytes of user data, 4 bytes of subheader
*/
#define BURN_FORM1		(1 << 4)
/** Track mode modifier - Form 2, | with MODE2 for reasonable results
    lots of user data.  not much LEC.
*/
#define BURN_FORM2		(1 << 5)
/** Track mode - audio
    2352 bytes per sector.  may be | with 4ch or preemphasis.
    NOT TO BE CONFUSED WITH BURN_MODE_RAW
    Audio data must be 44100Hz 16bit stereo with no riff or other header at
    beginning.  Extra header data will cause pops or clicks.  Audio data should
    also be in little-endian byte order.  Big-endian audio data causes static.
*/
#define BURN_AUDIO		(1 << 6)
/** Track mode modifier - 4 channel audio. */
#define BURN_4CH		(1 << 7)
/** Track mode modifier - Digital copy permitted, can be set on any track.*/
#define BURN_COPY		(1 << 8)
/** Track mode modifier - 50/15uS pre-emphasis */
#define BURN_PREEMPHASIS	(1 << 9)
/** Input mode modifier - subcodes present packed 16 */
#define BURN_SUBCODE_P16	(1 << 10)
/** Input mode modifier - subcodes present packed 96 */
#define BURN_SUBCODE_P96	(1 << 11)
/** Input mode modifier - subcodes present raw 96 */
#define BURN_SUBCODE_R96	(1 << 12)

/** Possible disc writing style/modes */
enum burn_write_types
{
	/** Packet writing.
	    currently unsupported
	*/
	BURN_WRITE_PACKET,
	/** Track At Once recording.
	    2s gaps between tracks, no fonky lead-ins
	*/
	BURN_WRITE_TAO,
	/** Session At Once.
	    block type MUST be BURN_BLOCK_SAO
	*/
	BURN_WRITE_SAO,
	/** Raw disc at once recording.
	    all subcodes must be provided by lib or user
	    only raw block types are supported
	*/
	BURN_WRITE_RAW
};

/** Data format to send to the drive */
enum burn_block_types
{
	/** sync, headers, edc/ecc provided by lib/user */
	BURN_BLOCK_RAW0 = 1,
	/** sync, headers, edc/ecc and p/q subs provided by lib/user */
	BURN_BLOCK_RAW16 = 2,
	/** sync, headers, edc/ecc and packed p-w subs provided by lib/user */
	BURN_BLOCK_RAW96P = 4,
	/** sync, headers, edc/ecc and raw p-w subs provided by lib/user */
	BURN_BLOCK_RAW96R = 8,
	/** only 2048 bytes of user data provided by lib/user */
	BURN_BLOCK_MODE1 = 256,
	/** 2336 bytes of user data provided by lib/user */
	BURN_BLOCK_MODE2R = 512,
	/** 2048 bytes of user data provided by lib/user
	    subheader provided in write parameters
	    are we ever going to support this shit?  I vote no.
	    (supposed to be supported on all drives...)
	*/
	BURN_BLOCK_MODE2_PATHETIC = 1024,
	/** 2048 bytes of data + 8 byte subheader provided by lib/user
	    hey, this is also dumb
	*/
	BURN_BLOCK_MODE2_LAME = 2048,
	/** 2324 bytes of data provided by lib/user
	    subheader provided in write parameters
	    no sir, I don't like it.
	*/
	BURN_BLOCK_MODE2_OBSCURE = 4096,
	/** 2332 bytes of data supplied by lib/user
	    8 bytes sub header provided in write parameters
	    this is the second least suck mode2, and is mandatory for
	    all drives to support.
	*/
	BURN_BLOCK_MODE2_OK = 8192,
	/** SAO block sizes are based on cue sheet, so use this. */
	BURN_BLOCK_SAO = 16384
};

/** Possible status' of the drive in regard to the disc in it. */
enum burn_disc_status
{
	/** The current status is not yet known */
	BURN_DISC_UNREADY,
	/** The drive holds a blank disc */
	BURN_DISC_BLANK,
	/** There is no disc at all in the drive */
	BURN_DISC_EMPTY,
	/** There is an incomplete disc in the drive */
	BURN_DISC_APPENDABLE,
	/** There is a disc with data on it in the drive */
	BURN_DISC_FULL
};

/** Possible types of messages form the library. */
enum burn_message_type
{
	/** Diagnostic/Process information. For the curious user. */
	BURN_MESSAGE_INFO,
	/** A warning regarding a possible problem. The user should probably
	    be notified, but its not fatal. */
	BURN_MESSAGE_WARNING,
	/** An error message. This usually means the current process will be
	    aborted, and the user should definately see these. */
	BURN_MESSAGE_ERROR
};

/** Possible information messages */
enum burn_message_info
{
	BURN_INFO_FOO
};

/** Possible warning messages */
enum burn_message_warning
{
	BURN_WARNING_FOO
};

/** Possible error messages */
enum burn_message_error
{
	BURN_ERROR_CANCELLED
};

/** Possible data source return values */
enum burn_source_status
{
	/** The source is ok */
	BURN_SOURCE_OK,
	/** The source is at end of file */
	BURN_SOURCE_EOF,
	/** The source is unusable */
	BURN_SOURCE_FAILED
};


/** Possible busy states for a drive */
enum burn_drive_status
{
	/** The drive is not in an operation */
	BURN_DRIVE_IDLE,
	/** The library is spawning the processes to handle a pending
	    operation (A read/write/etc is about to start but hasn't quite
	    yet) */
	BURN_DRIVE_SPAWNING,
	/** The drive is reading data from a disc */
	BURN_DRIVE_READING,
	/** The drive is writing data to a disc */
	BURN_DRIVE_WRITING,
	/** The drive is writing Lead-In */
	BURN_DRIVE_WRITING_LEADIN,
	/** The drive is writing Lead-Out */
	BURN_DRIVE_WRITING_LEADOUT,
	/** The drive is erasing a disc */
	BURN_DRIVE_ERASING,
	/** The drive is being grabbed */
	BURN_DRIVE_GRABBING
};

/** Information about a track on a disc - this is from the q sub channel of the
    lead-in area of a disc.  The documentation here is very terse.
    See a document such as mmc3 for proper information.
*/
struct burn_toc_entry
{
	/** Session the track is in */
	unsigned char session;
	/** Type of data.  for this struct to be valid, it must be 1 */
	unsigned char adr;
	/** Type of data in the track */
	unsigned char control;
	/** Zero.  Always.  Really. */
	unsigned char tno;
	/** Track number or special information */
	unsigned char point;
	unsigned char min;
	unsigned char sec;
	unsigned char frame;
	unsigned char zero;
	/** Track start time minutes for normal tracks */
	unsigned char pmin;
	/** Track start time seconds for normal tracks */
	unsigned char psec;
	/** Track start time frames for normal tracks */
	unsigned char pframe;
};


/** Data source for tracks */
struct burn_source {
	/** Reference count for the data source. Should be 1 when a new source
            is created.  Increment it to take a reference for yourself. Use
            burn_source_free to destroy your reference to it. */
	int refcount;

	/** Read data from the source */
	int (*read)(struct burn_source *,
	                                   unsigned char *buffer,
	                                   int size);

	/** Read subchannel data from the source (NULL if lib generated) */
	int (*read_sub)(struct burn_source *,
	                                       unsigned char *buffer,
	                                       int size);

	/** Get the size of the source's data */
	off_t (*get_size)(struct burn_source *);

	/** Clean up the source specific data */
	void (*free_data)(struct burn_source *);

	/** Next source, for when a source runs dry and padding is disabled
	    THIS IS AUTOMATICALLY HANDLED, DO NOT TOUCH
	*/
	struct burn_source *next;

	/** Source specific data */
	void *data;
};


/** Information on a drive in the system */
struct burn_drive_info
{
	/** Name of the vendor of the drive */
	char vendor[9];
	/** Name of the drive */
	char product[17];
	/** Revision of the drive */
	char revision[5];
	/** Location of the drive in the filesystem. */
	char location[17];
	/** This is currently the string which is used as persistent
	    drive address. But be warned: there is NO GUARANTEE that this
	    will stay so. Always use function  burn_drive_get_adr() to
	    inquire a persisten address.       ^^^^^^ ALWAYS ^^^^^^ */

	/** Can the drive read DVD-RAM discs */
	unsigned int read_dvdram:1;
	/** Can the drive read DVD-R discs */
	unsigned int read_dvdr:1;
	/** Can the drive read DVD-ROM discs */
	unsigned int read_dvdrom:1;
	/** Can the drive read CD-R discs */
	unsigned int read_cdr:1;
	/** Can the drive read CD-RW discs */
	unsigned int read_cdrw:1;

	/** Can the drive write DVD-RAM discs */
	unsigned int write_dvdram:1;
	/** Can the drive write DVD-R discs */
	unsigned int write_dvdr:1;
	/** Can the drive write CD-R discs */
	unsigned int write_cdr:1;
	/** Can the drive write CD-RW discs */
	unsigned int write_cdrw:1;

	/** Can the drive simulate a write */
	unsigned int write_simulate:1;

	/** Can the drive report C2 errors */
	unsigned int c2_errors:1;

	/** The size of the drive's buffer (in kilobytes) */
	int buffer_size;
	/** 
	 * The supported block types in tao mode.
	 * They should be tested with the desired block type.
	 * See also burn_block_types.
	 */
	int tao_block_types;
	/** 
	 * The supported block types in sao mode.
	 * They should be tested with the desired block type.
	 * See also burn_block_types.
	 */
	int sao_block_types;
	/** 
	 * The supported block types in raw mode.
	 * They should be tested with the desired block type.
	 * See also burn_block_types.
	 */
	int raw_block_types;
	/** 
	 * The supported block types in packet mode.
	 * They should be tested with the desired block type.
	 * See also burn_block_types.
	 */
	int packet_block_types;

	/** The value by which this drive can be indexed when using functions
	    in the library. This is the value to pass to all libbburn functions
	    that operate on a drive. */
	struct burn_drive *drive;
};

/** Messages from the library */
struct burn_message
{
	/** The drive associated with the message. NULL if the error is not
	    related to a specific drive. */
	struct burn_drive *drive;

	/** The type of message this is. See message_type for details. */
	enum burn_message_type type;

	/** The actual message */
	union detail {
		struct {
			enum burn_message_info    message;
		} info;
		struct {
			enum burn_message_warning message;
		} warning;
		struct {
			enum burn_message_error   message;
		} error;
	} detail;
};

/** Operation progress report. All values are 0 based indices. 
 * */
struct burn_progress {
	/** The total number of sessions */
	int sessions;
	/** Current session.*/
	int session;
	/** The total number of tracks */
	int tracks;
	/** Current track. */
	int track;
	/** The total number of indices */
	int indices;
	/** Curent index. */
	int index;
	/** The starting logical block address */
	int start_sector;
	/** The number of sector */
	int sectors;
	/** The current sector being processed */
	int sector;
};

/** Initialize the library.
    This must be called before using any other functions in the library. It
    may be called more than once with no effect.
    It is possible to 'restart' the library by shutting it down and
    re-initializing it. This is necessary if you follow the older and
    more general way of accessing a drive via burn_drive_scan() and
    burn_drive_grab(). See burn_drive_scan_and_grab() with its strong
    urges and its explanations.
    @return Nonzero if the library was able to initialize; zero if
            initialization failed.
*/
int burn_initialize(void);

/** Shutdown the library.
    This should be called before exiting your application. Make sure that all
    drives you have grabbed are released <i>before</i> calling this.
*/
void burn_finish(void);

/** Set the verbosity level of the library. The default value is 0, which means
    that nothing is output on stderr. The more you increase this, the more
    debug output should be displayed on stderr for you.
    @param level The verbosity level desired. 0 for nothing, higher positive
                 values for more information output.
*/
void burn_set_verbosity(int level);

/* ts A60813 */
/** Set parameters for behavior on opening device files. To be called early
    after burn_initialize() and before any bus scan. But not mandatory at all.
    Parameter value 1 enables a feature, 0 disables.  
    Default is (1,0,0). Have a good reason before you change it.
    @param exclusive Try to open only devices which are not marked as busy
                     and try to mark them busy if opened sucessfully. (O_EXCL)
                     There are kernels which simply don't care about O_EXCL.
                     Some have it off, some have it on, some are switchable.
    @param blocking  Try to wait for drives which do not open immediately but
                     also do not return an error as well. (O_NONBLOCK)
                     This might stall indefinitely with /dev/hdX hard disks.
    @param abort_on_busy  Unconditionally abort process when a non blocking
                          exclusive opening attempt indicates a busy drive.
                          Use this only after thorough tests with your app.
*/
void burn_preset_device_open(int exclusive, int blocking, int abort_on_busy);

/** Returns a newly allocated burn_message structure. This message should be
    freed with burn_message_free() when you are finished with it.
    @return A message or NULL when there are no more messages to retrieve.
*/
struct burn_message* burn_get_message(void);

/** Frees a burn_message structure */
void burn_message_free(struct burn_message *msg);


/* ts A60823 */
/** Aquire a drive with known persistent address.This is the sysadmin friendly
    way to open one drive and to leave all others untouched. It bundles
    the following API calls to form a non-obtrusive way to use libburn:
      burn_drive_add_whitelist() , burn_drive_scan() , burn_drive_grab()
    You are *strongly urged* to use this call whenever you know the drive
    address in advance.
    If not, then you have to use directly above calls. In that case, you are
    *strongly urged* to drop any unintended drive which will be exclusively
    occupied and not closed by burn_drive_scan().
    This can be done by shutting down the library including a call to
    burn_finish(). You may later start a new libburn session and should then
    use the function described here with an address obtained after
    burn_drive_scan() via burn_drive_get_adr(&(drive_infos[driveno]), adr) .
    Another way is to drop the unwanted drives by burn_drive_info_forget().
    @param drive_infos On success returns a one element array with the drive
                  (cdrom/burner). Thus use with driveno 0 only. On failure
                  the array has no valid elements at all.
                  The returned array should be freed via burn_drive_info_free()
                  when it is no longer needed, and before calling a scan
                  function again.
                  This is a result from call burn_drive_scan(). See there.
                  Use with driveno 0 only.
    @param adr    The persistent address of the desired drive. Either obtained
                  by burn_drive_get_adr() or guessed skillfully by application
                  resp. its user.
    @param load   Nonzero to make the drive attempt to load a disc (close its
                  tray door, etc).
    @return       1 = success , 0 = drive not found , -1 = other error
*/    
int burn_drive_scan_and_grab(struct burn_drive_info *drive_infos[],
                             char* adr, int load);


/* ts A51221 */
/** Maximum number of particularly permissible drive addresses */
#define BURN_DRIVE_WHITELIST_LEN 255
/** Add a device to the list of permissible drives. As soon as some entry is in
    the whitelist all non-listed drives are banned from scanning.
    @return 1 success, <=0 failure
*/
int burn_drive_add_whitelist(char *device_address);

/** Remove all drives from whitelist. This enables all possible drives. */
void burn_drive_clear_whitelist(void);


/** Scan for drives. This function MUST be called until it returns nonzero.
    No drives can be in use when this is called or it will assert.
    All drive pointers are invalidated by using this function. Do NOT store
    drive pointers across calls to this function or death AND pain will ensue.
    After this call all drives depicted by the returned array are subject
    to eventual (O_EXCL) locking. See burn_preset_device_open(). This state
    ends either with burn_drive_info_forget() or with burn_drive_release().
    It is unfriendly to other processes on the system to hold drives locked
    which one does not definitely plan to use soon.
    @param drive_infos Returns an array of drive info items (cdroms/burners).
                  The returned array must be freed by burn_drive_info_free()
                  before burn_finish(), and also before calling this function
                  burn_drive_scan() again.
    @param n_drives Returns the number of drive items in drive_infos.
    @return Zero while scanning is not complete; non-zero when it is finished.
*/
int burn_drive_scan(struct burn_drive_info *drive_infos[],
		    unsigned int *n_drives);

/* ts A60904 : ticket 62, contribution by elmom */
/** Release memory about a single drive and any exclusive lock on it.
    Become unable to inquire or grab it. Expect FATAL consequences if you try.
    @param drive_info pointer to a single element out of the array
                      obtained from burn_drive_scan() : &(drive_infos[driveno])
    @param force controls degree of permissible drive usage at the moment this
                 function is called, and the amount of automatically provided
                 drive shutdown : 
                  0= drive must be ungrabbed and BURN_DRIVE_IDLE
                  1= try to release drive resp. accept BURN_DRIVE_GRABBING 
                 Use these two only. Further values are to be defined.
    @return 1 on success, 2 if drive was already forgotten,
            0 if not permissible, <0 on other failures, 
*/
int burn_drive_info_forget(struct burn_drive_info *drive_info, int force);


/** Free a burn_drive_info array returned by burn_drive_scan
*/
void burn_drive_info_free(struct burn_drive_info drive_infos[]);


/* ts A60823 */
/** Maximum length+1 to expect with a persistent drive address string */
#define BURN_DRIVE_ADR_LEN 1024

/** Inquire the persistent address of the given drive.
    @param drive The drive to inquire. Usually some  &(drive_infos[driveno])
    @param adr   An application provided array of at least BURN_DRIVE_ADR_LEN
                 characters size. The persistent address gets copied to it.
*/
int burn_drive_get_adr(struct burn_drive_info *drive_info, char adr[]);


/** Grab a drive. This must be done before the drive can be used (for reading,
    writing, etc).
    @param drive The drive to grab. This is found in a returned
                 burn_drive_info struct.
    @param load Nonzero to make the drive attempt to load a disc (close its
                tray door, etc).
    @return 1 if it was possible to grab the drive, else 0
*/
int burn_drive_grab(struct burn_drive *drive, int load);


/** Release a drive. This should not be done until the drive is no longer
    busy (see burn_drive_get_status). The drive is (O_EXCL) unlocked
    afterwards.
	@param drive The drive to release.
	@param eject Nonzero to make the drive eject the disc in it.
*/
void burn_drive_release(struct burn_drive *drive, int eject);


/** Returns what kind of disc a drive is holding. This function may need to be
    called more than once to get a proper status from it. See burn_status
    for details.
    @param drive The drive to query for a disc.
    @return The status of the drive, or what kind of disc is in it.
*/
enum burn_disc_status burn_disc_get_status(struct burn_drive *drive);

/** Tells whether a disc can be erased or not
    @return Non-zero means erasable
*/
int burn_disc_erasable(struct burn_drive *d);

/** Returns the progress and status of a drive.
    @param drive The drive to query busy state for.
    @param p Returns the progress of the operation, NULL if you don't care
    @return the current status of the drive. See also burn_drive_status.
*/
enum burn_drive_status burn_drive_get_status(struct burn_drive *drive,
					     struct burn_progress *p);

/** Creates a write_opts struct for burning to the specified drive
    must be freed with burn_write_opts_free
    @param drive The drive to write with
    @return The write_opts
*/
struct burn_write_opts *burn_write_opts_new(struct burn_drive *drive);

/** Frees a write_opts struct created with burn_write_opts_new
    @param opts write_opts to free
*/
void burn_write_opts_free(struct burn_write_opts *opts);

/** Creates a write_opts struct for reading from the specified drive
    must be freed with burn_write_opts_free
    @param drive The drive to read from
    @return The read_opts
*/
struct burn_read_opts *burn_read_opts_new(struct burn_drive *drive);

/** Frees a read_opts struct created with burn_read_opts_new
    @param opts write_opts to free
*/
void burn_read_opts_free(struct burn_read_opts *opts);

/** Erase a disc in the drive. The drive must be grabbed successfully BEFORE
    calling this functions. Always ensure that the drive reports a status of
    BURN_DISC_FULL before calling this function. An erase operation is not
    cancellable, as control of the operation is passed wholly to the drive and
    there is no way to interrupt it safely.
    @param drive The drive with which to erase a disc.
    @param fast Nonzero to do a fast erase, where only the disc's headers are
                erased; zero to erase the entire disc.
*/
void burn_disc_erase(struct burn_drive *drive, int fast);

/** Read a disc from the drive and write it to an fd pair. The drive must be
    grabbed successfully BEFORE calling this function. Always ensure that the
    drive reports a status of BURN_DISC_FULL before calling this function.
    @param drive The drive from which to read a disc.
    @param o The options for the read operation.
*/
void burn_disc_read(struct burn_drive *drive, const struct burn_read_opts *o);

/** Write a disc in the drive. The drive must be grabbed successfully BEFORE
    calling this function. Always ensure that the drive reports a status of
    BURN_DISC_BLANK or BURN_STATUS_FULL (to append a new session to the
    disc) before calling this function.
    @param o The options for the writing operation.
    @param disc The struct burn_disc * that described the disc to be created
*/
void burn_disc_write(struct burn_write_opts *o, struct burn_disc *disc);

/** Cancel an operation on a drive.
    This will only work when the drive's busy state is BURN_DRIVE_READING or
    BURN_DRIVE_WRITING.
    @param drive The drive on which to cancel the current operation.
*/
void burn_drive_cancel(struct burn_drive *drive);

/** Convert a minute-second-frame (MSF) value to sector count
    @param m Minute component
    @param s Second component
    @param f Frame component
    @return The sector count
*/
int burn_msf_to_sectors(int m, int s, int f);

/** Convert a sector count to minute-second-frame (MSF)
    @param sectors The sector count
    @param m Returns the minute component
    @param s Returns the second component
    @param f Returns the frame component
*/
void burn_sectors_to_msf(int sectors, int *m, int *s, int *f);

/** Convert a minute-second-frame (MSF) value to an lba
    @param m Minute component
    @param s Second component
    @param f Frame component
    @return The lba
*/
int burn_msf_to_lba(int m, int s, int f);

/** Convert an lba to minute-second-frame (MSF)
    @param lba The lba
    @param m Returns the minute component
    @param s Returns the second component
    @param f Returns the frame component
*/
void burn_lba_to_msf(int lba, int *m, int *s, int *f);

/** Create a new disc (for DAO recording)*/
struct burn_disc *burn_disc_create(void);

/** Delete disc and decrease the reference count on all its sessions
	@param d The disc to be freed
*/
void burn_disc_free(struct burn_disc *d);

/** Create a new session (For SAO at once recording, or to be added to a 
    disc for DAO)
*/
struct burn_session *burn_session_create(void);

/** Free a session (and decrease reference count on all tracks inside)
	@param s Session to be freed
*/
void burn_session_free(struct burn_session *s);

/** Add a session to a disc at a specific position, increasing the 
    sessions's reference count.
	@param d Disc to add the session to
	@param s Session to add to the disc
	@param pos position to add at (BURN_POS_END is "at the end")
	@return 0 for failure, 1 for success
*/
int burn_disc_add_session(struct burn_disc *d, struct burn_session *s,
			  unsigned int pos);

/** Remove a session from a disc
	@param d Disc to remove session from
	@param s Session pointer to find and remove
*/
int burn_disc_remove_session(struct burn_disc *d, struct burn_session *s);


/** Create a track (for TAO recording, or to put in a session) */
struct burn_track *burn_track_create(void);

/** Free a track
	@param t Track to free
*/
void burn_track_free(struct burn_track *t);

/** Add a track to a session at specified position
	@param s Session to add to
	@param t Track to insert in session
	@param pos position to add at (BURN_POS_END is "at the end")
	@return 0 for failure, 1 for success
*/
int burn_session_add_track(struct burn_session *s, struct burn_track *t,
			   unsigned int pos);

/** Remove a track from a session
	@param s Session to remove track from
	@param t Track pointer to find and remove
	@return 0 for failure, 1 for success
*/
int burn_session_remove_track(struct burn_session *s, struct burn_track *t);


/** Define the data in a track
	@param t the track to define
	@param offset The lib will write this many 0s before start of data
	@param tail The number of extra 0s to write after data
	@param pad 1 means the lib should pad the last sector with 0s if the
	       track isn't exactly sector sized.  (otherwise the lib will
	       begin reading from the next track)
	@param mode data format (bitfield)
*/
void burn_track_define_data(struct burn_track *t, int offset, int tail,
			    int pad, int mode);

/** Set the ISRC details for a track
	@param t The track to change
	@param country the 2 char country code. Each character must be
	       only numbers or letters.
	@param owner 3 char owner code. Each character must be only numbers
	       or letters.
	@param year 2 digit year. A number in 0-99 (Yep, not Y2K friendly).
	@param serial 5 digit serial number. A number in 0-99999.
*/
void burn_track_set_isrc(struct burn_track *t, char *country, char *owner,
			 unsigned char year, unsigned int serial);

/** Disable ISRC parameters for a track
	@param t The track to change
*/
void burn_track_clear_isrc(struct burn_track *t);

/** Hide the first track in the "pre gap" of the disc
	@param s session to change
	@param onoff 1 to enable hiding, 0 to disable
*/
void burn_session_hide_first_track(struct burn_session *s, int onoff);

/** Get the drive's disc struct - free when done
	@param d drive to query
	@return the disc struct
*/
struct burn_disc *burn_drive_get_disc(struct burn_drive *d);

/** Set the track's data source
	@param t The track to set the data source for
	@param s The data source to use for the contents of the track
	@return An error code stating if the source is ready for use for
	        writing the track, or if an error occured
    
*/
enum burn_source_status burn_track_set_source(struct burn_track *t,
					      struct burn_source *s);

/** Free a burn_source (decrease its refcount and maybe free it)
	@param s Source to free
*/
void burn_source_free(struct burn_source *s);

/** Creates a data source for an image file (and maybe subcode file) */
struct burn_source *burn_file_source_new(const char *path,
					 const char *subpath);

/** Creates a data source for an image file (a track) from an open
    readable filedescriptor, an eventually open readable subcodes file
    descriptor and eventually a fixed size in bytes.
    @param datafd The source of data.
    @param subfd The eventual source for subcodes. Not used if -1.
    @param size The eventual fixed size of eventually both fds. 
                If this value is 0, the size will be determined from datafd.
*/
struct burn_source *burn_fd_source_new(int datafd, int subfd, off_t size);

/** Tells how long a track will be on disc */
int burn_track_get_sectors(struct burn_track *);


/** Sets drive read and write speed
    @param d The drive to set speed for
    @param read Read speed in k/s (0 is max)
    @param write Write speed in k/s (0 is max)
*/
void burn_drive_set_speed(struct burn_drive *d, int read, int write);

/* these are for my debugging, they will disappear */
void burn_structure_print_disc(struct burn_disc *d);
void burn_structure_print_session(struct burn_session *s);
void burn_structure_print_track(struct burn_track *t);

/** Sets the write type for the write_opts struct
    @param opts The write opts to change
    @param write_type The write type to use
    @param block_type The block type to use
    @return Returns 1 on success and 0 on failure.
*/
int burn_write_opts_set_write_type(struct burn_write_opts *opts,
				   enum burn_write_types write_type,
				   int block_type);

/** Supplies toc entries for writing - not normally required for cd mastering
    @param opts The write opts to change
    @param count The number of entries
    @param toc_entries
*/
void burn_write_opts_set_toc_entries(struct burn_write_opts *opts,
				     int count,
				     struct burn_toc_entry *toc_entries);

/** Sets the session format for a disc
    @param opts The write opts to change
    @param format The session format to set
*/
void burn_write_opts_set_format(struct burn_write_opts *opts, int format);

/** Sets the simulate value for the write_opts struct
    @param opts The write opts to change
    @param sim If non-zero, the drive will perform a simulation instead of a burn
    @return Returns 1 on success and 0 on failure.
*/
int  burn_write_opts_set_simulate(struct burn_write_opts *opts, int sim);

/** Controls buffer underrun prevention
    @param opts The write opts to change
    @param underrun_proof if non-zero, buffer underrun protection is enabled
    @return Returns 1 on success and 0 on failure.
*/
int burn_write_opts_set_underrun_proof(struct burn_write_opts *opts,
				       int underrun_proof);

/** Sets whether to use opc or not with the write_opts struct
    @param opts The write opts to change
    @param opc If non-zero, optical power calibration will be performed at
               start of burn
	 
*/
void burn_write_opts_set_perform_opc(struct burn_write_opts *opts, int opc);

void burn_write_opts_set_has_mediacatalog(struct burn_write_opts *opts, int has_mediacatalog);

void burn_write_opts_set_mediacatalog(struct burn_write_opts *opts, unsigned char mediacatalog[13]);

/** Sets whether to read in raw mode or not
    @param opts The read opts to change
    @param raw_mode If non-zero, reading will be done in raw mode, so that everything in the data tracks on the
            disc is read, including headers.
*/
void burn_read_opts_set_raw(struct burn_read_opts *opts, int raw_mode);

/** Sets whether to report c2 errors or not 
    @param opts The read opts to change
    @param c2errors If non-zero, report c2 errors.
*/
void burn_read_opts_set_c2errors(struct burn_read_opts *opts, int c2errors);

/** Sets whether to read subcodes from audio tracks or not
    @param opts The read opts to change
    @param subcodes_audio If non-zero, read subcodes from audio tracks on the disc.
*/
void burn_read_opts_read_subcodes_audio(struct burn_read_opts *opts,
					int subcodes_audio);

/** Sets whether to read subcodes from data tracks or not 
    @param opts The read opts to change
    @param subcodes_data If non-zero, read subcodes from data tracks on the disc.
*/
void burn_read_opts_read_subcodes_data(struct burn_read_opts *opts,
				       int subcodes_data);

/** Sets whether to recover errors if possible
    @param opts The read opts to change
    @param hardware_error_recovery If non-zero, attempt to recover errors if possible.
*/
void burn_read_opts_set_hardware_error_recovery(struct burn_read_opts *opts,
						int hardware_error_recovery);

/** Sets whether to report recovered errors or not
    @param opts The read opts to change
    @param report_recovered_errors If non-zero, recovered errors will be reported.
*/
void burn_read_opts_report_recovered_errors(struct burn_read_opts *opts,
					    int report_recovered_errors);

/** Sets whether blocks with unrecoverable errors should be read or not
    @param opts The read opts to change
    @param transfer_damaged_blocks If non-zero, blocks with unrecoverable errors will still be read.
*/
void burn_read_opts_transfer_damaged_blocks(struct burn_read_opts *opts,
					    int transfer_damaged_blocks);

/** Sets the number of retries to attempt when trying to correct an error
    @param opts The read opts to change
    @param hardware_error_retries The number of retries to attempt when correcting an error.
*/
void burn_read_opts_set_hardware_error_retries(struct burn_read_opts *opts,
					       unsigned char hardware_error_retries);

/** Gets the maximum write speed for a drive
    @param d Drive to query
    @return Maximum write speed in K/s
*/
int burn_drive_get_write_speed(struct burn_drive *d);

/** Gets the maximum read speed for a drive
    @param d Drive to query
    @return Maximum read speed in K/s
*/
int burn_drive_get_read_speed(struct burn_drive *d);

/** Gets a copy of the toc_entry structure associated with a track
    @param t Track to get the entry from
    @param entry Struct for the library to fill out
*/
void burn_track_get_entry(struct burn_track *t, struct burn_toc_entry *entry);

/** Gets a copy of the toc_entry structure associated with a session's lead out
    @param s Session to get the entry from
    @param entry Struct for the library to fill out
*/
void burn_session_get_leadout_entry(struct burn_session *s,
                                    struct burn_toc_entry *entry);

/** Gets an array of all the sessions for the disc
    THIS IS NO LONGER VALID AFTER YOU ADD OR REMOVE A SESSION
    @param d Disc to get session array for
    @param num Returns the number of sessions in the array
    @return array of sessions
*/
struct burn_session **burn_disc_get_sessions(struct burn_disc *d,
                                             int *num);

int burn_disc_get_sectors(struct burn_disc *d);

/** Gets an array of all the tracks for a session
    THIS IS NO LONGER VALID AFTER YOU ADD OR REMOVE A TRACK
    @param s session to get track array for
    @param num Returns the number of tracks in the array
    @return array of tracks
*/
struct burn_track **burn_session_get_tracks(struct burn_session *s,
                                            int *num);

int burn_session_get_sectors(struct burn_session *s);

/** Gets the mode of a track
    @param track the track to query
    @return the track's mode
*/
int burn_track_get_mode(struct burn_track *track);

/** Returns whether the first track of a session is hidden in the pregap
    @param session the session to query
    @return non-zero means the first track is hidden
*/
int burn_session_get_hidefirst(struct burn_session *session);

/** Returns the library's version in its parts
    @param major The major version number
    @param minor The minor version number
    @param micro The micro version number
*/
void burn_version(int *major, int *minor, int *micro);

#ifndef DOXYGEN

BURN_END_DECLS

#endif

#endif /*LIBBURN_H*/
