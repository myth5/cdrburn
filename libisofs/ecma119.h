/* vim: set noet ts=8 sts=8 sw=8 : */

/** 
 * \file ecma119.h
 *
 * Structures and definitions used for writing an emca119 (ISO9660) compatible
 * volume.
 */

#ifndef __ISO_ECMA119
#define __ISO_ECMA119

#include <stdio.h>
#include <sys/time.h>
#include "susp.h"

/**
 * Persistent data for writing directories according to the ecma119 standard.
 */
struct dir_write_info
{
	struct susp_info susp;		/**< \see node_write_info */
	struct susp_info self_susp;	/**< SUSP data for this directory's
					  *  "." entry.
					  */
	struct susp_info parent_susp;	/**< SUSP data for this directory's
					  * ".." entry.
					  */

	int len;		/**< The combined length of all children's
				  *  Directory Record lengths. This includes
				  *  the System Use areas.
				  */
	int susp_len;		/**< The combined length of all children's
				  *  SUSP Continuation Areas.
				  */

	/* the parent/child information prior to relocation */
	struct iso_tree_dir *real_parent;
	int real_nchildren;
	struct iso_tree_dir **real_children;
	int real_depth;

	/* joliet information */
	int joliet_block;	/**< The block at which the Joliet version of
				  *  this directory will be written.
				  */
	int joliet_len;		/**< The combined length of all children's
				  *  Joliet Directory Record lengths.
				  */
};

/**
 * Persistent data for writing files according to the ecma119 standard.
 */
struct file_write_info
{
	struct susp_info susp;	/**< \see node_write_info */

	struct iso_tree_dir *real_me;	/**< If this is non-NULL, the file is
					  *  a placeholder for a relocated
					  *  directory and this field points to
					  *  that relocated directory.
					  */
};

/**
 * The fields in common between file_write_info and dir_write_info.
 */
struct node_write_info
{
	struct susp_info susp;	/**< The SUSP data for this file. */
};

/**
 * The possible states that the ecma119 writer can be in.
 */
enum ecma119_write_state
{
	ECMA119_WRITE_BEFORE,

	ECMA119_WRITE_SYSTEM_AREA,
	ECMA119_WRITE_PRI_VOL_DESC,
	ECMA119_WRITE_SUP_VOL_DESC_JOLIET,
	ECMA119_WRITE_VOL_DESC_TERMINATOR,
	ECMA119_WRITE_L_PATH_TABLE,
	ECMA119_WRITE_M_PATH_TABLE,
	ECMA119_WRITE_L_PATH_TABLE_JOLIET,
	ECMA119_WRITE_M_PATH_TABLE_JOLIET,
	ECMA119_WRITE_DIR_RECORDS,
	ECMA119_WRITE_DIR_RECORDS_JOLIET,
	ECMA119_WRITE_FILES,

	ECMA119_WRITE_DONE
};

/**
 * Data describing the state of the ecma119 writer. Everything here should be
 * considered private!
 */
struct ecma119_write_target
{
	struct iso_volset *volset;
	int volnum;

	time_t now;		/**< Time at which writing began. */
	int total_size;		/**< Total size of the output. This only
				  *  includes the current volume. */
	uint32_t vol_space_size;

	unsigned int rockridge:1;
	unsigned int joliet:1;
	unsigned int iso_level:2;

	int curblock;
	uint16_t block_size;
	uint32_t path_table_size;
	uint32_t path_table_size_joliet;
	uint32_t l_path_table_pos;
	uint32_t m_path_table_pos;
	uint32_t l_path_table_pos_joliet;
	uint32_t m_path_table_pos_joliet;

	struct iso_tree_dir **dirlist;	/* A pre-order list of directories
					 * (this is the order in which we write
					 * out directory records).
					 */
	struct iso_tree_dir **pathlist;	/* A breadth-first list of directories.
					 * This is used for writing out the path
					 * tables.
					 */
	int dirlist_len;		/* The length of the previous 2 lists.
					 */

	struct iso_tree_file **filelist;/* A pre-order list of files with
					 *  non-NULL paths and non-zero sizes.
					 */
	int filelist_len;		/* Length of the previous list. */

	int curfile;			/* Used as a helper field for writing
					   out filelist and dirlist */

	/* Joliet versions of the above lists. Since Joliet doesn't require
	 * directory relocation, the order of these list might be different from
	 * the lists above. */
	struct iso_tree_dir **dirlist_joliet;
	struct iso_tree_dir **pathlist_joliet;

	enum ecma119_write_state state;	/* The current state of the writer. */

	/* persistent data for the various states. Each struct should not be
	 * touched except for the writer of the relevant stage. When the writer
	 * of the relevant stage is finished, it should set all fields to 0.
	 */
	union
	{
		struct
		{
			int blocks;
			unsigned char *data;
		} path_table;
		struct
		{
			size_t pos;	/* The number of bytes we have written
					 * so far in the current directory.
					 */
			size_t data_len;/* The number of bytes in the current
					 * directory.
					 */
			unsigned char *data; /* The data (combined Directory
					 * Records and susp_CE areas) of the
					 * current directory.
					 */
			int dir;	/* The index in dirlist that we are
					 * currently writing. */
		} dir_records;
		struct
		{
			size_t pos;	/* The number of bytes we have written
					 * so far in the current file.
					 */
			size_t data_len;/* The number of bytes in the currently
					 * open file.
					 */
			FILE *fd;	/* The currently open file. */
			int file;	/* The index in filelist that we are
					 * currently writing. */
		} files;
	} state_data;
};

/**
 * Create a new ecma119_write_target from the given volume number of the
 * given volume set.
 *
 * \pre \p volnum is less than \p volset-\>volset_size.
 * \post For each node in the tree, writer_data has been allocated.
 * \post The directory heirarchy has been reorganised to be ecma119-compatible.
 */
struct ecma119_write_target *ecma119_target_new(struct iso_volset *volset,
						int volnum);

/** Macros to help with casting between node_write_info and dir/file_write_info.
 */
#define DIR_INF(a) ( (struct dir_write_info*) (a) )
#define FILE_INF(a) ( (struct file_write_info*) (a) )
#define NODE_INF(a) ( (struct node_write_info*) (a) )

#define GET_DIR_INF(a) ( (struct dir_write_info*) (a)->writer_data )
#define GET_FILE_INF(a) ( (struct file_write_info*) (a)->writer_data )
#define GET_NODE_INF(a) ( (struct node_write_info*) (a)->writer_data )

#define TARGET_ROOT(t) ( (t)->volset->volume[(t)->volnum]->root )

#define NODE_NAMELEN(n,i) strlen(iso_tree_node_get_name(ISO_NODE(n), i))
#define NODE_JOLLEN(n) ucslen(iso_tree_node_get_name(ISO_NODE(n), \
						     ISO_NAME_JOLIET))


#endif /* __ISO_ECMA119 */
