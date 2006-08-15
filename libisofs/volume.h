/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set noet sts=8 ts=8 sw=8 : */

/**
 * Extra declarations for use with the iso_volume structure.
 */

#ifndef __ISO_VOLUME
#define __ISO_VOLUME

#include "libisofs.h"

struct iso_string
{
	char *cstr;
	uint16_t *jstr;
};

/**
 * Data volume.
 */
struct iso_volume
{
	int refcount;			/**< Number of used references to th
					     volume. */

	struct iso_tree_dir *root;	/**< Root of the directory tree for the
					     volume. */

	unsigned rockridge:1;
	unsigned joliet:1;
	unsigned iso_level:2;

	struct iso_string volume_id;		/**< Volume identifier. */
	struct iso_string publisher_id;		/**< Volume publisher. */
	struct iso_string data_preparer_id;	/**< Volume data preparer. */
};

/**
 * A set of data volumes.
 */
struct iso_volset
{
	int refcount;

	struct iso_volume **volume;	/**< The volumes belonging to this
					     volume set. */
	int volset_size;		/**< The number of volumes in this
					     volume set. */

	struct iso_string volset_id;	/**< The id of this volume set, encoded
					     in the current locale. */
};

#endif /* __ISO_VOLUME */
