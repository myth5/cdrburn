/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set noet ts=8 sts=8 sw=8 : */

/**
 * \file tree.h
 *
 * Extra declarations for use with the iso_tree_dir and iso_tree_file
 * structures.
 */

#ifndef __ISO_TREE
#define __ISO_TREE

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>

#include "libisofs.h"

/**
 * File or directory names or identifiers.
 */
struct iso_names
{
	char *full;		/**< Full version: either part of the path or
				     user input. */
	char *iso1;		/**< ISO level 1 identifier. */
	char *iso2;		/**< ISO level 2 identifier. */
	char *rockridge;	/**< Rock Ridge file or directory name. */
	uint16_t *joliet;	/**< Joliet identifier. */
};

/**
 * Directory on a volume.
 */
struct iso_tree_dir
{
	struct iso_tree_dir *parent;	/**< \see iso_tree_node */
	struct iso_volume *volume;	/**< \see iso_tree_node */
	struct iso_names name;		/**< \see iso_tree_node */
	struct stat attrib;		/**< \see iso_tree_node */
	off_t block;			/**< \see iso_tree_node */
	uint8_t dirent_len;		/**< \see iso_tree_node */
	void *writer_data;		/**< \see iso_tree_node */

	int depth;			/**< The depth of this directory in the
					  *  Directory Heirarchy. This is 1 for
					  *  the root directory.
					  */

	int nchildren;			/**< Number of child directories. */
	int nfiles;			/**< Number of files in this directory.
					  */
	struct iso_tree_dir **children;	/**< Child directories. */
	struct iso_tree_file **files;	/**< Files in this directory. */
};

/**
 * File on a volume.
 */
struct iso_tree_file
{
	struct iso_tree_dir *parent;	/**< \see iso_tree_node */
	struct iso_volume *volume;	/**< \see iso_tree_node */
	struct iso_names name;		/**< \see iso_tree_node */
	struct stat attrib;		/**< \see iso_tree_node */
	off_t block;			/**< \see iso_tree_node */
	uint8_t dirent_len;		/**< \see iso_tree_node */
	void *writer_data;		/**< \see iso_tree_node */

	char *path;			/**< Location of the file in the
					  *  local filesystem. This can be a
					  *  full or relative path. If this is
					  *  NULL, then the file doesn't exist
					  *  in the local filesystem and its
					  *  size must be zero.
					  *
					  *  FIXME: Allow references to files
					  *  on other volumes belonging to the
					  *  same volset as this file.
					  */
};

/**
 * Fields in common between iso_tree_file and iso_tree_dir.
 */
struct iso_tree_node
{
	struct iso_tree_dir *parent;	/**< The parent of this node. Must be
					  *  non-NULL unless we are the
					  *  root directory on a volume.
					  */
	struct iso_volume *volume;	/**< The volume to which this node
					  *  belongs.
					  */
	struct iso_names name;		/**< The name of this node in its parent
					  *  directory. Must be non-NULL unless
					  *  we are the root directory on a
					  *  volume.
					  */
	struct stat attrib;		/**< The POSIX attributes of this
					  *  node as documented in "man 2 stat".
					  *
					  *  Any node that is not a regular
					  *  file or a directory must have
					  *  \p attrib.st_size set to zero. Any
					  *  node that is a directory will have
					  *  \p attrib.st_size filled out by the
					  *  writer.
					  */

	/* information used for writing */
	off_t block;			/**< The block at which this node's
					  *  data will be written.
					  */
	uint8_t dirent_len;		/**< The size of this node's
					  *  Directory Record in its parent.
					  *  This does not include System Use
					  *  fields, if present.
					  */
	void *writer_data;		/**< This is writer-specific data. It
					  *  must be set to NULL when a node
					  *  is created and it should be NULL
					  *  again when the node is freed.
					  */
};

/** A macro to simplify casting between nodes and files/directories. */
#define ISO_NODE(a) ( (struct iso_tree_node*) (a) )

/** A macro to simplify casting between nodes and directories. */
#define ISO_DIR(a) ( (struct iso_tree_dir*) (a) )

/** A macro to simplify casting between nodes and files. */
#define ISO_FILE(a) ( (struct iso_tree_file*) (a) )

/**
 * Create a new root directory for a volume.
 *
 * \param volume The volume for which to create a new root directory.
 *
 * \pre \p volume is non-NULL.
 * \post \p volume has a non-NULL, empty root directory.
 * \return \p volume's new non-NULL, empty root directory.
 */
struct iso_tree_dir *iso_tree_new_root(struct iso_volume *volume);

/**
 * Create a new, empty, file.
 *
 * \param parent The parent of the new file.
 * \param name The name of the new file, encoded in the current locale.
 *
 * \pre \p parent is non-NULL.
 * \pre \p name is non-NULL and it does not match any other file or directory
 *	name in \p parent.
 * \post \p parent contains a file with the following properties:
 *	- the file's (full) name is \p name
 *	- the file's POSIX permissions are the same as \p parent's
 *	- the file is a regular file
 *	- the file is empty
 *
 * \return \p parent's newly created file.
 */
struct iso_tree_file *iso_tree_add_new_file(struct iso_tree_dir *parent,
					    const char *name);

/**
 * Recursively free a directory.
 *
 * \param root The root of the directory heirarchy to free.
 *
 * \pre \p root is non-NULL.
 */
void iso_tree_free(struct iso_tree_dir *root);

/**
 * Recursively sort all the files and child directories in a directory.
 *
 * \param root The root of the directory heirarchy to sort.
 *
 * \pre \p root is non-NULL.
 * \post For each directory \p dir in the directory heirarchy descended fro
 *	root, the fields in \p dir.children and \p dir.files are alphabetically
 *	sorted according to \p name.full.
 *
 * \see iso_names
 */
void iso_tree_sort(struct iso_tree_dir *root);

/**
 * Compare the names of 2 nodes, \p *v1 and \p *v2. This is compatible with
 * qsort.
 */
int iso_node_cmp(const void *v1, const void *v2);

/**
 * Compare the joliet names of 2 nodes, compatible with qsort.
 */
int iso_node_cmp_joliet(const void *v1, const void *v2);

/**
 * Compare the iso names of 2 nodes, compatible with qsort.
 */
int iso_node_cmp_iso(const void *v1, const void *v2);

/**
 * A function that prints verbose information about a directory.
 *
 * \param dir The directory about which to print information.
 * \param data Unspecified function-dependent data.
 * \param spaces The number of spaces to prepend to the output.
 *
 * \see iso_tree_print_verbose
 */
typedef void (*print_dir_callback) (const struct iso_tree_dir *dir,
				    void *data,
				    int spaces);
/**
 * A function that prints verbose information about a file.
 *
 * \param dir The file about which to print information.
 * \param data Unspecified function-dependent data.
 * \param spaces The number of spaces to prepend to the output.
 *
 * \see iso_tree_print_verbose
 */
typedef void (*print_file_callback) (const struct iso_tree_file *file,
				     void *data,
				     int spaces);

/**
 * Recursively print a directory heirarchy. For each node in the directory
 * heirarchy, call a callback function to print information more verbosely.
 *
 * \param root The root of the directory heirarchy to print.
 * \param dir The callback function to call for each directory in the tree.
 * \param file The callback function to call for each file in the tree.
 * \param callback_data The data to pass to the callback functions.
 * \param spaces The number of spaces to prepend to the output.
 *
 * \pre \p root is not NULL.
 * \pre Neither of the callback functions modifies the directory heirarchy.
 */
void iso_tree_print_verbose(const struct iso_tree_dir *root,
			    print_dir_callback dir,
			    print_file_callback file,
			    void *callback_data,
			    int spaces);

/**
 * Get a non-const version of the node name. This is used for name-mangling
 * iso names. It should only be used internally in libisofs; all other users
 * should only access the const name.
 */
char *iso_tree_node_get_name_nconst(const struct iso_tree_node *node,
				    enum iso_name_version ver);
#endif /* __ISO_TREE */
