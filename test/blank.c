/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/** See this for the decisive API specs . libburn.h is The Original */
#include <libburn/libburn.h>


/*  test/blank.c , API illustration of blanking a used CD-RW
    Copyright (C) ???? - 2006 Derek Foreman
    Copyright (C) 2006 - 2006 Thomas Schmitt
    This is provided under GPL only. Don't ask for anything else for now.
    Read. Try. Think. Play. Write yourself some code. Be free of our copyright.
*/


#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
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


/** You need to aquire a drive before burning. The API offers this as one
    compact call and alternatively as application controllable gestures of
    whitelisting, scanning for drives and finally grabbing one of them.

    If you have a persistent address of the drive, then the compact call is
    to prefer. See test/burniso.c for a more detailed explanation.
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

/** Makes a previously used CD-RW ready for thorough re-usal.

    To our knowledge it is hardly possible to abort an ongoing blank operation
    because after start it is entirely handled by the drive.
    So expect a blank run to survive the end of the blanking process and be
    patient for the usual timespan of a normal blank run. Only after that
    time has surely elapsed, only then you should start any rescue attempts
    with the drive. If necessary at all.
*/
static void blank_disc(struct burn_drive *drive, int blank_fast)
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
	  "Drive media status:  %d  (see libburn/libburn.h BURN_DISC_*)\n", s);
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
	if (!do_blank) {
		burn_drive_release(drive, 0);
		return;
	}

	printf(
	    "Beginning to %s-blank CD media. Will display sector numbers.\n",
		(blank_fast?"fast":"full"));
	burn_disc_erase(drive, blank_fast);
	while (burn_drive_get_status(drive, &p)) {
		printf("%d\n", p.sector);
		sleep(2);
	}
	printf("Done\n");

	burn_drive_release(drive, 0);
}

void parse_args(int argc, char **argv, char **drive_adr, int *driveno,
		int *blank_fast)
{
	int i;
	int help = 0;
	static char no_drive_adr[]= {""};

	*drive_adr = no_drive_adr;
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--blank_fast")) {
			*blank_fast= 1;

		} else if (!strcmp(argv[i], "--blank_full")) {
			*blank_fast= 0;

		} else if (!strcmp(argv[i], "--drive")) {
			++i;
			if (i >= argc) {
				printf("--drive requires an argument\n");
				exit(3);
			} else if (isdigit(argv[i][0])) {
				*drive_adr = no_drive_adr;
				*driveno = atoi(argv[i]);
			} else
				*drive_adr = argv[i];
		} else if (!strcmp(argv[i], "--verbose")) {
			++i;
			if (i >= argc)
				printf("--verbose requires an argument\n");
			else
				burn_set_verbosity(atoi(argv[i]));
		} else if (!strcmp(argv[i], "--help")) {
			help = 1;
		}
	}
	if (help) {
		printf("Usage: %s\n", argv[0]);
		printf("       [--blank_fast|--blank_full] [--drive <address>|<num>]\n");
		printf("       [--verbose <level>]\n");
		exit(3);
	}
}

int main(int argc, char **argv)
{
	int driveno = 0, blank_fast= 1;
	char *drive_adr= NULL;


	parse_args(argc, argv, &drive_adr, &driveno, &blank_fast);

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

	blank_disc(drives[driveno].drive, blank_fast);

	burn_drive_info_free(drives);
	burn_finish();
	return 0;
}
