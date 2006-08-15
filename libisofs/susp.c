/* vim: set noet ts=8 sts=8 sw=8 : */

#include "susp.h"
#include "tree.h"
#include "util.h"
#include "ecma119.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

static void susp_insert_direct(struct ecma119_write_target *t,
			       struct susp_info *susp, unsigned char *data,
			       int pos)
{
	int i;

	assert(pos <= susp->n_susp_fields);
	susp->n_susp_fields++;
	susp->susp_fields = realloc(susp->susp_fields,
				    sizeof(void*) * susp->n_susp_fields);

	for (i = susp->n_susp_fields-1; i > pos; i--) {
		susp->susp_fields[i] = susp->susp_fields[i - 1];
	}
	susp->susp_fields[pos] = data;
}

void susp_append(struct ecma119_write_target *t,
		 struct iso_tree_node *node, unsigned char *data)
{
	struct dir_write_info *inf = node->writer_data;
	struct susp_info *susp = &inf->susp;

	susp_insert_direct(t, susp, data, susp->n_susp_fields);
}

void susp_append_self(struct ecma119_write_target *t,
		      struct iso_tree_dir *dir, unsigned char *data)
{
	struct dir_write_info *inf = dir->writer_data;
	struct susp_info *susp = &inf->self_susp;

	susp_insert_direct(t, susp, data, susp->n_susp_fields);
}

void susp_append_parent(struct ecma119_write_target *t,
			struct iso_tree_dir *dir, unsigned char *data)
{
	struct dir_write_info *inf = dir->writer_data;
	struct susp_info *susp = &inf->parent_susp;

	susp_insert_direct(t, susp, data, susp->n_susp_fields);
}

void susp_insert(struct ecma119_write_target *t,
		 struct iso_tree_node *node, unsigned char *data, int pos)
{
	struct dir_write_info *inf = node->writer_data;
	struct susp_info *susp = &inf->susp;

	susp_insert_direct(t, susp, data, pos);
}

void susp_insert_self(struct ecma119_write_target *t,
		      struct iso_tree_dir *dir, unsigned char *data, int pos)
{
	struct dir_write_info *inf = dir->writer_data;
	struct susp_info *susp = &inf->self_susp;

	susp_insert_direct(t, susp, data, pos);
}

void susp_insert_parent(struct ecma119_write_target *t,
			struct iso_tree_dir *dir, unsigned char *data, int pos)
{
	struct dir_write_info *inf = dir->writer_data;
	struct susp_info *susp = &inf->parent_susp;

	susp_insert_direct(t, susp, data, pos);
}

unsigned char *susp_find(struct susp_info *susp, const char *name)
{
	int i;

	for (i = 0; i < susp->n_susp_fields; i++) {
		if (!strncmp((char *)susp->susp_fields[i], name, 2)) {
			return susp->susp_fields[i];
		}
	}
	return NULL;
}

/* utility function for susp_add_CE because susp_add_CE needs to act 3 times
 * on directories (for the "." and ".." entries. */
#define CE_LEN 28
static unsigned char *susp_add_single_CE(struct ecma119_write_target *t,
					 struct susp_info *susp, int len)
{
	int susp_length = 0, tmp_len;
	int i;
	unsigned char *CE;

	for (i = 0; i < susp->n_susp_fields; i++) {
		susp_length += susp->susp_fields[i][2];
	}
	if (susp_length <= len) {
		/* no need for a CE field */
		susp->non_CE_len = susp_length;
		susp->n_fields_fit = susp->n_susp_fields;
		return NULL;
	}

	tmp_len = susp_length;
	for (i = susp->n_susp_fields - 1; i >= 0; i--) {
		tmp_len -= susp->susp_fields[i][2];
		if (tmp_len + CE_LEN <= len) {
			susp->non_CE_len = tmp_len + CE_LEN;
			susp->CE_len = susp_length - tmp_len;

			/* i+1 because we have to count the CE field */
			susp->n_fields_fit = i + 1;

			CE = calloc(1, CE_LEN);
			/* we don't fill in the BLOCK LOCATION or OFFSET
			   fields yet. */
			CE[0] = 'C';
			CE[1] = 'E';
			CE[2] = (char)CE_LEN;
			CE[3] = (char)1;
			iso_bb(&CE[20], susp_length - tmp_len, 4);

			return CE;
		}
	}
	assert(0);
	return NULL;
}

/** See IEEE P1281 Draft Version 1.12/5.2. Because this function depends on the
 * length of the other SUSP fields, it should always be calculated last. */
void susp_add_CE(struct ecma119_write_target *t, struct iso_tree_node *node)
{
	struct dir_write_info *inf = node->writer_data;
	unsigned char *CE;

	CE = susp_add_single_CE(t, &inf->susp, 255 - node->dirent_len);
	if (CE)
		susp_insert(t, node, CE, inf->susp.n_fields_fit - 1);
	if (S_ISDIR(node->attrib.st_mode)) {
		CE = susp_add_single_CE(t, &inf->self_susp, 255 - 34);
		if (CE)
			susp_insert_self(t, (struct iso_tree_dir *)node, CE,
					 inf->self_susp.n_fields_fit - 1);
		CE = susp_add_single_CE(t, &inf->parent_susp, 255 - 34);
		if (CE)
			susp_insert_parent(t, (struct iso_tree_dir *)node, CE,
					   inf->parent_susp.n_fields_fit - 1);
	}
}

/** See IEEE P1281 Draft Version 1.12/5.3 */
void susp_add_SP(struct ecma119_write_target *t, struct iso_tree_dir *dir)
{
	unsigned char *SP = malloc(7);

	SP[0] = 'S';
	SP[1] = 'P';
	SP[2] = (char)7;
	SP[3] = (char)1;
	SP[4] = 0xbe;
	SP[5] = 0xef;
	SP[6] = 0;
	susp_append_self(t, dir, SP);
}

#if 0
/** See IEEE P1281 Draft Version 1.12/5.4 */
static void susp_add_ST(struct ecma119_write_target *t,
			struct iso_tree_node *node)
{
	unsigned char *ST = malloc(4);

	ST[0] = 'S';
	ST[1] = 'T';
	ST[2] = (char)4;
	ST[3] = (char)1;
	susp_append(t, node, ST);
}
#endif

/** See IEEE P1281 Draft Version 1.12/5.5 */
void susp_add_ER(struct ecma119_write_target *t, struct iso_tree_dir *dir)
{
	unsigned char *ER = malloc(182);

	ER[0] = 'E';
	ER[1] = 'R';
	ER[2] = 182;
	ER[3] = 1;
	ER[4] = 9;
	ER[5] = 72;
	ER[6] = 93;
	ER[7] = 1;
	memcpy(&ER[8], "IEEE_1282", 9);
	memcpy(&ER[17], "THE IEEE 1282 PROTOCOL PROVIDES SUPPORT FOR POSIX "
	       "FILE SYSTEM SEMANTICS.", 72);
	memcpy(&ER[89], "PLEASE CONTACT THE IEEE STANDARDS DEPARTMENT, "
	       "PISCATAWAY, NJ, USA FOR THE 1282 SPECIFICATION.", 93);
	susp_append_self(t, dir, ER);
}

static void susp_fin_CE(struct ecma119_write_target *t,
			struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = (struct dir_write_info *)
		dir->writer_data;
	struct node_write_info *cinf;
	unsigned char *CE;
	int i;
	int CE_offset = inf->len;

	if (inf->self_susp.CE_len) {
		CE = inf->self_susp.susp_fields[inf->self_susp.n_fields_fit -
						1];
		iso_bb(&CE[4], dir->block + CE_offset / 2048, 4);
		iso_bb(&CE[12], CE_offset % 2048, 4);
		CE_offset += inf->self_susp.CE_len;
	}
	if (inf->parent_susp.CE_len) {
		CE = inf->parent_susp.susp_fields[inf->parent_susp.
						  n_fields_fit - 1];
		iso_bb(&CE[4], dir->block + CE_offset / 2048, 4);
		iso_bb(&CE[12], CE_offset % 2048, 4);
		CE_offset += inf->parent_susp.CE_len;
	}

	for (i = 0; i < dir->nchildren; i++) {
		cinf = dir->children[i]->writer_data;
		if (!cinf->susp.CE_len) {
			continue;
		}
		CE = cinf->susp.susp_fields[cinf->susp.n_fields_fit - 1];
		iso_bb(&CE[4], dir->block + CE_offset / 2048, 4);
		iso_bb(&CE[12], CE_offset % 2048, 4);
		CE_offset += cinf->susp.CE_len;
	}
	for (i = 0; i < dir->nfiles; i++) {
		cinf = dir->files[i]->writer_data;
		if (!cinf->susp.CE_len) {
			continue;
		}
		CE = cinf->susp.susp_fields[cinf->susp.n_fields_fit - 1];
		iso_bb(&CE[4], dir->block + CE_offset / 2048, 4);
		iso_bb(&CE[12], CE_offset % 2048, 4);
		CE_offset += cinf->susp.CE_len;
	}
	assert(CE_offset == inf->len + inf->susp_len);
}

void susp_finalize(struct ecma119_write_target *t, struct iso_tree_dir *dir)
{
	int i;

	if (dir->depth != 1) {
		susp_fin_CE(t, dir);
	}

	for (i = 0; i < dir->nchildren; i++) {
		susp_finalize(t, dir->children[i]);
	}
}

void susp_write(struct ecma119_write_target *t, struct susp_info *susp,
		unsigned char *buf)
{
	int i;
	int pos = 0;

	for (i = 0; i < susp->n_fields_fit; i++) {
		memcpy(&buf[pos], susp->susp_fields[i],
		       susp->susp_fields[i][2]);
		pos += susp->susp_fields[i][2];
	}
}

void susp_write_CE(struct ecma119_write_target *t, struct susp_info *susp,
		   unsigned char *buf)
{
	int i;
	int pos = 0;

	for (i = susp->n_fields_fit; i < susp->n_susp_fields; i++) {
		memcpy(&buf[pos], susp->susp_fields[i],
		       susp->susp_fields[i][2]);
		pos += susp->susp_fields[i][2];
	}
}

void susp_free_fields(struct susp_info *susp)
{
	int i;

	for (i=0; i<susp->n_susp_fields; i++) {
		free(susp->susp_fields[i]);
	}
	if (susp->susp_fields) {
		free(susp->susp_fields);
	}
	memset(susp, 0, sizeof(struct susp_info));
}
