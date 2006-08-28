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


/*  test/burniso.c , API illustration of burning a single data track to CD
    Copyright (C) ???? - 2006 Derek Foreman
    Copyright (C) 2005 - 2006 Thomas Schmitt
    This is provided under GPL only. Don't ask for anything else for now. 
    Read. Try. Think. Play. Write yourself some code. Be free of our copyright.
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


/* Some in-advance definitions to allow a more comprehensive ordering
   of the functions and their explanations in here */
int burn_app_aquire_by_adr(char *drive_adr);
int burn_app_aquire_by_driveno(int drive_no);


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

/** Wether to burn for real or to *try* to simulate a burn */
static int simulate_burn = Burniso_try_to_simulatE ;


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
int burn_app_aquire_drive(char *drive_adr, int driveno)
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
	else
		printf("done\n");
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
int burn_app_aquire_by_driveno(int driveno)
{
	char adr[BURN_DRIVE_ADR_LEN];
	int ret;

	printf("Scanning for devices ...\n");
	while (!burn_drive_scan(&drives, &n_drives)) ;
	if (n_drives <= 0) {
		printf("Failed (no drives found)\n");
		return 0;
	}
	printf("done\n");
	if (n_drives <= driveno) {
		fprintf(stderr,
			"Found only %d drives. Number %d not available.\n",
			n_drives,driveno);
		return 0;
	}


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


	/* Now save yourself from sysadmins' revenge */

	/* If n_drives == 1 this would be not really necessary, though.
	   You could now call burn_drive_grab() and avoid libburn restart.
	   We don't try to be smart here and follow the API's strong urge. */

	if (burn_drive_get_adr(&(drives[driveno]), adr) <=0) {
		fprintf(stderr,
		"Cannot inquire persistent drive address of drive number %d\n",
			driveno);
		return 0;
	}
	printf("Detected '%s' as persistent address of drive number %d\n",
		adr,driveno);

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
	return ret;
}


/** Brings the preformatted image (ISO 9660 or whatever) onto media.

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
void burn_iso(struct burn_drive *drive, const char *path, off_t size)
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
		return;
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
		return;
	}
#endif /* Burniso_late_grab_obsoletion_revoke_and_faiL */

	while (burn_drive_get_status(drive, NULL))
		usleep(1000);

	/* Evaluate drive and media */
	while ((s = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
		usleep(1000);
	if (s != BURN_DISC_BLANK) {
		burn_drive_release(drive, 0);
		if (s == BURN_DISC_FULL || s == BURN_DISC_APPENDABLE) 
			fprintf(stderr,
		       "FATAL: Media with data detected. Need blank media.\n");
		else if (s == BURN_DISC_EMPTY) 
			fprintf(stderr,"FATAL: No media detected in drive\n");
		else
			fprintf(stderr,
			    "FATAL: Cannot recognize drive and media state\n");
		return;
	}

	o = burn_write_opts_new(drive);
	burn_write_opts_set_perform_opc(o, 0);

#ifdef Burniso_raw_mode_which_i_do_not_likE
	/* This yields higher CD capacity but hampers my IDE controller
	   with burning on one drive and reading on another simultaneously */
	burn_write_opts_set_write_type(o, BURN_WRITE_RAW, BURN_BLOCK_RAW96R);
#endif

	/* This is by what cdrskin competes with cdrecord -sao which
	   i understand is the mode preferrably advised by Joerg Schilling */
	burn_write_opts_set_write_type(o, BURN_WRITE_SAO, BURN_BLOCK_SAO);
	if(simulate_burn)
		printf("\n*** Will TRY to SIMULATE burning ***\n\n");
	burn_write_opts_set_simulate(o, simulate_burn);
	burn_structure_print_disc(disc);
	burn_drive_set_speed(drive, 0, 0);
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
	burn_drive_release(drive, 0);
	burn_track_free(tr);
	burn_session_free(session);
	burn_disc_free(disc);
	if(simulate_burn)
		printf("\n*** Did TRY to SIMULATE burning ***\n\n");
}

/** Converts command line arguments into a few program parameters.
*/
void parse_args(int argc, char **argv, char **drive_adr, int *driveno,
		 char **iso, off_t *size)
{
	int i;
	int help = 0;
	static char no_drive_adr[]= {""};

	*drive_adr = no_drive_adr;
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--drive")) {
			++i;
			if (i >= argc) {
				printf("--drive requires an argument\n");
				exit(3);
			} else if (isdigit(argv[i][0])) {
				*drive_adr = no_drive_adr;
				*driveno = atoi(argv[i]);
			} else
				*drive_adr = argv[i];
		} else if (!strcmp(argv[i], "--burn_for_real")) {
			simulate_burn = 0;

		} else if (!strcmp(argv[i], "--try_to_simulate")) {
			simulate_burn = 1;

		} else if (!strcmp(argv[i], "--stdin_size")) {
			++i;
			if (i >= argc) {
				printf("--stdin_size requires an argument\n");
				exit(3);
			} else
				*size = atoi(argv[i]);
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
	if (help || !*iso) {
		printf("Usage: %s\n", argv[0]);
		printf("       [--drive <address>|<driveno>] [--stdin_size <bytes>] [--verbose <level>]\n");
		printf("       [--burn_for_real|--try_to_simulate] <imagefile>|\"-\"\n");
		exit(EXIT_FAILURE);
	}
}


int main(int argc, char **argv)
{
	int driveno = 0;
	char *iso = NULL, *drive_adr= NULL;
	off_t stdin_size= 650*1024*1024;

	parse_args(argc, argv, &drive_adr, &driveno, &iso, &stdin_size);

	printf("Initializing library ...\n");
	if (burn_initialize())
		printf("done\n");
	else {
		printf("Failed\n");
		fprintf(stderr,"\nFATAL: Failed to initialize libburn.\n");
		return 1;
	}

	if (burn_app_aquire_drive(drive_adr,driveno)<=0) {
		fprintf(stderr,"\nFATAL: Failed to aquire drive.\n");
		return 2;
	}

	burn_iso(drives[driveno].drive, iso, stdin_size);

	burn_drive_info_free(drives);
	burn_finish();
	return 0;
}
