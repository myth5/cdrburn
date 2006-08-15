/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <libburn/libburn.h>

#include <stdio.h>

static struct burn_drive_info *drives;
static unsigned int n_drives;

static void show_devices()
{
	unsigned int i;

	fprintf(stderr, "Devices: (%u found)\n", n_drives);
	for (i = 0; i < n_drives; ++i) {
		fprintf(stderr, "%s - %s %s\n", drives[i].location,
			drives[i].vendor, drives[i].product);
	}
}

int main()
{
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

	show_devices();
	burn_drive_info_free(drives);
	burn_finish();
	return 0;
}
