
/*  test/libburner.c , API illustration of burning a single data track to CD */
/*  Copyright (C) 2005 - 2006 Thomas Schmitt <scdbackup@gmx.net> */
/*  Provided under GPL, see also "License and copyright aspects" at file end */


/** IMPORTANT: By default this program tries to make a simulated burn
	       on the CD recorder. Some obey, some do not.
	       If you want to burn really readable CD for sure by default,
	       then set this macro to 0 .
	       Explicit options: --burn_for_real  and  --try_to_simulate
*/
#define Libburner_try_to_simulatE 1

/**                               Overview 
  
  libburner is a minimal demo application for the library libburn as provided
  on  http://libburn.pykix.org . It can list the available devices, can
  blank a CD-RW and can burn to CD-R or CD-RW.
  It's main purpose, nevertheless, is to show you how to use libburn and also
  to serve the libburn team as reference application. libburner.c does indeed
  define the standard way how above three gestures can be implemented and
  stay upward compatible for a good while.
  
  Before you can do anything, you have to initialize libburn by
     burn_initialize()
  as it is done in main() at the end of this file. Then you aquire a
  drive in an appropriate way conforming to the API. The two main
  approaches are shown here in application functions:
     libburner_aquire_by_adr()     demonstrates usage as of cdrecord traditions
     libburner_aquire_by_driveno()      demonstrates a scan-and-choose approach
  With that aquired drive you blank a CD-RW
     libburner_blank_disc()
  Between blanking and burning one eventually has to reload the drive status
     libburner_regrab()
  With the aquired drive you can burn to CD-R or blank CD-RW
     libburner_payload()
  When everything is done, main() releases the drive and shuts down libburn:
     burn_drive_release();
     burn_finish()
  
*/

/* We shall prepare for times when more than 2 GB of data are to be handled.
   This gives POSIX-ly 64 bit off_t */
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 1
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

/** See this for the decisive API specs . libburn.h is The Original */
#include <libburn/libburn.h>

/* libburn is intended for Linux systems with kernel 2.4 or 2.6 for now */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>


/** For simplicity i use global variables to represent the drives.
    Drives are systemwide global, so we do not give away much of good style.
*/

/** This list will hold the drives known to libburn. This might be all CD
    drives of the system and thus might impose severe impact on the system.
*/
static struct burn_drive_info *drive_list;

/** If you start a long lasting operation with drive_count > 1 then you are
    not friendly to the users of other drives on those systems. Beware. */
static unsigned int drive_count;

/** This variable indicates wether the drive is grabbed and must be
    finally released */
static int drive_is_grabbed = 0;

/** Wether to burn for real or to *try* to simulate a burn */
static int simulate_burn = Libburner_try_to_simulatE ;


/* Some in-advance definitions to allow a more comprehensive ordering
   of the functions and their explanations in here */
int libburner_aquire_by_adr(char *drive_adr);
int libburner_aquire_by_driveno(int *drive_no);


/* ------------------------------- API gestures ---------------------------- */

/** You need to aquire a drive before burning. The API offers this as one
    compact call and alternatively as application controllable gestures of
    whitelisting, scanning for drives and finally grabbing one of them.

    If you have a persistent address of the drive, then the compact call is
    to prefer. It avoids a shutdown-init cycle of libburn and thus is more
    safe against race conditions between competing users of that drive.
    On modern Linux kernels, race conditions are supposed to end up by
    having one single winner or by having all losers. On modern Linux
    kernels, there should be no fatal disturbance of ongoing burns
    of other libburn instances. We use open(O_EXCL) by default.
    There are variants of cdrecord which participate in advisory O_EXCL
    locking of block devices. Others possibly don't. Some kernels do
    nevertheless impose locking on open drives anyway (e.g. SuSE 9.0, 2.4.21).
*/
int libburner_aquire_drive(char *drive_adr, int *driveno)
{
	int ret;

	if(drive_adr != NULL && drive_adr[0] != 0)
		ret = libburner_aquire_by_adr(drive_adr);
	else
		ret = libburner_aquire_by_driveno(driveno);
	return ret;
}


/** If the persistent drive address is known, then this approach is much
    more un-obtrusive to the systemwide livestock of drives. Only the
    given drive device will be opened during this procedure.
*/
int libburner_aquire_by_adr(char *drive_adr)
{
	int ret;
	
	printf("Aquiring drive '%s' ...\n",drive_adr);
	ret = burn_drive_scan_and_grab(&drive_list,drive_adr,1);
	if (ret <= 0) {
		fprintf(stderr,"FAILURE with persistent drive address  '%s'\n",
			drive_adr);
		if (strncmp(drive_adr,"/dev/sg",7) != 0 &&
		    strncmp(drive_adr,"/dev/hd",7) != 0)
			fprintf(stderr,"\nHINT: Consider addresses like  '/dev/hdc'  or  '/dev/sg0'\n");
	} else {
		printf("done\n");
		drive_is_grabbed = 1;
	}
	return ret;
}


/** This method demonstrates how to use libburn without knowing a persistent
    drive address in advance. It has to make sure that after assessing the
    list of available drives, all drives get closed again. Only then it is
    sysadmin-acceptable to aquire the desired drive for a prolonged time.
    This drive closing is enforced here by shutting down libburn and
    restarting it again with the much more un-obtrusive approach to use
    a persistent address and thus to only touch the one desired drive.
    @param driveno the index number in libburn's drive list. This will get
                   set to 0 on success and will then be the drive index to
                   use in the further dourse of processing.
    @return 1 success , <= 0 failure
*/
int libburner_aquire_by_driveno(int *driveno)
{
	char adr[BURN_DRIVE_ADR_LEN];
	int ret, i;

	printf("Beginning to scan for devices ...\n");
	while (!burn_drive_scan(&drive_list, &drive_count)) ;
	if (drive_count <= 0) {
		printf("FAILED (no drives found)\n");
		return 0;
	}
	printf("done\n");

	/* Interactive programs may choose the drive number at this moment.

	   drive[0] to drive[drive_count-1] are struct burn_drive_info
	   as defined in  libburn/libburn.h  . This structure is part of API
	   and thus will strive for future compatibility on source level.
	   Have a look at the info offered.
	   Caution: do not take .location for drive address. Always use
		    burn_drive_get_adr() or you might become incompatible
		    in future.
	   Note: bugs with struct burn_drive_info - if any - will not be
		 easy to fix. Please report them but also strive for
		 workarounds on application level.
	*/
	printf("\nOverview of accessible drives (%d found) :\n",
		drive_count);
	printf("-----------------------------------------------------------------------------\n");
	for (i = 0; i < drive_count; i++) {
		if (burn_drive_get_adr(&(drive_list[i]), adr) <=0)
			strcpy(adr, "-get_adr_failed-");
		printf("%d  --drive '%s'  :  '%s'  '%s'\n",
			i,adr,drive_list[i].vendor,drive_list[i].product);
	}
	printf("-----------------------------------------------------------------------------\n\n");

	if (*driveno < 0) {
		printf("Pseudo-drive \"-\" given : bus scanning done.\n");
		return 2; /* only return 1 will cause a burn */
	}

	/* We already made our choice via command line. (default is 0) */
	if (drive_count <= *driveno || *driveno < 0) {
		fprintf(stderr,
			"Found only %d drives. Number %d not available.\n",
			drive_count, *driveno);
		return 0;
	}


	/* Now save yourself from sysadmins' revenge */

	/* If drive_count == 1 this would be not really necessary, though.
	   You could now call burn_drive_grab() and avoid libburn restart.
	   We don't try to be smart here and follow the API's strong urge. */

	if (burn_drive_get_adr(&(drive_list[*driveno]), adr) <=0) {
		fprintf(stderr,
		"Cannot inquire persistent drive address of drive number %d\n",
			*driveno);
		return 0;
	}
	printf("Detected '%s' as persistent address of drive number %d\n",
		adr,*driveno);

/* In cdrskin this causes a delayed sigsegv. I understand we risk only
   a small memory leak by not doing:

	burn_drive_info_free(drive_list);
*/
	burn_finish();
	printf(
	     "Re-Initializing library to release any unintended drives ...\n");
	if (burn_initialize())
		printf("done\n");
	else {
		printf("FAILED\n");
		fprintf(stderr,"\nFailed to re-initialize libburn.\n");
		return 0;
	}
	ret = libburner_aquire_by_adr(adr);
	if (ret > 0)
		*driveno = 0;
	return ret;
}


/** Makes a previously used CD-RW ready for thorough re-usal.

    To our knowledge it is hardly possible to abort an ongoing blank operation
    because after start it is entirely handled by the drive.
    So expect a blank run to survive the end of the blanking process and be
    patient for the usual timespan of a normal blank run. Only after that
    time has surely elapsed, only then you should start any rescue attempts
    with the drive. If necessary at all.
*/
int libburner_blank_disc(struct burn_drive *drive, int blank_fast)
{
	enum burn_disc_status disc_state;
	struct burn_progress progress;

	while (burn_drive_get_status(drive, NULL))
		usleep(1001);

	while ((disc_state = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
		usleep(1001);
	printf(
	    "Drive media status:  %d  (see  libburn/libburn.h  BURN_DISC_*)\n",
	    disc_state);
	if (disc_state == BURN_DISC_BLANK) {
		fprintf(stderr,
		  "IDLE: Blank CD media detected. Will leave it untouched\n");
		return 2;
	} else if (disc_state == BURN_DISC_FULL ||
		   disc_state == BURN_DISC_APPENDABLE) {
		; /* this is what libburn is willing to blank */
	} else if (disc_state == BURN_DISC_EMPTY) {
		fprintf(stderr,"FATAL: No media detected in drive\n");
		return 0;
	} else {
		fprintf(stderr,
			"FATAL: Cannot recognize drive and media state\n");
		return 0;
	}
	if(!burn_disc_erasable(drive)) {
		fprintf(stderr,
			"FATAL : Media is not of erasable type\n");
		return 0;
	}
	printf(
	      "Beginning to %s-blank CD media.\n", (blank_fast?"fast":"full"));
	printf(
	      "Expect some garbage sector numbers and some zeros at first.\n");
	burn_disc_erase(drive, blank_fast);
	while (burn_drive_get_status(drive, &progress)) {
		printf("Blanking sector  %d\n", progress.sector);
		sleep(1);
	}
	printf("Done\n");
	return 1;
}


/** This gesture is necessary to get the drive info after blanking */
int libburner_regrab(struct burn_drive *drive) {
	int ret;

	printf("Releasing and regrabbing drive ...\n");
	if (drive_is_grabbed)
		burn_drive_release(drive, 0);
	drive_is_grabbed = 0;
	ret = burn_drive_grab(drive, 0);
	if (ret != 0) {
		drive_is_grabbed = 1;
	 	printf("done\n");
	} else
	 	printf("FAILED\n");
	return !!ret;
}


/** Brings the preformatted image (ISO 9660, afio, ext2, whatever) onto media.

    To make sure your image is fully readable on any Linux machine, this
    function adds 300 kB of padding to the track.

    Without a signal handler it is quite dangerous to abort the process
    while this function is active. See cdrskin/cdrskin.c and its usage
    of cdrskin/cleanup.[ch] for an example of application provided
    abort handling. It must cope with 2 of 3 threads reporting for
    being handled.

    Without signal handler have ready a command line 
       cdrecord dev=... -reset
    with a dev= previously inquired by cdrecord [dev=ATA] -scanbus 
    in order to get your drive out of shock state after raw abort.
    Thanks to Joerg Schilling for helping out unquestioned. :) 

    In general, libburn is less prone to system problems than cdrecord,
    i believe. But cdrecord had long years of time to complete itself.
    We are still practicing. Help us with that. :))
*/
int libburner_payload(struct burn_drive *drive, const char *source_adr,
		     off_t size)
{
	struct burn_source *data_src;
	struct burn_disc *target_disc;
	struct burn_session *session;
	struct burn_write_opts *burn_options;
	enum burn_disc_status disc_state;
	struct burn_track *track;
	struct burn_progress progress;
	time_t start_time;
	int last_sector = 0;

	target_disc = burn_disc_create();
	session = burn_session_create();
	burn_disc_add_session(target_disc, session, BURN_POS_END);
	track = burn_track_create();

	/* a padding of 300 kB is helpful to avoid the read-ahead bug */
	burn_track_define_data(track, 0, 300*1024, 1, BURN_MODE1);

	if (source_adr[0] == '-' && source_adr[1] == 0) {
		data_src = burn_fd_source_new(0, -1, size);
		printf("Note: using standard input as source with %.f bytes\n",
			(double) size);
	} else
		data_src = burn_file_source_new(source_adr, NULL);
	if (data_src == NULL) {
		fprintf(stderr,
		       "FATAL: Could not open data source '%s'.\n",source_adr);
		if(errno!=0)
			fprintf(stderr,"(Most recent system error: %s )\n",
				strerror(errno));
		return 0;
	}

	if (burn_track_set_source(track, data_src) != BURN_SOURCE_OK) {
		printf("FATAL: Cannot attach source object to track object\n");
		return 0;
	}
	burn_session_add_track(session, track, BURN_POS_END);
	burn_source_free(data_src);

	while (burn_drive_get_status(drive, NULL))
		usleep(1001);

	/* Evaluate drive and media */
	while ((disc_state = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
		usleep(1001);
	if (disc_state != BURN_DISC_BLANK) {
		if (disc_state == BURN_DISC_FULL ||
		    disc_state == BURN_DISC_APPENDABLE) {
			fprintf(stderr,
      		       "FATAL: Media with data detected. Need blank media.\n");
			if (burn_disc_erasable(drive))
				fprintf(stderr, "HINT: Try --blank_fast\n\n");
		} else if (disc_state == BURN_DISC_EMPTY) 
			fprintf(stderr,"FATAL: No media detected in drive\n");
		else
			fprintf(stderr,
			    "FATAL: Cannot recognize drive and media state\n");
		return 0;
	}

	burn_options = burn_write_opts_new(drive);
	burn_write_opts_set_perform_opc(burn_options, 0);

#ifdef Libburner_raw_mode_which_i_do_not_likE
	/* This yields higher CD capacity but hampers my IDE controller
	   with burning on one drive and reading on another simultaneously.
	   My burner does not obey the order --try_to_simulate in this mode.
        */
	burn_write_opts_set_write_type(burn_options,
				       BURN_WRITE_RAW, BURN_BLOCK_RAW96R);
#else

	/* This is by what cdrskin competes with cdrecord -sao which
	   i understand is the mode preferrably advised by Joerg Schilling */
	burn_write_opts_set_write_type(burn_options,
				       BURN_WRITE_SAO, BURN_BLOCK_SAO);

#endif
	if(simulate_burn)
		printf("\n*** Will TRY to SIMULATE burning ***\n\n");
	burn_write_opts_set_simulate(burn_options, simulate_burn);
	burn_structure_print_disc(target_disc);
	burn_drive_set_speed(drive, 0, 0);
	burn_write_opts_set_underrun_proof(burn_options, 1);

	printf("Burning starts. With e.g. 4x media expect up to a minute of zero progress.\n");
	start_time = time(0);
	burn_disc_write(burn_options, target_disc);

	burn_write_opts_free(burn_options);
	while (burn_drive_get_status(drive, NULL) == BURN_DRIVE_SPAWNING) ;
	while (burn_drive_get_status(drive, &progress)) {
		if( progress.sectors <= 0 || progress.sector == last_sector)
			printf(
			     "Thank you for being patient since %d seconds.\n",
			     (int) (time(0) - start_time));
		else
			printf("Burning sector %d of %d\n",
				progress.sector, progress.sectors);
		last_sector = progress.sector;
		sleep(1);
	}
	printf("\n");
	burn_track_free(track);
	burn_session_free(session);
	burn_disc_free(target_disc);
	if(simulate_burn)
		printf("\n*** Did TRY to SIMULATE burning ***\n\n");
	return 0;
}


/** Converts command line arguments into a few program parameters.
    drive_adr[] must provide at least BURN_DRIVE_ADR_LEN bytes.
    source_adr[] must provide at least 4096 bytes.
*/
int libburner_setup(int argc, char **argv, char drive_adr[], int *driveno,
                   int *do_blank, char source_adr[], off_t *size)
{
    int i, insuffient_parameters = 0;
    int print_help = 0;

    drive_adr[0] = 0;
    *driveno = 0;
    *do_blank = 0;
    source_adr[0] = 0;
    *size = 650*1024*1024;

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--blank_fast")) {
            *do_blank = 1;

        } else if (!strcmp(argv[i], "--blank_full")) {
            *do_blank = 2;

        } else if (!strcmp(argv[i], "--burn_for_real")) {
            simulate_burn = 0;

        } else if (!strcmp(argv[i], "--drive")) {
            ++i;
            if (i >= argc) {
                fprintf(stderr,"--drive requires an argument\n");
                return 1;
            } else if (strcmp(argv[i], "-") == 0) {
                drive_adr[0] = 0;
                *driveno = -1;
            } else if (isdigit(argv[i][0])) {
                drive_adr[0] = 0;
                *driveno = atoi(argv[i]);
            } else {
                if(strlen(argv[i]) >= BURN_DRIVE_ADR_LEN) {
                    fprintf(stderr,"--drive address too long (max. %d)\n",
                            BURN_DRIVE_ADR_LEN-1);
                    return 2;
                }
                strcpy(drive_adr, argv[i]);
            }

        } else if (!strcmp(argv[i], "--stdin_size")) {
            ++i;
            if (i >= argc) {
                fprintf(stderr,"--stdin_size requires an argument\n");
                return 3;
            } else
                *size = atoi(argv[i]);
        } else if (!strcmp(argv[i], "--try_to_simulate")) {
            simulate_burn = 1;

        } else if (!strcmp(argv[i], "--verbose")) {
            ++i;
            if (i >= argc) {
                fprintf(stderr,"--verbose requires an argument\n");
                return 4;
            } else
                burn_set_verbosity(atoi(argv[i]));
        } else if (!strcmp(argv[i], "--help")) {
            print_help = 1;

        } else {
            if(strlen(argv[i]) >= 4096) {
                fprintf(stderr, "Source address too long (max. %d)\n", 4096-1);
                return 5;
            }
            strcpy(source_adr, argv[i]);
        }
    }
    insuffient_parameters = 1;
    if (*driveno < 0)
        insuffient_parameters = 0;
    if (source_adr[0] != 0)
        insuffient_parameters = 0; 
    if (*do_blank)
        insuffient_parameters = 0;
    if (print_help || insuffient_parameters ) {
        printf("Usage: %s\n", argv[0]);
        printf("       [--drive <address>|<driveno>|\"-\"]\n");
        printf("       [--verbose <level>] [--blank_fast|--blank_full]\n");
        printf("       [--burn_for_real|--try_to_simulate] [--stdin_size <bytes>]\n");
        printf("       [<imagefile>|\"-\"]\n");
        printf("Examples\n");
        printf("A bus scan (needs rw-permissions to see a drive):\n");
        printf("  %s --drive -\n",argv[0]);
        printf("Burn a file to drive chosen by number:\n");
        printf("  %s --drive 0 --burn_for_real my_image_file\n",
            argv[0]);
        printf("Burn a file to drive chosen by persistent address:\n");
        printf("  %s --drive /dev/hdc --burn_for_real my_image_file\n", argv[0]);
        printf("Blank a used CD-RW (is combinable with burning in one run):\n");
        printf("  %s --drive 0 --blank_fast\n",argv[0]);
        printf("Burn a compressed afio archive on-the-fly, pad up to 700 MB:\n");
        printf("  ( cd my_directory ; find . -print | afio -oZ - ) | \\\n");
        printf("  %s --drive /dev/hdc --burn_for_real --stdin_size 734003200  -\n", argv[0]);
        printf("To be read from *not mounted* CD via:\n");
        printf("   afio -tvZ /dev/hdc\n");
        printf("Program tar would need a clean EOF which our padded CD cannot deliver.\n");
        if (insuffient_parameters)
            return 6;
    }
    return 0;
}


int main(int argc, char **argv)
{
	int driveno, ret, do_blank;
	char source_adr[4096], drive_adr[BURN_DRIVE_ADR_LEN];
	off_t stdin_size;

	ret = libburner_setup(argc, argv, drive_adr, &driveno, &do_blank, 
		       source_adr, &stdin_size);
	if (ret)
		exit(ret);

	printf("Initializing library ...\n");
	if (burn_initialize())
		printf("done\n");
	else {
		printf("FAILED\n");
		fprintf(stderr,"\nFATAL: Failed to initialize libburn.\n");
		exit(33);
	}

	/** Note: driveno might change its value in this call */
	ret = libburner_aquire_drive(drive_adr, &driveno);
	if (ret<=0) {
		fprintf(stderr,"\nFATAL: Failed to aquire drive.\n");
		{ ret = 34; goto finish_libburn; }
	}
	if (ret == 2)
		{ ret = 0; goto release_drive; }
	if (do_blank) {
		ret = libburner_blank_disc(drive_list[driveno].drive,
					  do_blank == 1);
		if (ret<=0)
			{ ret = 36; goto release_drive; }
		if (ret != 2 && source_adr[0] != 0)
			ret = libburner_regrab(drive_list[driveno].drive);
		if (ret<=0) {
			fprintf(stderr,
	        "FATAL: Cannot release and grab again drive after blanking\n");
			{ ret = 37; goto finish_libburn; }
		}
	}
	if (source_adr[0] != 0) {
		ret = libburner_payload(drive_list[driveno].drive, source_adr,
				 stdin_size);
		if (ret<=0)
			{ ret = 38; goto release_drive; }
	}
	ret = 0;
release_drive:;
	if (drive_is_grabbed)
		burn_drive_release(drive_list[driveno].drive, 0);

finish_libburn:;
	/* This app does not bother to know about exact scan state. 
	   Better to accept a memory leak here. We are done anyway. */
	/* burn_drive_info_free(drive_list); */

	burn_finish();
	return ret;
}


/*  License and copyright aspects:

This all is provided under GPL.
Read. Try. Think. Play. Write yourself some code. Be free of my copyright.

Be also invited to study the code of cdrskin/cdrskin.c et al.


Clarification in my name and in the name of Mario Danic, copyright holder
on toplevel of libburn. To be fully in effect after the remaining other
copyrighted code has been replaced by ours and by copyright-free contributions
of our friends:

We, the copyright holders, agree on the interpretation that
dynamical linking of our libraries constitutes "use of" and
not "derivation from" our work in the sense of GPL, provided
those libraries are compiled from our unaltered code.

Thus you may link our libraries dynamically with applications
which are not under GPL. You may distribute our libraries and
application tools in binary form, if you fulfill the usual
condition of GPL to offer a copy of the source code -altered
or unaltered- under GPL.

We ask you politely to use our work in open source spirit
and with the due reference to the entire open source community.

If there should really arise the case where above clarification
does not suffice to fulfill a clear and neat request in open source
spirit that would otherwise be declined for mere formal reasons,
only in that case we will duely consider to issue a special license
covering only that special case.
It is the open source idea of responsible freedom which will be
decisive and you will have to prove that you exhausted all own
means to qualify for GPL.

For now we are firmly committed to maintain one single license: GPL.

History:
libburner is a compilation of my own contributions to test/burniso.c and
fresh code which replaced the remaining parts under copyright of
Derek Foreman.
My respect and my thanks to Derek for providing me a start back in 2005.

*/

