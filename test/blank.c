/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <libburn/libburn.h>

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static struct burn_drive_info *drives;
static unsigned int n_drives;

static void blank_disc(struct burn_drive *drive)
{
	enum burn_disc_status s;
	struct burn_progress p;

	if (!burn_drive_grab(drive, 1)) {
		fprintf(stderr, "Unable to open the drive!\n");
		return;
	}

	while (burn_drive_get_status(drive, NULL)) {
		usleep(1000);
	}

	while ((s = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
		usleep(1000);
	printf("%d\n", s);
	if (s != BURN_DISC_FULL) {
		burn_drive_release(drive, 0);
		fprintf(stderr, "No disc found!\n");
		return;
	}

	fprintf(stderr, "Blanking disc...");
	burn_disc_erase(drive, 1);

	while (burn_drive_get_status(drive, &p)) {
		printf("%d\n", p.sector);
		sleep(2);
	}
	fprintf(stderr, "Done\n");

	burn_drive_release(drive, 0);
}

void parse_args(int argc, char **argv, int *drive)
{
	int i;
	int help = 0;

	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--drive")) {
			++i;
			if (i >= argc)
				printf("--drive requires an argument\n");
			else
				*drive = atoi(argv[i]);
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
		printf("Usage: %s [--drive <num>] [--verbose <level>]\n",
		       argv[0]);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	int drive = 0;

	parse_args(argc, argv, &drive);

	fprintf(stderr, "Initializing library...");
	if (burn_initialize())
		fprintf(stderr, "Success\n");
	else {
		printf("Failed\n");
		return 1;
	}

	fprintf(stderr, "Scanning for devices...");
	while (!burn_drive_scan(&drives, &n_drives)) ;
	fprintf(stderr, "Done\n");

	blank_disc(drives[drive].drive);
	burn_drive_info_free(drives);
	burn_finish();
	return 0;
}
