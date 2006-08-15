/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <libburn/libburn.h>

#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static struct burn_drive_info *drives;
static unsigned int n_drives;

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

	if (!burn_drive_grab(drive, 1)) {
		printf("Unable to open the drive!\n");
		return;
	}
	while (burn_drive_get_status(drive, NULL))
		usleep(1000);

	while ((s = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
		usleep(1000);

	if (s != BURN_DISC_BLANK) {
		burn_drive_release(drive, 0);
		printf("put a blank in the drive, corky\n");
		return;
	}
	o = burn_write_opts_new(drive);
	burn_write_opts_set_perform_opc(o, 0);
	burn_write_opts_set_write_type(o, BURN_WRITE_RAW, BURN_BLOCK_RAW96R);
	burn_write_opts_set_simulate(o, 1);

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
}

void parse_args(int argc, char **argv, int *drive, char **iso, off_t *size)
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
		} else if (!strcmp(argv[i], "--stdin_size")) {
			++i;
			if (i >= argc)
				printf("--stdin_size requires an argument\n");
			else
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
		printf("Usage: %s [--drive <num>] [--stdin_size <bytes>] [--verbose <level>] isofile|-\n", argv[0]);
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	int drive = 0;
	char *iso = NULL;
	off_t stdin_size= 650*1024*1024;

	parse_args(argc, argv, &drive, &iso, &stdin_size);

	printf("Initializing library...");
	if (burn_initialize())
		printf("Success\n");
	else {
		printf("Failed\n");
		return 1;
	}

	printf("Scanning for devices...");
	while (!burn_drive_scan(&drives, &n_drives)) ;
	printf("Done\n");

	burn_iso(drives[drive].drive, iso, stdin_size);

	burn_drive_info_free(drives);
	burn_finish();
	return 0;
}
