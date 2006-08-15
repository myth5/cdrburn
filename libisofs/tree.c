/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */
/* vim: set noet ts=8 sts=8 sw=8 : */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#include "libisofs.h"
#include "tree.h"
#include "util.h"
#include "volume.h"

struct iso_tree_dir *iso_tree_new_root(struct iso_volume *volume)
{
	struct iso_tree_dir *dir;

	assert(volume);

	dir = calloc(1, sizeof(struct iso_tree_dir));
	dir->attrib.st_mode = S_IFDIR;
	dir->volume = volume;

	return dir;
}

void iso_tree_free(struct iso_tree_dir *root)
{
	int i;

	assert(root);

	/* Free names. */
	free(root->name.full);
	free(root->name.iso1);
	free(root->name.iso2);
	free(root->name.rockridge);
	free(root->name.joliet);

	/* Free the children. */
	for (i = 0; i < root->nchildren; i++)
		iso_tree_free(root->children[i]);
	free(root->children);

	/* Free all files. */
	for (i = 0; i < root->nfiles; i++) {
		struct iso_tree_file *file = root->files[i];

		free(file->path);
		free(file->name.full);
		free(file->name.iso1);
		free(file->name.iso2);
		free(file->name.rockridge);
		free(file->name.joliet);
		free(file);
	}
	free(root->files);

	if (root->writer_data) {
		fprintf(stderr, "Warning: freeing a directory with non-NULL "
				"writer_data\n");
	}

	/* Free ourself. */
	free(root);
}

struct iso_tree_file *iso_tree_add_new_file(struct iso_tree_dir *parent,
					    const char *name)
{
	struct iso_tree_file *file;

	assert(parent && name);

	file = calloc(1, sizeof(struct iso_tree_file));
	file->path = calloc(1, 1);
	file->parent = parent;
	file->volume = parent->volume;
	file->attrib = parent->attrib;
	file->attrib.st_mode &= ~S_IFMT;
	file->attrib.st_mode |= S_IFREG;
	file->attrib.st_size = 0;
	iso_tree_file_set_name(file, name);

	/* Add the new file to the parent directory */
	parent->nfiles++;
	parent->files = realloc(parent->files, sizeof(void *) * parent->nfiles);
	parent->files[parent->nfiles - 1] = file;

	return file;
}

struct iso_tree_file *iso_tree_add_file(struct iso_tree_dir *parent,
					const char *path)
{
	struct iso_tree_file *file;
	struct stat st;
	int statret;
	const char *name;

	assert( parent != NULL && path != NULL );
	statret = lstat(path, &st);
	if (statret == -1) {
		fprintf(stderr, "couldn't stat file %s: %s\n",
				path, strerror(errno));
		return NULL;
	}

	/* Set up path, parent and volume. */
	file = calloc(1, sizeof(struct iso_tree_file));
	file->path = strdup(path);
	file->parent = parent;
	file->volume = parent->volume;
	file->attrib = st;

	/* find the last component in the path */
	name = strrchr(path, '/');
	if (name == NULL) {
		name = path;
	} else {
		name++;
	}
	iso_tree_file_set_name(file, name);

	if (!S_ISREG(st.st_mode)) {
		file->attrib.st_size = 0;
	}

	/* Add the new file to the parent directory */
	parent->nfiles++;
	parent->files = realloc(parent->files, sizeof(void *) * parent->nfiles);
	parent->files[parent->nfiles - 1] = file;

	return file;
}

struct iso_tree_dir *iso_tree_add_dir(struct iso_tree_dir *parent,
				      const char *path)
{
	struct iso_tree_dir *dir;
	struct stat st;
	int statret;
	char *pathcpy;
	char *name;

	assert( parent && path );
	statret = stat(path, &st);
	if (statret == -1) {
		fprintf(stderr, "couldn't stat directory %s: %s\n",
				path, strerror(errno));
		return NULL;
	}

	dir = calloc(1, sizeof(struct iso_tree_dir));
	dir->parent = parent;
	dir->volume = parent->volume;
	dir->attrib = st;

	/* find the last component in the path. We need to copy the path because
	 * we modify it if there is a trailing slash. */
	pathcpy = strdup(path);
	name = strrchr(pathcpy, '/');
	if (name == &pathcpy[strlen(pathcpy) - 1]) {
		/* get rid of the trailing slash */
		*name = '\0';
		name = strrchr(pathcpy, '/');
	}
	if (name == NULL) {
		name = pathcpy;
	} else {
		name++;
	}
	iso_tree_dir_set_name(dir, name);
	free(pathcpy);


	parent->nchildren++;
	parent->children = realloc(parent->children,
				   parent->nchildren * sizeof(void*));
	parent->children[parent->nchildren - 1] = dir;

	return dir;
}

struct iso_tree_dir *iso_tree_radd_dir(struct iso_tree_dir *parent,
				       const char *path)
{
	struct iso_tree_dir *nparent;
	struct stat st;
	int statret;
	DIR *dir;
	struct dirent *ent;

	assert( parent && path );
	statret = stat(path, &st);

	nparent = iso_tree_add_dir(parent, path);

	/* Open the directory for reading and iterate over the directory
	   entries. */
	dir = opendir(path);

	if (!dir) {
		fprintf(stderr, "couldn't open directory %s: %s\n",
				path, strerror(errno));
		return NULL;
	}

	while ((ent = readdir(dir))) {
		char *child;

		/* Skip current and parent directory entries. */
		if (strcmp(ent->d_name, ".") == 0 ||
		    strcmp(ent->d_name, "..") == 0)
			continue;

		/* Build the child's full pathname. */
		child = iso_pathname(path, ent->d_name);

		/* Skip to the next entry on errors. */
		if (stat(child, &st) != 0)
			continue;

		if (S_ISDIR(st.st_mode)) {
			iso_tree_radd_dir(nparent, child);
		} else {
			iso_tree_add_file(nparent, child);
		}

		free(child);
	}

	closedir(dir);

	return nparent;
}

struct iso_tree_dir *iso_tree_add_new_dir(struct iso_tree_dir *parent,
					  const char *name)
{
	struct iso_tree_dir *dir;

	assert( parent && name );

	dir = calloc(1, sizeof(struct iso_tree_dir));
	dir->parent = parent;
	dir->volume = parent->volume;

	iso_tree_dir_set_name(dir, name);

	dir->attrib = parent->attrib;
	dir->attrib.st_mtime = time(NULL);
	dir->attrib.st_atime = time(NULL);
	dir->attrib.st_ctime = time(NULL);


	/* add the new directory to parent */
	parent->nchildren++;
	parent->children = realloc(parent->children,
				   parent->nchildren * sizeof(void*));
	parent->children[parent->nchildren - 1] = dir;

	return dir;
}

void iso_tree_file_set_name(struct iso_tree_file *file, const char *name)
{
	file->name.full = strdup(name);
	file->name.iso1 = iso_1_fileid(name);
	file->name.iso2 = iso_2_fileid(name);
	file->name.rockridge = iso_p_filename(name);
	file->name.joliet = iso_j_id(file->name.full);
}

char *iso_tree_node_get_name_nconst(const struct iso_tree_node *node,
				    enum iso_name_version ver)
{
	if (ver == ISO_NAME_ISO) {
		if (node->volume->iso_level == 1) {
			return node->name.iso1;
		} else {
			return node->name.iso2;
		}
	}

	switch (ver) {
	case ISO_NAME_FULL:
		return node->name.full;
	case ISO_NAME_ISO:
		if (node->volume->iso_level == 1) {
			return node->name.iso1;
		} else {
			return node->name.iso2;
		}
	case ISO_NAME_ISO_L1:
		return node->name.iso1;
	case ISO_NAME_ISO_L2:
		return node->name.iso2;
	case ISO_NAME_ROCKRIDGE:
		return node->name.rockridge;
	case ISO_NAME_JOLIET:
		return (char*) node->name.joliet;
	}

	assert(0);
	return NULL; /* just to shut up warnings */
}

const char *iso_tree_node_get_name(const struct iso_tree_node *node,
				   enum iso_name_version ver)
{
	return iso_tree_node_get_name_nconst(node, ver);
}

void iso_tree_dir_set_name(struct iso_tree_dir *dir, const char *name)
{
	dir->name.full = strdup(name);
	/* Level 1 directory is a string of d-characters of maximum size 8. */
	dir->name.iso1 = iso_d_str(name, 8, 0);
	/* Level 2 directory is a string of d-characters of maximum size 31. */
	dir->name.iso2 = iso_d_str(name, 31, 0);
	dir->name.rockridge = iso_p_dirname(name);
	dir->name.joliet = iso_j_id(dir->name.full);
}

/* Compares file names for use with qsort. */
int iso_node_cmp(const void *v1, const void *v2)
{
	struct iso_tree_node **f1 = (struct iso_tree_node **)v1;
	struct iso_tree_node **f2 = (struct iso_tree_node **)v2;

	return strcmp(iso_tree_node_get_name(*f1, ISO_NAME_FULL),
		      iso_tree_node_get_name(*f2, ISO_NAME_FULL));
}

int iso_node_cmp_iso(const void *v1, const void *v2)
{
	struct iso_tree_node **f1 = (struct iso_tree_node **)v1;
	struct iso_tree_node **f2 = (struct iso_tree_node **)v2;

	return strcmp(iso_tree_node_get_name(*f1, ISO_NAME_ISO),
		      iso_tree_node_get_name(*f2, ISO_NAME_ISO));
}

int iso_node_cmp_joliet(const void *v1, const void *v2)
{
	struct iso_tree_node **f1 = (struct iso_tree_node **)v1;
	struct iso_tree_node **f2 = (struct iso_tree_node **)v2;

	return ucscmp((uint16_t*)iso_tree_node_get_name(*f1, ISO_NAME_JOLIET),
		      (uint16_t*)iso_tree_node_get_name(*f2, ISO_NAME_JOLIET));
}

void iso_tree_sort(struct iso_tree_dir *root)
{
	int i;

	qsort(root->files, root->nfiles, sizeof(struct iso_tree_file *),
	      iso_node_cmp);

	qsort(root->children, root->nchildren, sizeof(struct iso_tree_dir *),
	      iso_node_cmp);

	for (i = 0; i < root->nchildren; i++)
		iso_tree_sort(root->children[i]);
}

void iso_tree_print(const struct iso_tree_dir *root, int spaces)
{
	iso_tree_print_verbose(root, NULL, NULL, NULL, spaces);
}

void iso_tree_print_verbose(const struct iso_tree_dir *root,
			    print_dir_callback dc, print_file_callback fc,
			    void *data, int spaces)
{
	int i, j;

	for (i = 0; i < spaces; i++)
		printf(" ");

	/* Root directory doesn't have a name. */
	if (root->name.full != NULL)
		printf("%s", iso_tree_node_get_name(ISO_NODE(root),
						    ISO_NAME_ISO));
	printf("/\n");
	if (dc)	dc(root, data, spaces);

	spaces += 2;

	for (j = 0; j < root->nchildren; j++) {
		iso_tree_print_verbose(root->children[j], dc, fc, data,
				       spaces);
	}

	for (j = 0; j < root->nfiles; j++) {
		for (i = 0; i < spaces; i++)
			printf(" ");
		printf("%s\n",
		       iso_tree_node_get_name(ISO_NODE(root->files[j]),
					      ISO_NAME_ISO));
		if (fc) fc(root->files[j], data, spaces);
	}
}
