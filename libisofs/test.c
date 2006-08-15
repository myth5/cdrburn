/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set ts=8 sts=8 sw=8 noet : */

#define _GNU_SOURCE

#include "libisofs/libisofs.h"
#include "libburn/libburn.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define SECSIZE 2048

const char * const optstring = "JRL:h";
extern char *optarg;
extern int optind;

void usage()
{
	printf("test [OPTIONS] DIRECTORY OUTPUT\n");
}

void help()
{
	printf(
"Options:\n"
"  -J       Add Joliet support\n"
"  -R       Add Rock Ridge support\n"
"  -L <num> Set the ISO level (1 or 2)\n"
"  -h       Print this message\n"
);
}

int main(int argc, char **argv)
{
	struct iso_volset *volset;
	struct iso_volume *volume;
	struct burn_source *src;
	unsigned char buf[2048];
	FILE *fd;
	int c;
	int level=1, flags=0;
	DIR *dir;
	struct dirent *ent;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
		case 'h':
			usage();
			help();
			exit(0);
			break;
		case 'J':
			flags |= ECMA119_JOLIET;
			break;
		case 'R':
			flags |= ECMA119_ROCKRIDGE;
			break;
		case 'L':
			level = atoi(optarg);
			break;
		case '?':
			usage();
			exit(1);
			break;
		}
	}

	if (argc < 2) {
		printf ("must pass directory to build iso from\n");
		usage();
		return 1;
	}
	if (argc < 3) {
		printf ("must supply output file\n");
		usage();
		return 1;
	}
	fd = fopen(argv[optind+1], "w");
	if (!fd) {
		perror("error opening output file");
		exit(1);
	}

	volume = iso_volume_new( "VOLID", "PUBID", "PREPID" );
	volset = iso_volset_new( volume, "VOLSETID" );
	dir = opendir(argv[optind]);
	if (!dir) {
		perror("error opening input directory");
		exit(1);
	}

	while ( (ent = readdir(dir)) ) {
		struct stat st;
		char *name;

		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}

		name = malloc(strlen(argv[optind]) + strlen(ent->d_name) + 2);
		strcpy(name, argv[optind]);
		strcat(name, "/");
		strcat(name, ent->d_name);
		if (lstat(name, &st) == -1) {
			fprintf(stderr, "error opening file %s: %s\n",
					name, strerror(errno));
			exit(1);
		}

		if (S_ISDIR(st.st_mode)) {
			iso_tree_radd_dir(iso_volume_get_root(volume), name);
		} else {
			iso_tree_add_file(iso_volume_get_root(volume), name);
		}
		free(name);
	}

	iso_tree_print(iso_volume_get_root(volume), 0);

	src = iso_source_new_ecma119(volset, 0, level, flags);

	while (src->read(src, buf, 2048) == 2048) {
		fwrite(buf, 1, 2048, fd);
	}
	fclose(fd);

	return 0;
}
