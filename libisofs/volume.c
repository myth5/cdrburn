/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set ts=8 sts=8 sw=8 noet : */

#include <stdlib.h>

#include "libisofs.h"
#include "tree.h"
#include "util.h"
#include "volume.h"
#include <string.h>

struct iso_volset *iso_volset_new(struct iso_volume *vol, const char *id)
{
	struct iso_volset *volset = calloc(1, sizeof(struct iso_volset));

	volset->volset_size = 1;
	volset->volume = malloc(sizeof(void *));
	volset->volume[0] = vol;
	volset->volset_id.cstr = strdup(id);
	volset->volset_id.jstr = iso_j_str(id);
	return volset;
}

struct iso_volume *iso_volume_new(const char *volume_id,
				  const char *publisher_id,
				  const char *data_preparer_id)
{
	struct iso_volume *volume;

	volume = calloc(1, sizeof(struct iso_volume));
	volume->refcount = 1;

	/* Get a new root directory. */
	volume->root = iso_tree_new_root(volume);

	/* Set these fields, if given. */
	if (volume_id != NULL) {
		volume->volume_id.cstr = strdup(volume_id);
		volume->volume_id.jstr = iso_j_str(volume_id);
	}
	if (publisher_id != NULL) {
		volume->publisher_id.cstr = strdup(publisher_id);
		volume->publisher_id.jstr = iso_j_str(publisher_id);
	}
	if (data_preparer_id != NULL) {
		volume->data_preparer_id.cstr = strdup(data_preparer_id);
		volume->data_preparer_id.jstr = iso_j_str(data_preparer_id);
	}
	return volume;
}

void iso_volume_free(struct iso_volume *volume)
{
	/* Only free if no references are in use. */
	if (--volume->refcount < 1) {
		iso_tree_free(volume->root);

		free(volume->volume_id.cstr);
		free(volume->volume_id.jstr);
		free(volume->publisher_id.cstr);
		free(volume->publisher_id.jstr);
		free(volume->data_preparer_id.cstr);
		free(volume->data_preparer_id.jstr);

		free(volume);
	}
}

struct iso_tree_dir *iso_volume_get_root(const struct iso_volume *volume)
{
	return volume->root;
}
