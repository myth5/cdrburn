
/* libdax_msgs
   Message handling facility of libburn and libisofs.
   Copyright (C) 2006-2011 Thomas Schmitt <scdbackup@gmx.net>,
   provided under GPL version 2 or later.
*/


/*
  *Never* set this macro outside libdax_msgs.c !
  The entrails of the message handling facility are not to be seen by
  the other library components or the applications.
*/
#ifdef LIBDAX_MSGS_H_INTERNAL


#ifndef LIBDAX_MSGS_SINGLE_THREADED
#include <pthread.h>
#endif


struct libdax_msgs_item {

 double timestamp;
 pid_t process_id;
 int origin;

 int severity;
 int priority;

 /* Apply for your developer's error code range at
      libburn-hackers@pykix.org
    Report introduced codes in the list below. */
 int error_code;

 char *msg_text;
 int os_errno;
  
 struct libdax_msgs_item *prev,*next;

};


struct libdax_msgs {

 int refcount;

 struct libdax_msgs_item *oldest;
 struct libdax_msgs_item *youngest;
 int count;

 int queue_severity;
 int print_severity;
 char print_id[81];
 
#ifndef LIBDAX_MSGS_SINGLE_THREADED
 pthread_mutex_t lock_mutex;
#endif


};

#endif /* LIBDAX_MSGS_H_INTERNAL */


#ifndef LIBDAX_MSGS_H_INCLUDED
#define LIBDAX_MSGS_H_INCLUDED 1


#ifndef LIBDAX_MSGS_H_INTERNAL


                          /* Architectural aspects */
/* 
  libdax_msgs is designed to serve in libraries which want to offer their
  applications a way to control the output of library messages. It shall be
  incorporated by an owner, i.e. a software entity which encloses the code
  of the .c file.

  Owner of libdax_msgs is libburn. A fully compatible variant named libiso_msgs
  is owned by libisofs and can get generated by a script of the libburn
  project: libburn/libdax_msgs_to_xyz_msgs.sh .

  Reason: One cannot link two owners of the same variant together because
  both would offer the same functions to the linker. For that situation one
  has to create a compatible variant as it is done for libisofs.

  Compatible variants may get plugged together by call combinations like
    burn_set_messenger(iso_get_messenger());
  A new variant would demand a _set_messenger() function if it has to work
  with libisofs. If only libburn is planned as link partner then a simple
  _get_messenger() does suffice.
  Take care to shutdown libburn before its provider of the *_msgs object
  gets shut down.

*/

                          /* Public Opaque Handles */

/** A pointer to this is a opaque handle to a message handling facility */
struct libdax_msgs;

/** A pointer to this is a opaque handle to a single message item */
struct libdax_msgs_item;

#endif /* ! LIBDAX_MSGS_H_INTERNAL */


                            /* Public Macros */


/* Registered Severities */

/* It is well advisable to let applications select severities via strings and
   forwarded functions libdax_msgs__text_to_sev(), libdax_msgs__sev_to_text().
   These macros are for use by the owner of libdax_msgs.
*/

/** Use this to get messages of any severity. Do not use for submitting.
*/
#define LIBDAX_MSGS_SEV_ALL                                          0x00000000


/** Messages of this severity shall transport plain disk file paths
    whenever an event of severity SORRY or above is related with an
    individual disk file.
    No message text shall be added to the file path. The ERRFILE message
    shall be issued before the human readable message which carries the
    true event severity. That message should contain the file path so it
    can be found by strstr(message, path)!=NULL.
    The error code shall be the same as with the human readable message.
*/ 
#define LIBDAX_MSGS_SEV_ERRFILE                                      0x08000000


/** Debugging messages not to be visible to normal users by default
*/
#define LIBDAX_MSGS_SEV_DEBUG                                        0x10000000

/** Update of a progress report about long running actions
*/
#define LIBDAX_MSGS_SEV_UPDATE                                       0x20000000

/** Not so usual events which were gracefully handled
*/
#define LIBDAX_MSGS_SEV_NOTE                                         0x30000000

/** Possibilities to achieve a better result
*/
#define LIBDAX_MSGS_SEV_HINT                                         0x40000000

/** Warnings about problems which could not be handled optimally
*/
#define LIBDAX_MSGS_SEV_WARNING                                      0x50000000


/** Non-fatal error messages indicating that parts of an action failed but
    processing may go on if one accepts deviations from the desired result.

    SORRY may also be the severity for incidents which are severe enough
    for FAILURE but happen within already started irrevocable actions,
    like ISO image generation. A precondition for such a severity ease is
    that the action can be continued after the incident.
    See below MISHAP for what xorriso would need instead of this kind of SORRY
    and generates for itself in case of libisofs image generation.

    E.g.: A pattern yields no result.
          A speed setting cannot be made.
          A libisofs input file is inaccessible during image generation.

    After SORRY a function should try to go on if that makes any sense
    and if no threshold prescribes abort on SORRY. The function should
    nevertheless indicate some failure in its return value. 
    It should - but it does not have to.
*/
#define LIBDAX_MSGS_SEV_SORRY                                        0x60000000


/** A FAILURE (see below) which can be tolerated during long lasting
    operations just because they cannot simply be stopped or revoked.

    xorriso converts libisofs SORRY messages issued during image generation
    into MISHAP messages in order to allow its evaluators to distinguish
    image generation problems from minor image composition problems.
    E.g.:
      A libisofs input file is inaccessible during image generation.

    After a MISHAP a function should behave like after SORRY.
*/
#define LIBDAX_MSGS_SEV_MISHAP                                       0x64000000


/** Non-fatal error indicating that an important part of an action failed and
    that only a new setup of preconditions will give hope for sufficient
    success.

    E.g.: No media is inserted in the output drive.
          No write mode can be found for inserted media.
          A libisofs input file is inaccessible during grafting.

    After FAILURE a function should end with a return value indicating failure.
    It is at the discretion of the function whether it ends immediately in any
    case or whether it tries to go on if the eventual threshold allows.
*/
#define LIBDAX_MSGS_SEV_FAILURE                                      0x68000000


/** An error message which puts the whole operation of the program in question

    E.g.: Not enough memory for essential temporary objects.
          Irregular errors from resources.
          Programming errors (soft assert).

    After FATAL a function should end very soon with a return value
    indicating severe failure.
*/
#define LIBDAX_MSGS_SEV_FATAL                                        0x70000000


/** A message from an abort handler which will finally finish libburn
*/
#define LIBDAX_MSGS_SEV_ABORT                                        0x71000000

/** A severity to exclude resp. discard any possible message.
    Do not use this severity for submitting.
*/
#define LIBDAX_MSGS_SEV_NEVER                                        0x7fffffff


/* Registered Priorities */

/* Priorities are to be selected by the programmers and not by the user. */

#define LIBDAX_MSGS_PRIO_ZERO                                        0x00000000 
#define LIBDAX_MSGS_PRIO_LOW                                         0x10000000 
#define LIBDAX_MSGS_PRIO_MEDIUM                                      0x20000000
#define LIBDAX_MSGS_PRIO_HIGH                                        0x30000000
#define LIBDAX_MSGS_PRIO_TOP                                         0x7ffffffe

/* Do not use this priority for submitting */
#define LIBDAX_MSGS_PRIO_NEVER                                       0x7fffffff


/* Origin numbers of libburn drives may range from 0 to 1048575 */
#define LIBDAX_MSGS_ORIGIN_DRIVE_BASE          0
#define LIBDAX_MSGS_ORIGIN_DRIVE_TOP     0xfffff

/* Origin numbers of libisofs images may range from 1048575 to 2097152 */
#define LIBDAX_MSGS_ORIGIN_IMAGE_BASE   0x100000
#define LIBDAX_MSGS_ORIGIN_IMAGE_TOP    0x1fffff



                            /* Public Functions */

       /* Calls initiated from inside the direct owner (e.g. from libburn) */


/** Create new empty message handling facility with queue and issue a first
    official reference to it.
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return >0 success, <=0 failure
*/
int libdax_msgs_new(struct libdax_msgs **m, int flag);


/** Destroy a message handling facility and all its eventual messages.
    The submitted pointer gets set to NULL.
    Actually only the last destroy call of all offical references to the object
    will really dispose it. All others just decrement the reference counter.
    Call this function only with official reference pointers obtained by
    libdax_msgs_new() or libdax_msgs_refer(), and only once per such pointer.
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 for success, 0 for pointer to NULL, -1 for fatal error
*/
int libdax_msgs_destroy(struct libdax_msgs **m, int flag);


/** Create an official reference to an existing libdax_msgs object. The
    references keep the object alive at least until it is released by
    a matching number of destroy calls. So each reference MUST be revoked
    by exactly one call to libdax_msgs_destroy().
    @param pt The pointer to be set and registered
    @param m  A pointer to the existing object
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 for success, 0 for failure
*/
int libdax_msgs_refer(struct libdax_msgs **pt, struct libdax_msgs *o, int flag);


/** Submit a message to a message handling facility.
    @param origin  program specific identification number of the originator of
                   a message. E.g. drive number. Programs should have an own
                   range of origin numbers. See above LIBDAX_MSGS_ORIGIN_*_BASE
                   Use -1 if no number is known.
    @param error_code  Unique error code. Use only registered codes. See below.
                   The same unique error_code may be issued at different
                   occasions but those should be equivalent out of the view
                   of a libdax_msgs application. (E.g. "cannot open ATA drive"
                   versus "cannot open SCSI drive" would be equivalent.)
    @param severity The LIBDAX_MSGS_SEV_* of the event.
    @param priority The LIBDAX_MSGS_PRIO_* number of the event.
    @param msg_text Printable and human readable message text.
    @param os_errno Eventual error code from operating system (0 if none)
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 on success, 0 on rejection, <0 for severe errors
*/
int libdax_msgs_submit(struct libdax_msgs *m, int origin, int error_code,
                       int severity, int priority, char *msg_text, 
                       int os_errno, int flag);



     /* Calls from applications (to be forwarded by direct owner) */


/** Convert a registered severity number into a severity name
    @param flag Bitfield for control purposes:
      bit0= list all severity names in a newline separated string
    @return >0 success, <=0 failure
*/
int libdax_msgs__sev_to_text(int severity, char **severity_name,
                             int flag);


/** Convert a severity name into a severity number,
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return >0 success, <=0 failure
*/
int libdax_msgs__text_to_sev(char *severity_name, int *severity,
                             int flag);


/** Set minimum severity for messages to be queued (default
    LIBDAX_MSGS_SEV_ALL) and for messages to be printed directly to stderr
    (default LIBDAX_MSGS_SEV_NEVER).
    @param print_id A text of at most 80 characters to be printed before
                    any eventually printed message (default is "libdax: ").
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return always 1 for now
*/
int libdax_msgs_set_severities(struct libdax_msgs *m, int queue_severity,
                               int print_severity, char *print_id, int flag);


/** Obtain a message item that has at least the given severity and priority.
    Usually all older messages of lower severity are discarded then. If no
    item of sufficient severity was found, all others are discarded from the
    queue.
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 if a matching item was found, 0 if not, <0 for severe errors
*/
int libdax_msgs_obtain(struct libdax_msgs *m, struct libdax_msgs_item **item,
                       int severity, int priority, int flag);


/** Destroy a message item obtained by libdax_msgs_obtain(). The submitted
    pointer gets set to NULL.
    Caution: Copy eventually obtained msg_text before destroying the item,
             if you want to use it further.
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 for success, 0 for pointer to NULL, <0 for severe errors
*/
int libdax_msgs_destroy_item(struct libdax_msgs *m,
                             struct libdax_msgs_item **item, int flag);


/** Obtain from a message item the three application oriented components as
    submitted with the originating call of libdax_msgs_submit().
    Caution: msg_text becomes a pointer into item, not a copy.
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 on success, 0 on invalid item, <0 for servere errors
*/
int libdax_msgs_item_get_msg(struct libdax_msgs_item *item, 
                             int *error_code, char **msg_text, int *os_errno,
                             int flag);


/** Obtain from a message item the submitter identification submitted
    with the originating call of libdax_msgs_submit().
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 on success, 0 on invalid item, <0 for servere errors
*/
int libdax_msgs_item_get_origin(struct libdax_msgs_item *item, 
                            double *timestamp, pid_t *process_id, int *origin,
                            int flag);


/** Obtain from a message item severity and priority as submitted
    with the originating call of libdax_msgs_submit().
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 on success, 0 on invalid item, <0 for servere errors
*/
int libdax_msgs_item_get_rank(struct libdax_msgs_item *item, 
                              int *severity, int *priority, int flag);


#ifdef LIBDAX_MSGS_________________


                      /* Registered Error Codes */


Format: error_code  (LIBDAX_MSGS_SEV_*,LIBDAX_MSGS_PRIO_*) = explanation
If no severity or priority are fixely associated, use "(,)".

------------------------------------------------------------------------------
Range "libdax_msgs"        :  0x00000000 to 0x0000ffff

 0x00000000 (ALL,ZERO)     = Initial setting in new libdax_msgs_item
 0x00000001 (DEBUG,ZERO)   = Test error message
 0x00000002 (DEBUG,ZERO)   = Debugging message
 0x00000003 (FATAL,HIGH)   = Out of virtual memory


------------------------------------------------------------------------------
Range "elmom"              :  0x00010000 to 0x0001ffff



------------------------------------------------------------------------------
Range "scdbackup"          :  0x00020000 to 0x0002ffff

 Acessing and defending drives:

 0x00020001 (SORRY,LOW)    = Cannot open busy device
 0x00020002 (SORRY,HIGH)   = Encountered error when closing drive
 0x00020003 (SORRY,HIGH)   = Could not grab drive
 0x00020004 (NOTE,HIGH)    = Opened O_EXCL scsi sibling
 0x00020005 (SORRY,HIGH)   = Failed to open device
 0x00020006 (FATAL,HIGH)   = Too many scsi siblings
 0x00020007 (NOTE,HIGH)    = Closed O_EXCL scsi siblings
 0x00020008 (SORRY,HIGH)   = Device busy. Failed to fcntl-lock
 0x00020009 (SORRY,HIGH)   = Neither stdio-path nor its directory exist
 0x0002000a (FAILURE,HIGH) = Cannot accept '...' as SG_IO CDROM drive
 0x0002000b (FAILURE,HIGH) = File object '...' not found
 0x0002000c (FAILURE,HIGH) = Cannot start device file enumeration
 0x0002000d (FAILURE,HIGH) = Cannot enumerate next device
           
 General library operations:

 0x00020101 (WARNING,HIGH) = Cannot find given worker item
 0x00020102 (SORRY,HIGH)   = A drive operation is still going on
 0x00020103 (WARNING,HIGH) = After scan a drive operation is still going on
 0x00020104 (SORRY,HIGH)   = NULL pointer caught
 0x00020105 (SORRY,HIGH)   = Drive is already released
 0x00020106 (SORRY,HIGH)   = Drive is busy on attempt to close
 0x00020107 (WARNING,HIGH) = A drive is still busy on shutdown of library
 0x00020108 (SORRY,HIGH)   = Drive is not grabbed on disc status inquiry
 0x00020108 (FATAL,HIGH)   = Could not allocate new drive object
 0x00020109 (FATAL,HIGH)   = Library not running
 0x0002010a (FATAL,HIGH)   = Unsuitable track mode
 0x0002010b (FATAL,HIGH)   = Burn run failed
 0x0002010c (FATAL,HIGH)   = Failed to transfer command to drive
 0x0002010d (DEBUG,HIGH)   = Could not inquire TOC
 0x0002010e (FATAL,HIGH)   = Attempt to read ATIP from ungrabbed drive
 0x0002010f (DEBUG,HIGH)   = SCSI error condition on command
 0x00020110 (FATAL,HIGH)   = Persistent drive address too long
 0x00020111 (FATAL,HIGH)   = Could not allocate new auxiliary object
 0x00020112 (SORRY,HIGH)   = Bad combination of write_type and block_type
 0x00020113 (FATAL,HIGH)   = Drive capabilities not inquired yet
 0x00020114 (SORRY,HIGH)   = Attempt to set ISRC with bad data
 0x00020115 (SORRY,HIGH)   = Attempt to set track mode to unusable value
 0x00020116 (FATAL,HIGH)   = Track mode has unusable value
 0x00020117 (FATAL,HIGH)   = toc_entry of drive is already in use
 0x00020118 (DEBUG,HIGH)   = Closing track
 0x00020119 (DEBUG,HIGH)   = Closing session
 0x0002011a (NOTE,HIGH)    = Padding up track to minimum size
 0x0002011b (FATAL,HIGH)   = Attempt to read track info from ungrabbed drive
 0x0002011c (FATAL,HIGH)   = Attempt to read track info from busy drive
 0x0002011d (FATAL,HIGH)   = SCSI error on write
 0x0002011e (SORRY,HIGH)   = Unsuitable media detected
 0x0002011f (SORRY,HIGH)   = Burning is restricted to a single track
 0x00020120 (NOTE,HIGH)    = FORMAT UNIT ignored
 0x00020121 (FATAL,HIGH)   = Write preparation setup failed
 0x00020122 (FATAL,HIGH)   = SCSI error on format_unit
 0x00020123 (SORRY,HIGH)   = DVD Media are unsuitable for desired track type
 0x00020124 (SORRY,HIGH)   = SCSI error on set_streaming
 0x00020125 (SORRY,HIGH)   = Write start address not supported
 0x00020126 (SORRY,HIGH)   = Write start address not properly aligned
 0x00020127 (NOTE,HIGH)    = Write start address is ...
 0x00020128 (FATAL,HIGH)   = Unsupported inquiry_type with mmc_get_performance
 0x00020129 (SORRY,HIGH)   = Will not format media type
 0x0002012a (FATAL,HIGH)   = Cannot inquire write mode capabilities
 0x0002012b (FATAL,HIGH)   = Drive offers no suitable write mode with this job
 0x0002012c (SORRY,HIGH)   = Too many logical tracks recorded
 0x0002012d (FATAL,HIGH)   = Exceeding range of permissible write addresses
 0x0002012e (NOTE,HIGH)    = Activated track default size
 0x0002012f (SORRY,HIGH)   = SAO is restricted to single fixed size session
 0x00020130 (SORRY,HIGH)   = Drive and media state unsuitable for blanking
 0x00020131 (SORRY,HIGH)   = No suitable formatting type offered by drive
 0x00020132 (SORRY,HIGH)   = Selected format is not suitable for libburn
 0x00020133 (SORRY,HIGH)   = Cannot mix data and audio in SAO mode
 0x00020134 (NOTE,HIGH)    = Defaulted TAO to DAO
 0x00020135 (SORRY,HIGH)   = Cannot perform TAO, job unsuitable for DAO
 0x00020136 (SORRY,HIGH)   = DAO burning restricted to single fixed size track
 0x00020137 (HINT,HIGH)    = TAO would be possible
 0x00020138 (FATAL,HIGH)   = Cannot reserve track
 0x00020139 (SORRY,HIGH)   = Write job parameters are unsuitable
 0x0002013a (FATAL,HIGH)   = No suitable media detected
 0x0002013b (DEBUG,HIGH)   = SCSI command indicates host or driver error
 0x0002013c (SORRY,HIGH)   = Malformed capabilities page 2Ah received
 0x0002013d (DEBUG,LOW)    = Waiting for free buffer space takes long time
 0x0002013e (SORRY,HIGH)   = Timeout with waiting for free buffer. Now disabled
 0x0002013f (DEBUG,LOW)    = Reporting total time spent with waiting for buffer
 0x00020140 (FATAL,HIGH)   = Drive is busy on attempt to write random access
 0x00020141 (SORRY,HIGH)   = Write data count not properly aligned
 0x00020142 (FATAL,HIGH)   = Drive is not grabbed on random access write
 0x00020143 (SORRY,HIGH)   = Read start address not properly aligned
 0x00020144 (SORRY,HIGH)   = SCSI error on read
 0x00020145 (FATAL,HIGH)   = Drive is busy on attempt to read data
 0x00020146 (FATAL,HIGH)   = Drive is a virtual placeholder
 0x00020147 (SORRY,HIGH)   = Cannot address start byte
 0x00020148 (SORRY,HIGH)   = Cannot write desired amount of data
 0x00020149 (SORRY,HIGH)   = Unsuitable filetype for pseudo-drive
 0x0002014a (SORRY,HIGH)   = Cannot read desired amount of data
 0x0002014b (SORRY,HIGH)   = Drive is already registered resp. scanned
 0x0002014c (FATAL,HIGH)   = Emulated drive caught in SCSI function
 0x0002014d (SORRY,HIGH)   = Asynchromous SCSI error
 0x0002014f (SORRY,HIGH)   = Timeout with asynchromous SCSI command
 0x00020150 (DEBUG,LOW)    = Reporting asynchronous waiting time
 0x00020151 (FAILURE,HIGH) = Read attempt on write-only drive
 0x00020152 (FATAL,HIGH)   = Cannot start fifo thread
 0x00020153 (SORRY,HIGH)   = Read error on fifo input
 0x00020154 (NOTE,HIGH)    = Forwarded input error ends output
 0x00020155 (SORRY,HIGH)   = Desired fifo buffer too large
 0x00020156 (SORRY,HIGH)   = Desired fifo buffer too small
 0x00020157 (FATAL,HIGH)   = burn_source is not a fifo object
 0x00020158 (DEBUG,LOW)    = Reporting thread disposal precautions
 0x00020159 (DEBUG,HIGH)   = TOC Format 0 returns inconsistent data
 0x0002015a (NOTE,HIGH)    = Could not examine busy device
 0x0002015b (HINT,HIGH)    = Busy '...' seems to be a hard disk, as '...1' exists
 0x0002015c (FAILURE,HIGH) = Fifo size too small for desired peek buffer
 0x0002015d (FAILURE,HIGH) = Fifo input ended short of desired peek buffer size
 0x0002015e (FATAL,HIGH)   = Fifo is already under consumption when peeking
 0x0002015f (MISHAP,HIGH)  = Damaged CD table-of-content detected and truncated
 0x00020160 (WARNING,HIGH) = Session without leadout encountered
 0x00020161 (WARNING,HIGH) = Empty session deleted
 0x00020162 (SORRY,HIGH)   = BD-R not unformatted blank any more. Cannot format
 0x00020163 (NOTE,HIGH)    = Blank BD-R left unformatted for zero spare capacity
 0x00020164 (SORRY,HIGH)   = Drive does not format BD-RE without spares
 0x00020165 (WARNING,HIGH) = Drive does not support fast formatting
 0x00020166 (WARNING,HIGH) = Drive does not support full formatting
 0x00020167 (SORRY,HIGH)   = Drive does not support non-default formatting
 0x00020168 (FAILURE,HIGH) = Media not properly formatted. Cannot write.
 0x00020169 (WARNING,HIGH) = Last session on media is still open
 0x0002016a (FAILURE,HIGH) = No MMC transport adapter is present
 0x0002016b (WARNING,HIGH) = No MMC transport adapter is present
 0x0002016c (DEBUG,HIGH)   = No MMC transport adapter is present
 0x0002016e (DEBUG,HIGH)   = MODE SENSE page 2A too short
 0x0002016f (DEBUG,HIGH)   = Unable to grab scanned drive
 0x00020170 (NOTE,HIGH)    = Closing open session before writing new one
 0x00020171 (NOTE,HIGH)    = Closing BD-R with accidently open session
 0x00020172 (SORRY,HIGH)   = Read start address larger than number of readable blocks
 0x00020173 (FAILURE,HIGH) = Drive tells NWA smaller than last written address
 0x00020174 (SORRY,HIGH)   = Fifo alignment does not allow desired read size
 0x00020175 (FATAL,HIGH)   = Supporting library is too old
 0x00020176 (NOTE,HIGH)    = Stream recording disabled because of small OS buffer
 0x00020177 (ABORT,HIGH)   = Urged drive worker threads to do emergency halt
 0x00020178 (DEBUG,HIGH)   = Write thread ended
 0x00020179 (FAILURE,HIGH) = Offset source start address is before end of previous source
 0x0002017a (FAILURE,HIGH) = Expected offset source object as parameter
 0x0002017b (WARNING,HIGH) = Sequential BD-R media likely to soon fail writing
 0x0002017c (FAILURE,HIGH) = No valid write type selected
 0x0002017d (FATAL,HIGH)   = Invalid file descriptor with stdio pseudo-drive
 0x0002017e (FAILURE,HIGH) = Failed to close track, session, or disc
 0x0002017f (FAILURE,HIGH) = Failed to synchronize drive cache
 0x00020180 (FAILURE,HIGH) = Premature end of input encountered
 0x00020181 (FAILURE,HIGH) = Pseudo-drive is a read-only file. Cannot write.
 0x00020182 (FAILURE,HIGH) = Cannot truncate disk file for pseudo blanking
 0x00020183 (WARNING,HIGH) = Failed to open device (a pseudo-drive) for reading
 0x00020184 (WARNING,HIGH) = No Next-Writable-Address
 0x00020185 (WARNING,HIGH) = Track damaged, not closed and not writable
 0x00020186 (WARNING,HIGH) = Track damaged and not closed
 0x00020187 (NOTE,HIGH)    = Track not marked as damaged. No action taken.
 0x00020188 (FAILURE,HIGH) = Cannot close damaged track on given media type
 0x00020189 (FATAL,HIGH)   = Drive is already grabbed by libburn

 libdax_audioxtr:
 0x00020200 (SORRY,HIGH)   = Cannot open audio source file
 0x00020201 (SORRY,HIGH)   = Audio source file has unsuitable format
 0x00020202 (SORRY,HIGH)   = Failed to prepare reading of audio data



------------------------------------------------------------------------------
Range "vreixo"              :  0x00030000 to 0x0003ffff

 0x0003ffff (FAILURE,HIGH) = Operation canceled
 0x0003fffe (FATAL,HIGH)   = Unknown or unexpected fatal error
 0x0003fffd (FAILURE,HIGH) = Unknown or unexpected error
 0x0003fffc (FATAL,HIGH)   = Internal programming error
 0x0003fffb (FAILURE,HIGH) = NULL pointer where NULL not allowed
 0x0003fffa (FATAL,HIGH)   = Memory allocation error
 0x0003fff9 (FATAL,HIGH)   = Interrupted by a signal
 0x0003fff8 (FAILURE,HIGH) = Invalid parameter value
 0x0003fff7 (FATAL,HIGH)   = Cannot create a needed thread
 0x0003fff6 (FAILURE,HIGH) = Write error
 0x0003fff5 (FAILURE,HIGH) = Buffer read error
 0x0003ffc0 (FAILURE,HIGH) = Trying to add a node already added to another dir
 0x0003ffbf (FAILURE,HIGH) = Node with same name already exist
 0x0003ffbe (FAILURE,HIGH) = Trying to remove a node that was not added to dir
 0x0003ffbd (FAILURE,HIGH) = A requested node does not exist
 0x0003ffbc (FAILURE,HIGH) = Image already bootable
 0x0003ffbb (FAILURE,HIGH) = Trying to use an invalid file as boot image
 0x0003ff80 (FAILURE,HIGH) = Error on file operation
 0x0003ff7f (FAILURE,HIGH) = Trying to open an already openned file
 0x0003ff7e (FAILURE,HIGH) = Access to file is not allowed
 0x0003ff7d (FAILURE,HIGH) = Incorrect path to file
 0x0003ff7c (FAILURE,HIGH) = The file does not exist in the filesystem
 0x0003ff7b (FAILURE,HIGH) = Trying to read or close a file not openned
 0x0003ff7a (FAILURE,HIGH) = Directory used where no dir is expected
 0x0003ff79 (FAILURE,HIGH) = File read error
 0x0003ff78 (FAILURE,HIGH) = Not dir used where a dir is expected
 0x0003ff77 (FAILURE,HIGH) = Not symlink used where a symlink is expected
 0x0003ff76 (FAILURE,HIGH) = Cannot seek to specified location
 0x0003ff75 (HINT,MEDIUM)  = File not supported in ECMA-119 tree and ignored
 0x0003ff74 (HINT,MEDIUM)  = File bigger than supported by used standard
 0x0003ff73 (MISHAP,HIGH)  = File read error during image creation
 0x0003ff72 (HINT,MEDIUM)  = Cannot convert filename to requested charset
 0x0003ff71 (SORRY,HIGH)   = File cannot be added to the tree
 0x0003ff70 (HINT,MEDIUM)  = File path breaks specification constraints
 0x0003ff00 (FAILURE,HIGH) = Charset conversion error
 0x0003feff (FAILURE,HIGH) = Too much files to mangle
 0x0003fec0 (FAILURE,HIGH) = Wrong or damaged Primary Volume Descriptor
 0x0003febf (SORRY,HIGH)   = Wrong or damaged RR entry
 0x0003febe (SORRY,HIGH)   = Unsupported RR feature
 0x0003febd (FAILURE,HIGH) = Wrong or damaged ECMA-119
 0x0003febc (FAILURE,HIGH) = Unsupported ECMA-119 feature
 0x0003febb (SORRY,HIGH)   = Wrong or damaged El-Torito catalog
 0x0003feba (SORRY,HIGH)   = Unsupported El-Torito feature
 0x0003feb9 (SORRY,HIGH)   = Cannot patch isolinux boot image
 0x0003feb8 (SORRY,HIGH)   = Unsupported SUSP feature
 0x0003feb7 (WARNING,HIGH) = Error on a RR entry that can be ignored
 0x0003feb6 (HINT,MEDIUM)  = Error on a RR entry that can be ignored
 0x0003feb5 (WARNING,HIGH) = Multiple ER SUSP entries found
 0x0003feb4 (HINT,MEDIUM)  = Unsupported volume descriptor found
 0x0003feb3 (WARNING,HIGH) = El-Torito related warning
 0x0003feb2 (MISHAP,HIGH)  = Image write cancelled
 0x0003feb1 (WARNING,HIGH) = El-Torito image is hidden

Outdated codes which may not be re-used for other purposes than
re-instating them, if ever:

X 0x00031001 (SORRY,HIGH)    = Cannot read file (ignored)
X 0x00031002 (FATAL,HIGH)    = Cannot read file (operation canceled)
X 0x00031000 (FATAL,HIGH)    = Unsupported ISO-9660 image
X 0x00031001 (HINT,MEDIUM)   = Unsupported Vol Desc that will be ignored
X 0x00031002 (FATAL,HIGH)    = Damaged ISO-9660 image
X 0x00031003 (SORRY,HIGH)    = Cannot read previous image file
X 0x00030101 (HINT,MEDIUM)   = Unsupported SUSP entry that will be ignored
X 0x00030102 (SORRY,HIGH)    = Wrong/damaged SUSP entry
X 0x00030103 (WARNING,MEDIUM)= Multiple SUSP ER entries where found
X 0x00030111 (SORRY,HIGH)    = Unsupported RR feature
X 0x00030112 (SORRY,HIGH)    = Error in a Rock Ridge entry
X 0x00030201 (HINT,MEDIUM)   = Unsupported Boot Vol Desc that will be ignored
X 0x00030202 (SORRY,HIGH)    = Wrong El-Torito catalog
X 0x00030203 (HINT,MEDIUM)   = Unsupported El-Torito feature
X 0x00030204 (SORRY,HIGH)    = Invalid file to be an El-Torito image
X 0x00030205 (WARNING,MEDIUM)= Cannot properly patch isolinux image
X 0x00030206 (WARNING,MEDIUM)= Copying El-Torito from a previous image without
X                              enought info about it
X 0x00030301 (NOTE,MEDIUM)   = Unsupported file type for Joliet tree


------------------------------------------------------------------------------
Range "application"         :  0x00040000 to 0x0004ffff

 0x00040000 (ABORT,HIGH)    : Application supplied message
 0x00040001 (FATAL,HIGH)    : Application supplied message
 0x00040002 (SORRY,HIGH)    : Application supplied message
 0x00040003 (WARNING,HIGH)  : Application supplied message
 0x00040004 (HINT,HIGH)     : Application supplied message
 0x00040005 (NOTE,HIGH)     : Application supplied message
 0x00040006 (UPDATE,HIGH)   : Application supplied message
 0x00040007 (DEBUG,HIGH)    : Application supplied message
 0x00040008 (*,HIGH)        : Application supplied message


------------------------------------------------------------------------------
Range "libisofs-xorriso"    :  0x00050000 to 0x0005ffff

This is an alternative representation of libisofs.so.6 error codes in xorriso.
If values returned by iso_error_get_code() do not fit into 0x30000 to 0x3ffff
then they get truncated to 16 bit and mapped into this range.
(This should never need to happen, of course.)

------------------------------------------------------------------------------
Range "libisoburn"          :  0x00060000 to 0x00006ffff

 0x00060000 (*,*)           : Message which shall be attributed to libisoburn

 >>> the messages of libisoburn need to be registered individually


------------------------------------------------------------------------------

#endif /* LIBDAX_MSGS_________________ */



#ifdef LIBDAX_MSGS_H_INTERNAL

                             /* Internal Functions */


/** Lock before doing side effect operations on m */
static int libdax_msgs_lock(struct libdax_msgs *m, int flag);

/** Unlock after effect operations on m are done */
static int libdax_msgs_unlock(struct libdax_msgs *m, int flag);


/** Create new empty message item.
    @param link Previous item in queue
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return >0 success, <=0 failure
*/
static int libdax_msgs_item_new(struct libdax_msgs_item **item, 
                                struct libdax_msgs_item *link, int flag);

/** Destroy a message item obtained by libdax_msgs_obtain(). The submitted
    pointer gets set to NULL.
    @param flag Bitfield for control purposes (unused yet, submit 0)
    @return 1 for success, 0 for pointer to NULL
*/
static int libdax_msgs_item_destroy(struct libdax_msgs_item **item, int flag);


#endif /* LIBDAX_MSGS_H_INTERNAL */


#endif /* ! LIBDAX_MSGS_H_INCLUDED */
