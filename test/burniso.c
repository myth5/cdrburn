/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */


/** See this for the decisive API specs . libburn.h is The Original */
#include <libburn/libburn.h>

/** IMPORTANT: By default this program tries to make a simulated burn
	       on the CD recorder. Some obey, some do not.
	       If you want to burn really readable CD for sure by default,
	       then set this macro to 0 .
	       Explicit options: --burn_for_real  and  --try_to_simulate
*/
#define Burniso_try_to_simulatE 1

/**                               Overview 

    Before you can do anything, you have to initialize libburn by
       burn_initialize()
    as it is done in main() at the end of this file. Then you aquire a
    drive in an appropriate way conforming to the API. The two main
    approaches are shown here in application functions:
       burn_app_aquire_by_adr()    demonstrates usage as of cdrecord traditions
       burn_app_aquire_by_driveno()     demonstrates a scan-and-choose approach
    With that aquired drive you blank a CD-RW
       burn_app_blank_disc()
    Between blanking and burning one eventually has to reload the drive status
       burn_app_regrab()
    With the aquired drive you can burn to CD-R or blank CD-RW
       burn_app_payload()
    When everything is done, main() releases the drive and shuts down libburn:
       burn_drive_release();
       burn_finish()
*/


/*  test/burniso.c , API illustration of burning a single data track to CD
    Copyright (C) ???? - 2006 Derek Foreman
    Copyright (C) 2005 - 2006 Thomas Schmitt
    This is provided under GPL only. Don't ask for anything else for now. 
    Read. Try. Think. Play. Write yourself some code. Be free of our copyright.

    Be also invited to study the code of cdrskin/cdrskin.c et al.
*/


#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>



/** This is a rough example. For simplicity it uses global variables.
    Drives are systemwide global, so we do not give away much of good style.
*/

/** This list will hold the drives known to libburn. This might be all CD
    drives of the system and thus might impose severe impact on the system.
*/
static struct burn_drive_info *drives;

/** If you start a long lasting operation with n_drives > 1 then you are not
    friendly to the users of other drives on those systems. Beware. */
static unsigned int n_drives;

/** This variable indicates wether the drive is grabbed and must be
    finally released */
static int drive_is_grabbed = 0;

/** Wether to burn for real or to *try* to simulate a burn */
static int simulate_burn = Burniso_try_to_simulatE ;


/* Some in-advance definitions to allow a more comprehensive ordering
   of the functions and their explanations in here */
int burn_app_aquire_by_adr(char *drive_adr);
int burn_app_aquire_by_driveno(int *drive_no);


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
int burn_app_aquire_drive(char *drive_adr, int *driveno)
{
	int ret;

	if(drive_adr != NULL && drive_adr[0] != 0)
		ret = burn_app_aquire_by_adr(drive_adr);
	else
		ret = burn_app_aquire_by_driveno(driveno);
	return ret;
}


/** If the persistent drive address is known, then this approach is much
    more un-obtrusive to the systemwide livestock of drives. Only the
    given drive device will be opened during this procedure.
*/
int burn_app_aquire_by_adr(char *drive_adr)
{
	int ret;
	
	printf("Aquiring drive '%s' ...\n",drive_adr);
	ret = burn_drive_scan_and_grab(&drives,drive_adr,1);
	if (ret <= 0)
		printf("Failed\n");
	else {
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
*/
int burn_app_aquire_by_driveno(int *driveno)
{
	char adr[BURN_DRIVE_ADR_LEN];
	int ret, i;

	printf("Scanning for devices ...\n");
	while (!burn_drive_scan(&drives, &n_drives)) ;
	if (n_drives <= 0) {
		printf("Failed (no drives found)\n");
		return 0;
	}
	printf("done\n");

	/* Interactive programs may choose the drive number at this moment.

	   drive[0] to drive[n_drives-1] are struct burn_drive_info
	   as defined in  libburn/libburn.h  . This structure is part of API
	   and thus will strive for future compatibility on source level.
	   Have a look at the info offered. Have a look at test/devices.c .
	   Caution: do not take .location for drive address. Always use
		    burn_drive_get_adr() or you might become incompatible
		    in future.
	   Note: bugs with struct burn_drive_info - if any - will not be
		 easy to fix. Please report them but also strive for
		 workarounds on application level.
	*/
	printf("\nOverview of accessible drives (%d found) :\n",
		n_drives);
	printf("-----------------------------------------------------------------------------\n");
	for (i = 0; i < n_drives; i++) {
		if (burn_drive_get_adr(&(drives[i]), adr) <=0)
			strcpy(adr, "-get_adr_failed-");
		printf("%d  --drive '%s'  :  '%s'  '%s'\n",
			i,adr,drives[i].vendor,drives[i].product);
	}
printf("-----------------------------------------------------------------------------\n\n");

	if (*driveno < 0) {
		printf("Pseudo-drive \"-\" given : bus scanning done.\n");
		return 2; /* only return 1 will cause a burn */
	}

	/* We already made our choice via command line. (default is 0) */
	if (n_drives <= *driveno || *driveno < 0) {
		fprintf(stderr,
			"Found only %d drives. Number %d not available.\n",
			n_drives, *driveno);
		return 0;
	}


	/* Now save yourself from sysadmins' revenge */

	/* If n_drives == 1 this would be not really necessary, though.
	   You could now call burn_drive_grab() and avoid libburn restart.
	   We don't try to be smart here and follow the API's strong urge. */

	if (burn_drive_get_adr(&(drives[*driveno]), adr) <=0) {
		fprintf(stderr,
		"Cannot inquire persistent drive address of drive number %d\n",
			*driveno);
		return 0;
	}
	printf("Detected '%s' as persistent address of drive number %d\n",
		adr,*driveno);

/* In cdrskin this causes a delayed sigsegv. I understand we risk only
   a small memory leak by not doing:

	burn_drive_info_free(drives);
*/
	burn_finish();
	printf(
	     "Re-Initializing library to release any unintended drives ...\n");
	if (burn_initialize())
		printf("done\n");
	else {
		printf("Failed\n");
		fprintf(stderr,"\nFailed to re-initialize libburn.\n");
		return 0;
	}
	ret = burn_app_aquire_by_adr(adr);
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
int burn_app_blank_disc(struct burn_drive *drive, int blank_fast)
{
	enum burn_disc_status s;
	struct burn_progress p;
	int do_blank = 0;

#ifdef Blank_late_grab_obsoletion_revoke_and_faiL
        /* This is obsoleted now by the burn_app_aquire* functions.
           There is no real benefit in grabbing late.
           Beneficial is to scan only one drive.
        */ 
	if (!burn_drive_grab(drive, 1)) {
		fprintf(stderr, "Unable to open the drive!\n");
		return;
	}
#endif /* Blank_late_grab_obsoletion_revoke_and_faiL */

	while (burn_drive_get_status(drive, NULL))
		usleep(1000);

	while ((s = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
		usleep(1000);
	printf(
	 "Drive media status:  %d  (see  libburn/libburn.h  BURN_DISC_*)\n",s);
	do_blank = 0;
	if (s == BURN_DISC_BLANK) {
		fprintf(stderr,
		  "IDLE: Blank CD media detected. Will leave it untouched\n");
	} else if (s == BURN_DISC_FULL || s == BURN_DISC_APPENDABLE) {
		do_blank = 1;
	} else if (s == BURN_DISC_EMPTY) {
		fprintf(stderr,"FATAL: No media detected in drive\n");
	} else {
		fprintf(stderr,
			"FATAL: Cannot recognize drive and media state\n");
	}
	if(do_blank && !burn_disc_erasable(drive)) {
		fprintf(stderr,
		       "FATAL : Media is not of erasable type\n");
		do_blank = 0;
	}
	if (!do_blank)
		return 2;

	printf(
	    "Beginning to %s-blank CD media. Will display sector numbers.\n",
		(blank_fast?"fast":"full"));
	printf(
          "Expect some garbage numbers and some 0 sector numbers at first.\n");
	burn_disc_erase(drive, blank_fast);
	while (burn_drive_get_status(drive, &p)) {
		printf("%d\n", p.sector);
		sleep(2);
	}
	printf("Done\n");
	return 1;

}


/** This gesture is necessary to get the drive info after blanking */
int burn_app_regrab(struct burn_drive *drive) {
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
	 	printf("Failed\n");
	return !!ret;
}


/** Brings the preformatted image (ISO 9660, afio, ext2, whatever) onto media.

    To make sure your image is readable on any Linux machine, you should
    add at least 300 kB of padding. This program will not do this for you.
    For a file on disk, do:
      dd if=/dev/zero bs=1K count=300 >>my_image_file
    For on-the-fly streams it suffices to add the 300 kB to the argument for
    --stdin_size (which is then needed due to lack of TAO mode).

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
int burn_app_payload(struct burn_drive *drive, const char *path, off_t size)
{
	struct burn_source *src;
	struct burn_disc *disc;
	struct burn_session *session;
	struct burn_write_opts *o;
	enum burn_disc_status s;
	struct burn_track *tr;
	struct burn_progress p;

	disc = burn_disc_create();
	session = burn_session_create();
	burn_disc_add_session(disc, session, BURN_POS_END);
	tr = burn_track_create();
	burn_track_define_data(tr, 0, 0, 0, BURN_MODE1);
	if (path[0] == '-' && path[1] == 0) {
		src = burn_fd_source_new(0, -1, size);
		printf("Note: using standard input as source with %.f bytes\n",
			(double) size);
	} else
		src = burn_file_source_new(path, NULL);
	assert(src);

	if (burn_track_set_source(tr, src) != BURN_SOURCE_OK) {
		printf("problem with the source\n");
		return 0;
	}
	burn_session_add_track(session, tr, BURN_POS_END);
	burn_source_free(src);

#ifdef Burniso_late_grab_obsoletion_revoke_and_faiL
	/* This is obsoleted now by the burn_app_aquire* functions.
	   There is no real benefit in grabbing late.
	   Beneficial is to scan only one drive.
	*/
	if (!burn_drive_grab(drive, 1)) {
		printf("Unable to open the drive!\n");
		return 0;
	}
#endif /* Burniso_late_grab_obsoletion_revoke_and_faiL */

	while (burn_drive_get_status(drive, NULL))
		usleep(1000);

	/* Evaluate drive and media */
	while ((s = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
		usleep(1000);
	if (s != BURN_DISC_BLANK) {
		if (s == BURN_DISC_FULL || s == BURN_DISC_APPENDABLE) 
			fprintf(stderr,
		       "FATAL: Media with data detected. Need blank media.\n");
		else if (s == BURN_DISC_EMPTY) 
			fprintf(stderr,"FATAL: No media detected in drive\n");
		else
			fprintf(stderr,
			    "FATAL: Cannot recognize drive and media state\n");
		return 0;
	}

	o = burn_write_opts_new(drive);
	burn_write_opts_set_perform_opc(o, 0);

#ifdef Burniso_raw_mode_which_i_do_not_likE
	/* This yields higher CD capacity but hampers my IDE controller
	   with burning on one drive and reading on another simultaneously.
	   My burner does not obey the order --try_to_simulate in this mode.
        */
	burn_write_opts_set_write_type(o, BURN_WRITE_RAW, BURN_BLOCK_RAW96R);
#else

	/* This is by what cdrskin competes with cdrecord -sao which
	   i understand is the mode preferrably advised by Joerg Schilling */
	burn_write_opts_set_write_type(o, BURN_WRITE_SAO, BURN_BLOCK_SAO);

#endif
	if(simulate_burn)
		printf("\n*** Will TRY to SIMULATE burning ***\n\n");
	burn_write_opts_set_simulate(o, simulate_burn);
	burn_structure_print_disc(disc);
	burn_drive_set_speed(drive, 0, 0);
	burn_write_opts_set_underrun_proof(o, 1);

	printf("Burning starts. Expect up to a minute of zero progress.\n");
	burn_disc_write(o, disc);

	burn_write_opts_free(o);
	while (burn_drive_get_status(drive, NULL) == BURN_DRIVE_SPAWNING) ;
	while (burn_drive_get_status(drive, &p)) {
		printf("S: %d/%d ", p.session, p.sessions);
		printf("T: %d/%d ", p.track, p.tracks);
		printf("L: %d: %d/%d\n", p.start_sector, p.sector,
		       p.sectors);
		sleep(2);
	}
	printf("\n");
	burn_track_free(tr);
	burn_session_free(session);
	burn_disc_free(disc);
	if(simulate_burn)
		printf("\n*** Did TRY to SIMULATE burning ***\n\n");
	return 0;
}


/** Converts command line arguments into a few program parameters.
*/
void parse_args(int argc, char **argv, char **drive_adr, int *driveno,
		int *do_blank, char **iso, off_t *size)
{
	int i, insuffient_parameters = 0;
	int help = 0;
	static char no_drive_adr[] = {""}, no_iso[] = {""};

	*drive_adr = no_drive_adr;
	*driveno = 0;
	*do_blank = 0;
	*iso = no_iso;
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
				printf("--drive requires an argument\n");
				exit(3);
			} else if (strcmp(argv[i], "-") == 0) {
				*drive_adr = no_drive_adr;
				*driveno = -1;
			} else if (isdigit(argv[i][0])) {
				*drive_adr = no_drive_adr;
				*driveno = atoi(argv[i]);
			} else
				*drive_adr = argv[i];
		} else if (!strcmp(argv[i], "--stdin_size")) {
			++i;
			if (i >= argc) {
				printf("--stdin_size requires an argument\n");
				exit(3);
			} else
				*size = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--try_to_simulate")) {
			simulate_burn = 1;

		} else if (!strcmp(argv[i], "--verbose")) {
			++i;
			if (i >= argc)
				printf("--verbose requires an argument\n");
			else
				burn_set_verbosity(atoi(argv[i]));
		} else if (!strcmp(argv[i], "--help")) {
			help = 1;
		} else
			*iso = argv[i];
	}
	if ((*iso)[0] ==0 && *driveno >= 0 && *do_blank == 0)
		insuffient_parameters = 1;
	if (help || insuffient_parameters ) {
		printf("Usage: %s\n", argv[0]);
		printf("       [--drive <address>|<driveno>|\"-\"]\n");
		printf("       [--verbose <level>] [--blank_fast|--blank_full]\n");
		printf("       [--burn_for_real|--try_to_simulate] [--stdin_size <bytes>]\n");
		printf("       <imagefile>|\"-\"\n");
		printf("Examples\n");
		printf("A bus scan (needs rw-permissions to see a drive):\n");
		printf("  %s --drive -\n",argv[0]);
		printf("Burn a file to drive chosen by number:\n");
		printf("  %s --drive 0 --burn_for_real my_image_file\n",
			argv[0]);
		printf("Burn a file to drive chosen by persistent address:\n");
		printf("  %s --drive /dev/hdc --burn_for_real my_image_file\n",
			argv[0]);
		printf("Burn a compressed afio archive on-the-fly, pad up to 700 MB:\n");
		printf("  ( cd my_directory ; find . -print | afio -oZ - ) | \\\n");
                printf("  %s --drive /dev/hdc --burn_for_real --stdin_size 734003200  -\n",
			argv[0]);
		printf("To be read from *not mounted* CD via:\n");
		printf("   afio -tvZ /dev/hdc\n");
		printf("Program tar would need a clean EOF which our padded CD cannot deliver.\n");
		if (insuffient_parameters)
			exit(4);
	}
}


int main(int argc, char **argv)
{
	/* Note: the effective default values are set in parse_args() */
	int driveno = 0, ret, do_blank= 0;
	char *iso = "", *drive_adr= "";
	off_t stdin_size= 0;

	parse_args(argc, argv, &drive_adr, &driveno, &do_blank, 
		   &iso, &stdin_size);

	printf("Initializing library ...\n");
	if (burn_initialize())
		printf("done\n");
	else {
		printf("Failed\n");
		fprintf(stderr,"\nFATAL: Failed to initialize libburn.\n");
		return 1;
	}

	/** Note: driveno might change its value in this call */
	ret= burn_app_aquire_drive(drive_adr, &driveno);

	if (ret<=0) {
		fprintf(stderr,"\nFATAL: Failed to aquire drive.\n");
		{ ret = 2; goto finish_libburn; }
	}
	if (do_blank) {
		ret = burn_app_blank_disc(drives[driveno].drive,
					  do_blank == 1);
		if (ret<=0)
			{ ret = 4; goto release_drive; }
		ret = burn_app_regrab(drives[driveno].drive);
		if (ret<=0) {
			fprintf(stderr,
	        "FATAL: Cannot release and grab again drive after blanking\n");
			{ ret = 5; goto finish_libburn; }
		}
	}
	if (iso[0] != 0) {
		ret = burn_app_payload(drives[driveno].drive, iso, stdin_size);
		if (ret<=0)
			{ ret = 6; goto release_drive; }
	}
	ret = 0;
release_drive:;
	if(drive_is_grabbed)
		burn_drive_release(drives[driveno].drive, 0);

finish_libburn:;
	/* This app does not bother to know about exact scan state. 
	   Better to accept a memory leak here. We are done anyway. */
	/* burn_drive_info_free(drives); */

	burn_finish();
	return ret;
}
