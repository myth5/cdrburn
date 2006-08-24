/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set ts=8 sts=8 sw=8 noet : */

#include <stdlib.h>
#include <string.h>

#include "libisofs.h"
#include "tree.h"
#include "util.h"
#include "volume.h"

struct iso_volset*
iso_volset_new(struct iso_volume *vol, const char *id)
{
	struct iso_volset *volset = calloc(1, sizeof(struct iso_volset));

	volset->volset_size = 1;
	volset->refcount = 1;
	volset->volume = malloc(sizeof(void *));
	volset->volume[0] = vol;
	volset->volset_id = towcs(id);

	vol->refcount++;
	return volset;
}

void
iso_volset_free(struct iso_volset *volset)
{
	if (--volset->refcount < 1) {
		int i;
		for (i = 0; i < volset->volset_size; i++) {
			iso_volume_free(volset->volume[i]);
		}
		free(volset->volume);
		free(volset->volset_id);
	}
}

struct iso_volume*
iso_volume_new(const char *volume_id,
	       const char *publisher_id,
	       const char *data_preparer_id)
{
	return iso_volume_new_with_root(volume_id,
					publisher_id,
					data_preparer_id,
					NULL);
}

struct iso_volume*
iso_volume_new_with_root(const char *volume_id,
			 const char *publisher_id,
			 const char *data_preparer_id,
			 struct iso_tree_node *root)
{
	struct iso_volume *volume;

	volume = calloc(1, sizeof(struct iso_volume));
	volume->refcount = 1;

	volume->root = root ? root : iso_tree_new_root(volume);

	if (volume_id != NULL)
		volume->volume_id = towcs(volume_id);
	if (publisher_id != NULL)
		volume->publisher_id = towcs(publisher_id);
	if (data_preparer_id != NULL)
		volume->data_preparer_id = towcs(data_preparer_id);
	return volume;
}

void iso_volume_free(struct iso_volume *volume)
{
	/* Only free if no references are in use. */
	if (--volume->refcount < 1) {
		iso_tree_free(volume->root);

		free(volume->volume_id);
		free(volume->publisher_id);
		free(volume->data_preparer_id);

		free(volume);
	}
}

struct iso_tree_node *iso_volume_get_root(const struct iso_volume *volume)
{
	return volume->root;
}
