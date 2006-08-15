/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __WRITER
#define __WRITER

#include "libisofs.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

enum iso_write_state
{
	ISO_WRITE_BEFORE,

	ISO_WRITE_SYSTEM_AREA,
	ISO_WRITE_PRI_VOL_DESC,
	ISO_WRITE_VOL_DESC_TERMINATOR,
	ISO_WRITE_L_PATH_TABLE,
	ISO_WRITE_M_PATH_TABLE,
	ISO_WRITE_DIR_RECORDS,
	ISO_WRITE_ER_AREA,
	ISO_WRITE_FILES,

	ISO_WRITE_DONE
};

/** File Flags (9.1.6) */
enum
{
	ISO_FILE_FLAG_NORMAL = 0,
	ISO_FILE_FLAG_HIDDEN = 1 << 0,
	ISO_FILE_FLAG_DIRECTORY = 1 << 1,
	ISO_FILE_FLAG_ASSOCIATED = 1 << 2,
	ISO_FILE_FLAG_RECORD = 1 << 3,
	ISO_FILE_FLAG_PROTECTION = 1 << 4,
	ISO_FILE_FLAG_MULTIEXTENT = 1 << 7
};

struct iso_write_target
{
	struct iso_volumeset *volset;
	int volume;

	/* the time at which the writing began */
	time_t now;

	/* size of a physical sector on the target disc */
	int phys_sector_size;
	/* size of the total output */
	int total_size;

	/* when compiling the iso, this is the next available logical block.
	   when writing the iso, this is the next block to write. */
	int logical_block;
	/* The number of Logical Blocks for the Volume Space */
	int volume_space_size;
	/* The Logical Block size */
	int logical_block_size;
	/* The Path Table size */
	int path_table_size;
	/* Locations of Type L Path Table (Logical Block Number) */
	int l_path_table_pos;
	/* Locations of Type M Path Table (Logical Block Number) */
	int m_path_table_pos;
	/* Location of the SUSP ER area (Logical Block Number) */
	int susp_er_pos;
	/* Current file being written in iso_write_files() */
	struct iso_tree_file **current_file;
	FILE *current_fd;

	/* what we're doing when the generate function gets called next */
	enum iso_write_state state;
	union
	{
		struct iso_state_system_area
		{
			/* how many sectors in the system area have been
			   written */
			int sectors;
		} system_area;
		struct iso_state_path_tables
		{
			/* how many sectors in the path table area have been
			   written */
			int sectors;
		} path_tables;
		struct iso_state_dir_records
		{
			/* how many sectors in the directory records area have
			   been written */
			int sectors;
		} dir_records;
		struct iso_state_files
		{
			/* how many sectors in the current file have been
			   written */
			int sectors;
		} files;
	} state_data;
};

#endif /* __WRITER */
