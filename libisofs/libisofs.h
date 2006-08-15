/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set noet ts=8 sts=8 sw=8 : */

/**
 * Create an ISO-9660 data volume with Rock Ridge and Joliet extensions.
 * Usage is easy:
 *  - Create a new volume.
 *  - Add files and directories.
 *  - Write the volume to a file or create a burn source for use with Libburn.
 */

#ifndef __LIBISOFS
#define __LIBISOFS

#include "libburn/libburn.h"

/**
 * Data volume.
 * @see volume.h for details.
 */
struct iso_volume;

/**
 * A set of data volumes.
 * @see volume.h for details.
 */
struct iso_volset;

/**
 * Directory on a volume.
 * @see tree.h for details.
 */
struct iso_tree_dir;

/**
 * File on a volume.
 * @see tree.h for details.
 */
struct iso_tree_file;

/**
 * Either a file or a directory.
 * \see tree.h
 */
struct iso_tree_node;

/**
 * Possible versions of a file or directory name or identifier.
 */
enum iso_name_version {
	ISO_NAME_FULL,		/**< In the current locale. */
	ISO_NAME_ISO,		/**< Current ISO level identifier. */
	ISO_NAME_ISO_L1,	/**< ISO level 1 identifier. */
	ISO_NAME_ISO_L2,	/**< ISO level 2 identifier. */
	ISO_NAME_ROCKRIDGE,	/**< Rock Ridge file or directory name. */
	ISO_NAME_JOLIET		/**< Joliet identifier. */
};

enum ecma119_extension_flag {
	ECMA119_ROCKRIDGE	= (1<<0),
	ECMA119_JOLIET		= (1<<1)
};

/**
 * Create a new volume.
 * The parameters can be set to NULL if you wish to set them later.
 */
struct iso_volume *iso_volume_new(const char *volume_id,
				  const char *publisher_id,
				  const char *data_preparer_id);

/**
 * Free a volume.
 */
void iso_volume_free(struct iso_volume *volume);

/**
 * Get the root directory for a volume.
 */
struct iso_tree_dir *iso_volume_get_root(const struct iso_volume *volume);

/**
 * Fill in the volume identifier for a volume.
 */
void iso_volume_set_volume_id(struct iso_volume *volume,
			      const char *volume_id);

/**
 * Fill in the publisher for a volume.
 */
void iso_volume_set_publisher_id(struct iso_volume *volume,
				 const char *publisher_id);

/**
 * Fill in the data preparer for a volume.
 */
void iso_volume_set_data_preparer_id(struct iso_volume *volume,
				     const char *data_preparer_id);

/**
 * Get the current ISO level for a volume.
 */
int iso_volume_get_iso_level(const struct iso_volume *volume);

/**
 * Set the current ISO level for a volume.
 * ISO level must be 1 or 2.
 */
void iso_volume_set_iso_level(struct iso_volume *volume, int level);

/**
 * See if Rock Ridge (POSIX) is enabled for a volume.
 */
int iso_volume_get_rockridge(const struct iso_volume *volume);

/**
 * Enable or disable Rock Ridge (POSIX) for a volume.
 */
void iso_volume_set_rockridge(struct iso_volume *volume, int rockridge);

/**
 * See if Joliet (Unicode) is enabled for a volume.
 */
int iso_volume_get_joliet(const struct iso_volume *volume);

/**
 * Enable or disable Joliet (Unicode) for a volume.
 */
void iso_volume_set_joliet(struct iso_volume *volume, int joliet);

/**
 * Create a new Volume Set consisting of only one volume.
 * @param volume The first and only volume for the volset to contain.
 * @param volset_id The Volume Set ID.
 * @return A new iso_volset.
 */
struct iso_volset *iso_volset_new(struct iso_volume *volume,
                                  const char *volset_id);

/**
 * Add a file to a directory.
 *
 * \param path The path, on the local filesystem, of the file.
 *
 * \pre \p parent is non-NULL
 * \pre \p path is non-NULL and is a valid path to a non-directory on the local
 *	filesystem.
 * \return An iso_tree_file whose path is \p path and whose parent is \p parent.
 */
struct iso_tree_file *iso_tree_add_file(struct iso_tree_dir *parent,
					const char *path);

/**
 * Add a directory from the local filesystem to the tree.
 * Warning: this only adds the directory itself, no files or subdirectories.
 *
 * \param path The path, on the local filesystem, of the directory.
 *
 * \pre \p parent is non-NULL
 * \pre \p path is non-NULL and is a valid path to a directory on the local
 *	filesystem.
 * \return a pointer to the newly created directory.
 */
struct iso_tree_dir *iso_tree_add_dir(struct iso_tree_dir *parent,
				      const char *path);

/**
 * Recursively add an existing directory to the tree.
 * Warning: when using this, you'll lose pointers to files or subdirectories.
 * If you want to have pointers to all files and directories,
 * use iso_tree_add_file and iso_tree_add_dir.
 *
 * \param path The path, on the local filesystem, of the directory to add.
 *
 * \pre \p parent is non-NULL
 * \pre \p path is non-NULL and is a valid path to a directory on the local
 *	filesystem.
 * \return a pointer to the newly created directory.
 */
struct iso_tree_dir *iso_tree_radd_dir(struct iso_tree_dir *parent,
				       const char *path);

/**
 * Creates a new, empty directory on the volume.
 *
 * \pre \p parent is non-NULL
 * \pre \p name is unique among the children and files belonging to \p parent.
 *	Also, it doesn't contain '/' characters.
 *
 * \post \p parent contains a child directory whose name is \p name and whose
 *	POSIX attributes are the same as \p parent's.
 * \return a pointer to the newly created directory.
 */
struct iso_tree_dir *iso_tree_add_new_dir(struct iso_tree_dir *parent,
					  const char *name);

/**
 * Get the name of a node.
 */
const char *iso_tree_node_get_name(const struct iso_tree_node *node,
				   enum iso_name_version ver);

/**
 * Set the name of a file.
 * The name you input here will be the full name and will be used to derive the
 * ISO, RockRidge and Joliet names.
 */
void iso_tree_file_set_name(struct iso_tree_file *file, const char *name);

/**
 * Set the name of a directory.
 * The name you input here will be the full name and will be used to derive the
 * ISO, RockRidge and Joliet names.
 */
void iso_tree_dir_set_name(struct iso_tree_dir *dir, const char *name);

/**
 * Recursively print a directory to stdout.
 * \param spaces The initial number of spaces on the left. Set to 0 if you
 *	supply a root directory.
 */
void iso_tree_print(const struct iso_tree_dir *root, int spaces);

/** Create a burn_source which can be used as a data source for a track
 *
 * The volume set used to create the libburn_source can _not_ be modified
 * until the libburn_source is freed.
 *
 * \param volumeset The volume set from which you want to write
 * \param volnum The volume in the set which you want to write (usually 0)
 * \param level ISO level to write at.
 * \param flags Which extensions to support.
 *
 * \pre \p volumeset is non-NULL
 * \pre \p volnum is less than \p volset->volset_size.
 * \return A burn_source to be used for the data source for a track
 */
struct burn_source* iso_source_new_ecma119 (struct iso_volset *volumeset,
					    int volnum,
					    int level,
					    int flags);

#endif /* __LIBISOFS */
