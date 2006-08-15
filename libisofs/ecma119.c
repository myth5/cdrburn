/* vim: set noet ts=8 sts=8 sw=8 : */

#include "tree.h"
#include "ecma119.h"
#include "volume.h"
#include "util.h"
#include "struct.h"
#include "rockridge.h"
#include "libisofs.h"
#include "libburn/libburn.h"
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>

const char* const ecma119_standard_id = "CD001";

/* Format definitions */
const char* const ecma119_vol_fmt = "B5bB"; /* common between all vol descs */

const char* const ecma119_privol_fmt =
	"B"	/* volume desc type */
	"5b"	/* standard id */
	"B"	/* volume desc version */
	"x"
	"32b"	/* system id */
	"32b"	/* volume id */
	"8x"
	"=L"	/* volume space size */
	"32x"
	"=H"	/* volume set size */
	"=H"	/* volume sequence number */
	"=H"	/* block size */
	"=L"	/* path table size */
	"<L"	/* l path table 1 */
	"<L"	/* l path table 2 */
	">L"	/* m path table 1 */
	">L"	/* m path table 2 */
	"34B"	/* root directory record */
	"128b"	/* volume id */
	"128b"	/* publisher id */
	"128b"	/* data preparer id */
	"128b"	/* application id */
	"37b"	/* copyright file id */
	"37b"	/* abstract file id */
	"37b"	/* bibliographic file id */
	"T"	/* creation timestamp */
	"T"	/* modification timestamp */
	"T"	/* expiration timestamp */
	"T"	/* effective timestamp */
	"B"	/* file structure version */
	"x"
	"512x"	/* Application Use */
	"653x";	/* reserved */

const char* const ecma119_supvol_joliet_fmt =
	"B"	/* volume desc type */
	"5b"	/* standard id */
	"B"	/* volume desc version */
	"B"	/* volume flags */
	"16h"	/* system id */
	"16h"	/* volume id */
	"8x"
	"=L"	/* volume space size */
	"32b"	/* escape sequences */
	"=H"	/* volume set size */
	"=H"	/* volume sequence number */
	"=H"	/* block size */
	"=L"	/* path table size */
	"<L"	/* l path table 1 */
	"<L"	/* l path table 2 */
	">L"	/* m path table 1 */
	">L"	/* m path table 2 */
	"34B"	/* root directory record */
	"64h"	/* volume id */
	"64h"	/* publisher id */
	"64h"	/* data preparer id */
	"64h"	/* application id */
	"18hx"	/* copyright file id */
	"18hx"	/* abstract file id */
	"18hx"	/* bibliographic file id */
	"T"	/* creation timestamp */
	"T"	/* modification timestamp */
	"T"	/* expiration timestamp */
	"T"	/* effective timestamp */
	"B"	/* file structure version */
	"x"
	"512x"	/* Application Use */
	"653x";	/* reserved */

const char* const ecma119_dir_record_fmt =
	"B"	/* dir record length */
	"B"	/* extended attribute length */
	"=L"	/* block */
	"=L"	/* size */
	"S"
	"B"	/* file flags */
	"B"	/* file unit size */
	"B"	/* interleave gap size */
	"=H"	/* volume sequence number */
	"B";	/* name length */
	/* file id ansa SU field have variable length, so they don't get defined
	 * yet. */

/* abstract string functions away from char* to make Joliet stuff easier */
typedef void (*copy_string)(unsigned char *dest, void *str, int maxlen);
typedef int (*node_namelen)(struct iso_tree_node *node);
typedef const void* (*node_getname)(struct iso_tree_node *node);

/* because we don't write SUSP fields in the Joliet Directory Records, it helps
 * to abstract away the task of getting the non_CE_len of a susp_info.
 */
typedef int (*susp_len)(struct susp_info*);
typedef int (*total_dirent_len)(struct iso_tree_node*);

const char application_id[] = "LIBBURN SUITE (C) 2002 D.FOREMAN/B.JANSENS";
const char system_id[] = "LINUX";

const uint16_t* application_id_joliet = (uint16_t*) "\0j\0a\0p\0p\0i\0d\0\0";
const uint16_t* system_id_joliet = (uint16_t*) "\0j\0s\0y\0s\0i\0d\0\0";

/* since we have to pass things by address to iso_struct_pack, this makes
 * it easier. */
const uint8_t zero = 0;
const uint8_t one = 1;

enum RecordType {
	RECORD_TYPE_SELF,
	RECORD_TYPE_PARENT,
	RECORD_TYPE_NORMAL,
	RECORD_TYPE_ROOT
};

/* layout functions */
static void ecma119_reorganize_heirarchy(struct ecma119_write_target *target,
					 struct iso_tree_dir *dir);
static void ecma119_alloc_writer_data(struct ecma119_write_target *target,
				      struct iso_tree_dir *dir);
static void ecma119_setup_path_tables_iso(struct ecma119_write_target*);
static void ecma119_setup_path_tables_joliet(struct ecma119_write_target*);

/* burn_source functions */
static int ecma119_read(struct burn_source*, unsigned char*, int);
static int ecma119_get_size(struct burn_source*);
static void ecma119_free_data(struct burn_source*);

/* Writers for the different write_states. */
static void ecma119_write_system_area(struct ecma119_write_target*,
					unsigned char *buf);
static void ecma119_write_privol_desc(struct ecma119_write_target*,
				      unsigned char *buf);
static void ecma119_write_supvol_desc_joliet(struct ecma119_write_target*,
					     unsigned char *buf);
static void ecma119_write_vol_desc_terminator(struct ecma119_write_target*,
					unsigned char *buf);
static void ecma119_write_path_table(struct ecma119_write_target*,
					unsigned char *buf,
					int flags,
					int m_type);
static void ecma119_write_dir_records(struct ecma119_write_target*,
					unsigned char *buf,
					int flags);
static void ecma119_write_files(struct ecma119_write_target*,
					unsigned char *buf);

/* helpers for the writers */
static void ecma119_write_path_table_full(struct ecma119_write_target*,
					unsigned char *buf,
					int flags,
					int mtype);
static unsigned char *ecma119_write_dir(struct ecma119_write_target*,
					int flags,
					struct iso_tree_dir*);
static void ecma119_write_dir_record(struct ecma119_write_target*,
					unsigned char *,
					int flags,
					struct iso_tree_node*,
					enum RecordType);
static void ecma119_write_dir_record_iso(struct ecma119_write_target *t,
					 uint8_t *buf,
					 struct iso_tree_node *node,
					 enum RecordType type);
static void ecma119_write_dir_record_joliet(struct ecma119_write_target *t,
					    uint8_t *buf,
					    struct iso_tree_node *node,
					    enum RecordType type);
static void ecma119_write_dir_record_noname(struct ecma119_write_target *t,
					    uint8_t *buf,
					    struct iso_tree_node *node,
					    enum RecordType type,
					    uint32_t block,
					    uint32_t size,
					    int joliet);


/* name-mangling routines */

/* return a newly allocated array of pointers to all the children, in
 * in alphabetical order and with mangled names. */
static struct iso_tree_node **ecma119_mangle_names(struct iso_tree_dir *dir);

/* mangle a single filename according to where the last '.' is:
 * \param name the name to mangle
 * \param num_change the number of characters to mangle
 * \param seq_num the string ("%0<num_change>d", seq_num) to which it should
 *	be mangled
 */
static void ecma119_mangle_name(char *name,
				int num_change,
				int seq_num);

/* mangle a single filename based on the explicit positions passed. That is,
 * the difference between this and ecma119_mangle_name_iso is that this one
 * doesn't look for the '.' character; it changes the character at the
 * beginning of name. */
static void ecma119_mangle_name_priv(char *name,
				     int num_change,
				     int seq_num);

/* return a newly allocated array of pointers to all the children, in
 * alphabetical order. The difference between this and ecma119_mangle_names
 * is that this function sorts the files according to joliet names and it
 * doesn't mangle the names. */
static struct iso_tree_node **ecma119_sort_joliet(struct iso_tree_dir *dir);

/* string abstraction functions */
static int node_namelen_iso(struct iso_tree_node *node)
{
	return strlen(iso_tree_node_get_name(node, ISO_NAME_ISO));
}

static int node_namelen_joliet(struct iso_tree_node *node)
{
	const char *name = iso_tree_node_get_name(node, ISO_NAME_JOLIET);

	/* return length in bytes, not length in characters */
	return ucslen((uint16_t*)name) * 2;
}

static const void *node_getname_iso(struct iso_tree_node *node)
{
	return iso_tree_node_get_name(node, ISO_NAME_ISO);
}

static const void *node_getname_joliet(struct iso_tree_node *node)
{
	return iso_tree_node_get_name(node, ISO_NAME_JOLIET);
}

static int susp_len_iso(struct susp_info *s)
{
	return s->non_CE_len;
}

static int susp_len_joliet(struct susp_info *s)
{
	return 0;
}

static int dirent_len_iso(struct iso_tree_node *n)
{
	return n->dirent_len + GET_NODE_INF(n)->susp.non_CE_len;
}

static int dirent_len_joliet(struct iso_tree_node *n)
{
	return 34 + node_namelen_joliet(n);
}

static void print_dir_info(struct iso_tree_dir *dir,
			   struct ecma119_write_target *t,
			   int spaces)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	int i;

	for (i=0; i<spaces; i++) {
		putchar(' ');
	}

	printf("non_CE_len=%d, CE_len=%d, self->non_CE_len=%d, "
	       "parent->non_CE_len=%d, len=%d, blk=%d, jblk=%d\n",
	       inf->susp.non_CE_len,
	       inf->susp.CE_len,
	       inf->self_susp.non_CE_len,
	       inf->parent_susp.non_CE_len,
	       inf->len,
	       (int)dir->block,
	       (int)inf->joliet_block);
}

static void print_file_info(struct iso_tree_file *file,
			    struct ecma119_write_target *t,
			    int spaces)
{
	struct file_write_info *inf = GET_FILE_INF(file);
	int i;

	for (i=0; i<spaces; i++) {
		putchar(' ');
	}
	printf("dirent size=%d, non_CE_len=%d, CE_len=%d, len=%d, blk=%d\n",
		(int)file->dirent_len,
		inf->susp.non_CE_len,
		inf->susp.CE_len,
		(int)file->attrib.st_size,
		(int)file->block);
}

static struct iso_tree_node **ecma119_mangle_names(struct iso_tree_dir *dir)
{
	size_t retsize = dir->nfiles + dir->nchildren;
	struct iso_tree_node **ret = calloc(1, sizeof(void*) * retsize);
	int i, j, k;

	j = 0;
	for (i=0; i<dir->nfiles; i++) {
		ret[j++] = ISO_NODE(dir->files[i]);
	}
	for (i=0; i<dir->nchildren; i++) {
		ret[j++] = ISO_NODE(dir->children[i]);
	}

	qsort(ret, retsize, sizeof(void*), iso_node_cmp_iso);

	for (i=0; i<retsize; i++) {
		int n_change;
		char *name;

		/* find the number of consecutive equal names */
		j = 1;
		while (i+j < retsize && !iso_node_cmp_iso(&ret[i], &ret[i+j]))
			j++;

		if (j == 1) continue;

		/* mangle the names */
		n_change = j / 10 + 1;
		for (k=0; k<j; k++) {
			name = iso_tree_node_get_name_nconst(ret[i+k],
							     ISO_NAME_ISO);
			ecma119_mangle_name(name, n_change, k);
		}

		/* skip ahead by the number of mangled names */
		i += j - 1;
	}

	return ret;
}

static void ecma119_mangle_name(char *name,
				int num_change,
				int seq_num)
{
	char *dot = strrchr(name, '.');
	int dot_pos;
	int len = strlen(name);
	int ext_len;

	if (!dot) dot = name + len;
	dot_pos = dot - name;
	ext_len = len - dot_pos;
	if (dot_pos < num_change) {
		/* we need to shift part of the mangling into the extension */
		int digits=0, i;
		int n_ext;
		int nleft = num_change - ext_len;
		char *pos = name + len - num_change;

		pos = MAX(pos, dot+1);
		n_ext = name + len - pos;

		/* digits = 10^ext_len */
		for (i=0; i<ext_len; i++) {
			digits *= 10;
		}
		ecma119_mangle_name_priv(pos, n_ext, seq_num % digits);
		ecma119_mangle_name_priv(dot-nleft, nleft, seq_num / digits);
	} else {
		ecma119_mangle_name_priv(dot-num_change, num_change, seq_num);
	}
}

static void ecma119_mangle_name_priv(char *name,
				     int num_change,
				     int seq_num)
{
	char fmt[5];
	char overwritten;

	if (num_change <= 0 || num_change >= 10) {
		return;
	}

	overwritten = name[num_change];
	sprintf(fmt, "%%0%1dd", num_change);
	sprintf(name, fmt, seq_num);
	name[num_change] = overwritten;
}

static struct iso_tree_node **ecma119_sort_joliet(struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	size_t retsize = dir->nfiles + inf->real_nchildren;
	struct iso_tree_node **ret = malloc(retsize * sizeof(void*));
	int i, j;

	j = 0;
	for (i=0; i<dir->nfiles; i++) {
		ret[j++] = ISO_NODE(dir->files[i]);
	}
	for (i=0; i<inf->real_nchildren; i++) {
		ret[j++] = ISO_NODE(inf->real_children[i]);
	}
	qsort(ret, retsize, sizeof(void*), iso_node_cmp_joliet);

	return ret;
}

struct ecma119_write_target *ecma119_target_new(struct iso_volset *volset,
						int volnum)
{
	struct ecma119_write_target *t;

	assert(volnum < volset->volset_size);

	t = calloc(1, sizeof(struct ecma119_write_target));
	t->volset = volset;
	t->volnum = volnum;
	t->now = time(NULL);
	t->block_size = 2048;

	iso_tree_sort(TARGET_ROOT(t));
	ecma119_alloc_writer_data(t, TARGET_ROOT(t));
	ecma119_reorganize_heirarchy(t, TARGET_ROOT(t));

	return t;
}

/**
 * Write create all necessary SUSP fields for the given directory and
 * initialise them. Any SUSP fields that require an offset won't be completed
 * yet. Ensure that the size fields in the susp_info structs are correct.
 */
static void ecma119_susp_dir_layout(struct ecma119_write_target *target,
				    struct iso_tree_dir *dir,
				    int flags)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	struct susp_info *susp = &inf->susp;
	susp->n_susp_fields = 0;
	susp->susp_fields = NULL;

	if (!target->rockridge) {
		/* since Rock Ridge is the only SUSP extension supported,
		 * no need to continue. */
		return;
	}

	if (dir->depth == 1) {
		susp_add_SP(target, dir);
		susp_add_ER(target, dir);
	} else {
		rrip_add_NM(target, ISO_NODE(dir));
		rrip_add_TF(target, ISO_NODE(dir));
	}
	rrip_add_PX_dir(target, dir);

	if (inf->real_parent != dir->parent) {
		rrip_add_RE(target, ISO_NODE(dir));
		rrip_add_PL(target, dir);
	}
	susp_add_CE(target, ISO_NODE(dir));
}

/**
 * Write create all necessary SUSP fields for the given file and
 * initialise them. Any SUSP fields that require an offset won't be completed
 * yet. Ensure that the size fields in the susp_info structs are correct.
 */
static void ecma119_susp_file_layout(struct ecma119_write_target *target,
				     struct iso_tree_file *file)
{
	struct file_write_info *inf = GET_FILE_INF(file);

	if (!target->rockridge) {
		return;
	}

	rrip_add_PX(target, ISO_NODE(file));
	rrip_add_NM(target, ISO_NODE(file));
	rrip_add_TF(target, ISO_NODE(file));

	if (inf->real_me) {
		rrip_add_CL(target, ISO_NODE(file));
	}
	if (S_ISLNK(file->attrib.st_mode)) {
		rrip_add_SL(target, ISO_NODE(file));
	}
	if (S_ISCHR(file->attrib.st_mode) || S_ISBLK(file->attrib.st_mode)) {
		rrip_add_PN(target, ISO_NODE(file));
	}
	susp_add_CE(target, ISO_NODE(file));
}

/**
 * Recursively allocate the writer_data pointer for each node in the tree.
 * Also, save the current state of the tree in the inf->real_XXX pointers.
 */
static void ecma119_alloc_writer_data(struct ecma119_write_target *target,
				      struct iso_tree_dir *dir)
{
	struct dir_write_info *inf;
	int i;

	inf = calloc(1, sizeof(struct dir_write_info));
	inf->real_parent = dir->parent;
	inf->real_nchildren = dir->nchildren;
	inf->real_children = malloc(sizeof(void*) * dir->nchildren);
	memcpy(inf->real_children, dir->children, dir->nchildren*sizeof(void*));
	inf->real_depth = dir->depth;

	dir->writer_data = inf;

	for (i=0; i<dir->nchildren; i++) {
		ecma119_alloc_writer_data(target, dir->children[i]);
	}
	for (i=0; i<dir->nfiles; i++) {
		dir->files[i]->writer_data =
			calloc(1, sizeof(struct file_write_info));
	}
}

/* ensure that the maximum height of the directory tree is 8, repositioning
 * directories as necessary */
static void ecma119_reorganize_heirarchy(struct ecma119_write_target *target,
					 struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	struct iso_tree_dir *root = TARGET_ROOT(target);
	struct iso_tree_file *file;
	struct file_write_info *finf;

	/* save this now in case a recursive call modifies this value */
	int nchildren = dir->nchildren;
	int i;

	if (dir == root) {
		dir->depth = 1;
	} else {
		dir->depth = dir->parent->depth + 1;

		assert(dir->depth <= 9);
		if (dir->depth == 9) {
			dir->depth = 2;
			dir->parent = root;
			root->nchildren++;
			root->children = realloc(root->children,
					root->nchildren * sizeof(void*));
			root->children[root->nchildren-1] = dir;

			/* no need to reshuffle the siblings since we know that
			 * _every_ sibling will also be relocated. */
			inf->real_parent->nchildren--;
			if (!inf->real_parent->nchildren) {
				free(inf->real_parent->children);
				inf->real_parent->children = NULL;
			}
			/* insert a placeholder file for the CL field */
			file = iso_tree_add_new_file(inf->real_parent,
						     dir->name.full);
			finf = calloc(1, sizeof(struct file_write_info));
			finf->real_me = dir;
			file->writer_data = finf;
		}
	}
	for (i=0; i<nchildren; i++) {
		ecma119_reorganize_heirarchy(target, dir->children[i]);
	}
}

/* set directory sizes recursively. Also fill out the dirlist_len and
 * filelist_len fields in the ecma119 writer.
 */
static void ecma119_target_rsize(struct ecma119_write_target *t,
				 struct iso_tree_dir *dir)
{
	int i;
	struct dir_write_info *inf = GET_DIR_INF(dir);
	struct node_write_info *cinf;

	t->dirlist_len++;

	/* work out the size of the dirents */
	if (dir->depth == 1) {
		dir->dirent_len = 34;
	} else {
		dir->dirent_len = 33 + node_namelen_iso(ISO_NODE(dir));
	}
	dir->dirent_len += dir->dirent_len % 2;

	for (i=0; i<dir->nfiles; i++) {
		dir->files[i]->dirent_len = 33 + node_namelen_iso(
						ISO_NODE(dir->files[i]));
		dir->files[i]->dirent_len += dir->files[i]->dirent_len % 2;

		if (dir->files[i]->path && dir->files[i]->attrib.st_size) {
			t->filelist_len++;
		}
	}

	/* layout all the susp entries and calculate the total size */
	ecma119_susp_dir_layout(t, dir, 0);

	inf->len = 34 + inf->self_susp.non_CE_len /* for "." and ".." */
		 + 34 + inf->parent_susp.non_CE_len;
	inf->susp_len = inf->susp.CE_len
		      + inf->self_susp.CE_len
		      + inf->parent_susp.CE_len;

	for (i=0; i<dir->nfiles; i++) {
		ecma119_susp_file_layout(t, dir->files[i]);
		cinf = GET_NODE_INF(dir->files[i]);
		inf->len += dir->files[i]->dirent_len + cinf->susp.non_CE_len;
		inf->susp_len += cinf->susp.CE_len;
	}
	for (i=0; i<dir->nchildren; i++) {
		ecma119_target_rsize(t, dir->children[i]);
		cinf = GET_NODE_INF(dir->children[i]);
		inf->len += dir->children[i]->dirent_len +cinf->susp.non_CE_len;
		inf->susp_len += cinf->susp.CE_len;
	}
	dir->attrib.st_size = inf->len; /* the actual size of the data is
					 * inf->len + inf->susp_len because we
					 * append the CE data to the end of the
					 * directory. But the ISO volume
					 * doesn't need to know.
					 */
}

static void ecma119_target_rsize_joliet(struct ecma119_write_target *t,
					struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	int i;

	inf->joliet_len = 34 + 34; /* for "." and ".." */
	for (i=0; i<dir->nfiles; i++) {
		/* don't count files that are placeholders for Rock Ridge
		 * relocated directories */
		if (!GET_FILE_INF(dir->files[i])->real_me) {
			inf->joliet_len +=
				dirent_len_joliet(ISO_NODE(dir->files[i]));
		}
	}
	for (i=0; i<inf->real_nchildren; i++) {
		struct iso_tree_node *ch = ISO_NODE(inf->real_children[i]);
		inf->joliet_len += dirent_len_joliet(ch);
		ecma119_target_rsize_joliet(t, inf->real_children[i]);
	}
}

/* set directory positions recursively. Also fill out the dirlist in the
 * ecma119_write_target */
static void ecma119_target_dir_layout(struct ecma119_write_target *t,
				      struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	int i;

	t->dirlist[t->curfile++] = dir;
	dir->block = t->curblock;
	t->curblock += DIV_UP(inf->len + inf->susp_len, 2048);

	for (i=0; i<dir->nchildren; i++) {
		ecma119_target_dir_layout(t, dir->children[i]);
	}
}

/* same as ecma119_target_dir_layout, but for Joliet. */
static void ecma119_target_dir_layout_joliet(struct ecma119_write_target *t,
					     struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	int i;

	t->dirlist_joliet[t->curfile++] = dir;
	inf->joliet_block = t->curblock;
	t->curblock += DIV_UP(inf->joliet_len, 2048);

	for (i=0; i<inf->real_nchildren; i++) {
		ecma119_target_dir_layout_joliet(t, inf->real_children[i]);
	}
}

/* set file positions recursively. Also fill in the filelist in the
 * ecma119_write_target */
static void ecma119_target_file_layout(struct ecma119_write_target *t,
				       struct iso_tree_dir *dir)
{
	int i;

	for (i=0; i<dir->nfiles; i++) {
		if (dir->files[i]->path && dir->files[i]->attrib.st_size) {
			t->filelist[t->curfile++] = dir->files[i];
		}
		dir->files[i]->block = t->curblock;
		t->curblock += DIV_UP(dir->files[i]->attrib.st_size, 2048);
	}
	for (i=0; i<dir->nchildren; i++) {
		ecma119_target_file_layout(t, dir->children[i]);
	}
}

void ecma119_target_layout(struct ecma119_write_target *t)
{
	ecma119_target_rsize(t, TARGET_ROOT(t));
	if (t->joliet) {
		ecma119_target_rsize_joliet(t, TARGET_ROOT(t));
	}
	ecma119_setup_path_tables_iso(t);
	t->curblock = 16 /* for the system area */
		+ 1  /* volume desc */
		+ 1; /* volume desc terminator */
	if (t->joliet) {
		t->curblock++; /* joliet supplementary volume desc */
	}

	t->l_path_table_pos = t->curblock;
	t->curblock += DIV_UP(t->path_table_size, 2048);
	t->m_path_table_pos = t->curblock;
	t->curblock += DIV_UP(t->path_table_size, 2048);

	if (t->joliet) {
		ecma119_setup_path_tables_joliet(t);
		t->l_path_table_pos_joliet = t->curblock;
		t->curblock += DIV_UP(t->path_table_size_joliet, 2048);
		t->m_path_table_pos_joliet = t->curblock;
		t->curblock += DIV_UP(t->path_table_size_joliet, 2048);
	}

	t->dirlist = calloc(1, sizeof(void*) * t->dirlist_len);
	t->filelist = calloc(1, sizeof(void*) * t->filelist_len);
	t->curfile = 0;
	ecma119_target_dir_layout(t, TARGET_ROOT(t));

	if (t->joliet) {
		t->curfile = 0;
		t->dirlist_joliet = calloc(1, sizeof(void*) * t->dirlist_len);
		ecma119_target_dir_layout_joliet(t, TARGET_ROOT(t));
	}

	t->curfile = 0;
	ecma119_target_file_layout(t, TARGET_ROOT(t));
	t->total_size = t->curblock * 2048;
	t->vol_space_size = t->curblock;

	if (t->rockridge) {
		susp_finalize(t, TARGET_ROOT(t));
		rrip_finalize(t, TARGET_ROOT(t));
	}

	iso_tree_print_verbose(TARGET_ROOT(t),
			       (print_dir_callback)print_dir_info,
			       (print_file_callback)print_file_info,
			       t, 0);

	/* prepare for writing */
	t->curblock = -1;
	t->state = ECMA119_WRITE_SYSTEM_AREA;
}

struct burn_source *iso_source_new_ecma119(struct iso_volset *volset,
					   int volnum,
					   int level,
					   int flags)
{
	struct burn_source *src = calloc(1, sizeof(struct burn_source));
	struct ecma119_write_target *t = ecma119_target_new(volset, volnum);
	struct iso_volume *vol = volset->volume[volnum];

	t->iso_level = level;
	t->rockridge = (flags & ECMA119_ROCKRIDGE) ? 1:0;
	t->joliet = (flags & ECMA119_JOLIET) ? 1:0;

	vol->iso_level = t->iso_level;
	vol->rockridge = t->rockridge;
	vol->joliet = t->joliet;

	ecma119_target_layout(t);
	src->read = ecma119_read;
	src->read_sub = NULL;
	src->get_size = ecma119_get_size;
	src->free_data = ecma119_free_data;
	src->data = t;
	return src;
}

static int ecma119_read(struct burn_source *src,
			unsigned char *data,
			int size)
{
	struct ecma119_write_target *t = src->data;

	assert( src->read == ecma119_read && src->get_size == ecma119_get_size
					  && src->free_data == ecma119_free_data
					  && size == 2048);

	t->curblock++;
	memset(data, 0, size);
	switch(t->state) {
	case ECMA119_WRITE_SYSTEM_AREA:
		ecma119_write_system_area(t, data);
		break;
	case ECMA119_WRITE_PRI_VOL_DESC:
		ecma119_write_privol_desc(t, data);
		break;
	case ECMA119_WRITE_SUP_VOL_DESC_JOLIET:
		ecma119_write_supvol_desc_joliet(t, data);
		break;
	case ECMA119_WRITE_VOL_DESC_TERMINATOR:
		ecma119_write_vol_desc_terminator(t, data);
		break;
	case ECMA119_WRITE_L_PATH_TABLE:
		ecma119_write_path_table(t, data, 0, 0);
		break;
	case ECMA119_WRITE_M_PATH_TABLE:
		ecma119_write_path_table(t, data, 0, 1);
		break;
	case ECMA119_WRITE_L_PATH_TABLE_JOLIET:
		ecma119_write_path_table(t, data, ECMA119_JOLIET, 0);
		break;
	case ECMA119_WRITE_M_PATH_TABLE_JOLIET:
		ecma119_write_path_table(t, data, ECMA119_JOLIET, 1);
		break;
	case ECMA119_WRITE_DIR_RECORDS:
		ecma119_write_dir_records(t, data, 0);
		break;
	case ECMA119_WRITE_DIR_RECORDS_JOLIET:
		ecma119_write_dir_records(t, data, ECMA119_JOLIET);
		break;
	case ECMA119_WRITE_FILES:
		ecma119_write_files(t, data);
		break;
	case ECMA119_WRITE_DONE:
		return 0;
	default:
		assert(0);
	}

	return 2048;
}

static int ecma119_get_size(struct burn_source *src)
{
	struct ecma119_write_target *t = src->data;

	assert( src->read == ecma119_read && src->get_size == ecma119_get_size
					&& src->free_data == ecma119_free_data);

	return t->total_size;
}

/* free writer_data fields recursively */
static void ecma119_free_writer_data(struct ecma119_write_target *t,
				     struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	int i;

	susp_free_fields(&inf->susp);
	susp_free_fields(&inf->self_susp);
	susp_free_fields(&inf->parent_susp);

	if (inf->real_children) {
		free(inf->real_children);
	}

	for (i=0; i<dir->nfiles; i++) {
		struct file_write_info *finf = GET_FILE_INF(dir->files[i]);
		susp_free_fields(&finf->susp);
	}
	for (i=0; i<dir->nchildren; i++) {
		ecma119_free_writer_data(t, dir->children[i]);
	}
}

static void ecma119_free_data(struct burn_source *src)
{
	struct ecma119_write_target *t = src->data;

	assert( src->read == ecma119_read && src->get_size == ecma119_get_size
					&& src->free_data == ecma119_free_data);

	if (t->filelist) free(t->filelist);
	if (t->dirlist) free(t->dirlist);
	if (t->pathlist) free(t->pathlist);
	if (t->dirlist_joliet) free(t->dirlist_joliet);
	if (t->pathlist_joliet) free(t->pathlist_joliet);

	ecma119_free_writer_data(t, TARGET_ROOT(t));

	free(t);
	src->data = NULL;
	src->read = NULL;
	src->get_size = NULL;
	src->free_data = NULL;
}

/*============================================================================*/
/*				Writing functions			      */
/*============================================================================*/

static void ecma119_write_system_area(struct ecma119_write_target *t,
				      unsigned char *buf)
{
	if (t->curblock == 15) {
		t->state = ECMA119_WRITE_PRI_VOL_DESC;
	}
}

static void ecma119_write_privol_desc(struct ecma119_write_target *t,
				      unsigned char *buf)
{
	struct iso_tree_node *root = ISO_NODE(TARGET_ROOT(t));
	struct iso_volume *vol = t->volset->volume[t->volnum];
	uint8_t one = 1;	/* so that we can take &one */
	uint32_t zero = 0;
	time_t never = -1;
	uint8_t dir_record[34];

	ecma119_write_dir_record(t, dir_record, 0, root, RECORD_TYPE_ROOT);
	iso_struct_pack(ecma119_privol_fmt, buf,
			&one,
			"CD001",
			&one,
			system_id,
			vol->volume_id.cstr,
			&t->vol_space_size,
			&t->volset->volset_size,
			&t->volnum,
			&t->block_size,
			&t->path_table_size,
			&t->l_path_table_pos,
			&zero,
			&t->m_path_table_pos,
			&zero,
			dir_record,
			t->volset->volset_id.cstr,
			vol->publisher_id.cstr,
			vol->data_preparer_id.cstr,
			application_id,
			"",
			"",
			"",
			&t->now,
			&t->now,
			&never,
			&t->now,
			&one);

	t->state = (t->joliet) ? ECMA119_WRITE_SUP_VOL_DESC_JOLIET :
				 ECMA119_WRITE_VOL_DESC_TERMINATOR;
}

static void ecma119_write_supvol_desc_joliet(struct ecma119_write_target *t,
					     unsigned char *buf)
{	struct iso_tree_node *root = ISO_NODE(TARGET_ROOT(t));
	struct iso_volume *vol = t->volset->volume[t->volnum];
	uint8_t one = 1;	/* so that we can take &one */
	uint8_t two = 2;
	uint32_t zero = 0;
	time_t never = -1;
	uint8_t dir_record[34];

	ecma119_write_dir_record(t, dir_record, ECMA119_JOLIET,
				 root, RECORD_TYPE_ROOT);
	iso_struct_pack(ecma119_supvol_joliet_fmt, buf,
			&two,
			"CD001",
			&one,
			&zero,
			system_id_joliet,
			vol->volume_id.jstr,
			&t->vol_space_size,
			"%/E",
			&t->volset->volset_size,
			&t->volnum,
			&t->block_size,
			&t->path_table_size_joliet,
			&t->l_path_table_pos_joliet,
			&zero,
			&t->m_path_table_pos_joliet,
			&zero,
			dir_record,
			t->volset->volset_id.jstr,
			vol->publisher_id.jstr,
			vol->data_preparer_id.jstr,
			application_id_joliet,
			&zero,
			&zero,
			&zero,
			&t->now,
			&t->now,
			&never,
			&t->now,
			&one);

	t->state = ECMA119_WRITE_VOL_DESC_TERMINATOR;
}

static void ecma119_write_vol_desc_terminator(struct ecma119_write_target *t,
					      unsigned char *buf)
{
	buf[0] = 255;
	strcpy((char*)&buf[1], "CD001");
	buf[6] = 1;

	t->state = ECMA119_WRITE_L_PATH_TABLE;
}

/**
 * Write a full path table to the buffer (it is assumed to be large enough).
 * The path table will be broken into 2048-byte chunks later is necessary.
 */
static void ecma119_write_path_table_full(struct ecma119_write_target *t,
					  unsigned char *buf,
					  int flags,
					  int m_type)
{
	void (*write_int)(uint8_t*, uint32_t, int);
	struct iso_tree_dir *dir;
	const char *name;
	int len, parent, i;
	size_t off;

	node_namelen namelen;
	node_getname getname;
	off_t root_block;
	struct iso_tree_dir **pathlist;

	if (flags & ECMA119_JOLIET) {
		namelen = node_namelen_joliet;
		getname = node_getname_joliet;
		pathlist = t->pathlist_joliet;
		root_block = GET_DIR_INF(pathlist[0])->joliet_block;
	} else {
		namelen = node_namelen_iso;
		getname = node_getname_iso;
		pathlist = t->pathlist;
		root_block = pathlist[0]->block;
	}

	write_int = m_type ? iso_msb : iso_lsb;

	/* write the root directory */
	buf[0] = 1;
	buf[1] = 0;
	write_int(&buf[2], root_block, 4);
	write_int(&buf[6], 1, 2);

	/* write the rest */
	off = 10;
	for (i=1; i<t->dirlist_len; i++) {
		struct iso_tree_dir *dirparent;
		off_t block;

		dir = pathlist[i];
		name = getname(ISO_NODE(dir));
		len = namelen(ISO_NODE(dir));
		if (flags & ECMA119_JOLIET) {
			dirparent = GET_DIR_INF(dir)->real_parent;
			block = GET_DIR_INF(dir)->joliet_block;
		} else {
			dirparent = dir->parent;
			block = dir->block;
		}

		for (parent=0; parent<i; parent++) {
			if (pathlist[parent] == dirparent) {
				break;
			}
		}
		assert(parent<i);

		buf[off + 0] = len;
		buf[off + 1] = 0;	/* extended attribute length */
		write_int(&buf[off + 2], block, 4);
		write_int(&buf[off + 6], parent + 1, 2);
		memcpy(&buf[off + 8], name, len);
		off += 8 + len + len % 2;
	}
}

#define SDATA t->state_data.path_table
static void ecma119_write_path_table(struct ecma119_write_target *t,
				     unsigned char *buf,
				     int flags,
				     int m_type)
{
	int path_table_size;

	if (flags & ECMA119_JOLIET) {
		path_table_size = t->path_table_size_joliet;
	} else {
		path_table_size = t->path_table_size;
	}

	if (!SDATA.data) {
		SDATA.data = calloc(1, ROUND_UP(path_table_size, 2048));
		ecma119_write_path_table_full(t, SDATA.data, flags, m_type);
	}

	memcpy(buf, SDATA.data + SDATA.blocks*2048, 2048);
	SDATA.blocks++;

	if (SDATA.blocks*2048 >= path_table_size) {
		free(SDATA.data);
		SDATA.data = NULL;
		SDATA.blocks = 0;
		if (!t->joliet && m_type) {
			t->state = ECMA119_WRITE_DIR_RECORDS;
		} else {
			t->state++;
		}
	}
}
#undef SDATA

#define SDATA t->state_data.dir_records
static void ecma119_write_dir_records(struct ecma119_write_target *t,
				      unsigned char *buf,
				      int flags)
{
	struct iso_tree_dir **dirlist;

	if (flags & ECMA119_JOLIET) {
		dirlist = t->dirlist_joliet;
	} else {
		dirlist = t->dirlist;
	}

	if (!SDATA.data) {
		struct iso_tree_dir *dir = dirlist[SDATA.dir++];
		struct dir_write_info *inf = GET_DIR_INF(dir);

		SDATA.data = ecma119_write_dir(t, flags, dir);
		SDATA.pos = 0;

		if (flags & ECMA119_JOLIET) {
			SDATA.data_len = inf->joliet_len;
		} else {
			SDATA.data_len = inf->len + inf->susp_len;
		}
	}

	memcpy(buf, SDATA.data + SDATA.pos, 2048);
	SDATA.pos += 2048;

	if (SDATA.pos >= SDATA.data_len) {
		free(SDATA.data);
		SDATA.data = 0;
		SDATA.pos = 0;
		SDATA.data_len = 0;
		if (SDATA.dir == t->dirlist_len) {
			SDATA.dir = 0;
			if (!t->joliet || (flags & ECMA119_JOLIET)) {
				t->state = t->filelist_len ? ECMA119_WRITE_FILES
							   : ECMA119_WRITE_DONE;
			} else {
				t->state = ECMA119_WRITE_DIR_RECORDS_JOLIET;
			}
		}
	}
}
#undef SDATA

#define SDATA t->state_data.files
static void ecma119_write_files(struct ecma119_write_target *t,
				unsigned char *buf)
{
	int numread;

	if (!SDATA.fd) {
		struct iso_tree_file *f = t->filelist[SDATA.file++];
		assert(t->curblock == f->block);
		SDATA.fd = fopen(f->path, "r");
		SDATA.data_len = f->attrib.st_size;
		if (!SDATA.fd) {
			fprintf(stderr, "Error: couldn't open file %s: %s\n",
					f->path, strerror(errno));
		}
	}

	if (SDATA.fd) {
		numread = fread(buf, 1, 2048, SDATA.fd);
	} else {
		numread = t->block_size;
	}
	if (numread == -1) {
		fprintf(stderr, "Error reading file: %s\n", strerror(errno));
		return;
	}
	SDATA.pos += numread;

	if (!SDATA.pos || SDATA.pos >= SDATA.data_len) {
		fclose(SDATA.fd);
		SDATA.data_len = 0;
		SDATA.fd = NULL;
		SDATA.pos = 0;
		if (SDATA.file == t->filelist_len) {
			SDATA.file = 0;
			t->state = ECMA119_WRITE_DONE;
		}
	}
}
#undef SDATA

static unsigned char *ecma119_write_dir(struct ecma119_write_target *t,
					int flags,
					struct iso_tree_dir *dir)
{
	struct dir_write_info *inf = GET_DIR_INF(dir);
	int len = ROUND_UP(inf->susp_len + inf->len, 2048);
	unsigned char *buf;
	int i, pos;
	struct iso_tree_node *ch;

	susp_len slen;
	total_dirent_len dlen;
	struct iso_tree_node **children;
	int nchildren;

	assert ( ((flags & ECMA119_JOLIET) && t->curblock == inf->joliet_block)
			|| t->curblock == dir->block );

	if (flags & ECMA119_JOLIET) {
		children = ecma119_sort_joliet(dir);
		nchildren = inf->real_nchildren + dir->nfiles;
		len = ROUND_UP(inf->joliet_len, 2048);
		slen = susp_len_joliet;
		dlen = dirent_len_joliet;
	} else {
		children = ecma119_mangle_names(dir);
		nchildren = dir->nchildren + dir->nfiles;
		len = ROUND_UP(inf->susp_len + inf->len, 2048);
		slen = susp_len_iso;
		dlen = dirent_len_iso;
	}

	buf = calloc(1, len);
	pos = 0;

	/* write all the dir records */
	ecma119_write_dir_record(t, buf+pos, flags, ISO_NODE(dir),
				 RECORD_TYPE_SELF);
	pos += 34 + slen(&inf->self_susp);
	ecma119_write_dir_record(t, buf+pos, flags, ISO_NODE(dir),
				 RECORD_TYPE_PARENT);
	pos += 34 + slen(&inf->parent_susp);

	for (i=0; i<nchildren; i++) {
		ch = children[i];

		/* Joliet directories shouldn't list RR directory placeholders.
		 */
		if (flags & ECMA119_JOLIET
				&& S_ISREG( ch->attrib.st_mode )
				&& GET_FILE_INF(ch)->real_me) {
			continue;
		}
		ecma119_write_dir_record(t, buf+pos, flags, ch,
					 RECORD_TYPE_NORMAL);
		pos += dlen(ch);
	}

	if (flags & ECMA119_JOLIET) {
		free(children);
		return buf;
	}

	/* write all the SUSP continuation areas */
	susp_write_CE(t, &inf->self_susp, buf+pos);
	pos += inf->self_susp.CE_len;
	susp_write_CE(t, &inf->parent_susp, buf+pos);
	pos += inf->parent_susp.CE_len;
	for (i=0; i<nchildren; i++) {
		struct node_write_info *ninf = GET_NODE_INF(children[i]);
		susp_write_CE(t, &ninf->susp, buf+pos);
		pos += ninf->susp.CE_len;
	}
	free(children);
	return buf;
}

static void ecma119_write_dir_record(struct ecma119_write_target *t,
				     unsigned char *buf,
				     int flags,
				     struct iso_tree_node *node,
				     enum RecordType type)
{
	if (flags & ECMA119_JOLIET) {
		ecma119_write_dir_record_joliet(t, buf, node, type);
	} else {
		ecma119_write_dir_record_iso(t, buf, node, type);
	}
}

static void ecma119_write_dir_record_iso(struct ecma119_write_target *t,
					 uint8_t *buf,
					 struct iso_tree_node *node,
					 enum RecordType type)
{
	struct node_write_info *inf = GET_NODE_INF(node);
	uint8_t len_dr, len_fi, flags;
	uint8_t vol_seq_num = t->volnum;

	if (type == RECORD_TYPE_NORMAL) {
		len_fi = node_namelen_iso(node);
		len_dr = 33 + len_fi + 1 - len_fi%2 + inf->susp.non_CE_len;
		flags = (S_ISDIR(node->attrib.st_mode)) ? 2 : 0;
		iso_struct_pack(ecma119_dir_record_fmt, buf,
				&len_dr,
				&zero,
				&node->block,
				&node->attrib.st_size,
				&t->now,
				&flags,
				&zero,
				&zero,
				&vol_seq_num,
				&len_fi);
		iso_struct_pack_long(&buf[33], len_fi, '<', 'B', 0, 0, 0,
				     node_getname_iso(node));
		susp_write(t, &inf->susp, &buf[len_dr - inf->susp.non_CE_len]);
	} else if (type == RECORD_TYPE_PARENT && node->parent) {
		ecma119_write_dir_record_noname(t, buf, node, type,
					node->parent->block,
					node->parent->attrib.st_size, 0);
		susp_write(t, &GET_DIR_INF(node)->parent_susp, &buf[34]);
	} else {
		ecma119_write_dir_record_noname(t, buf, node, type,
						node->block,
						node->attrib.st_size, 0);
		if (type != RECORD_TYPE_ROOT) {
			susp_write(t, &GET_DIR_INF(node)->self_susp, &buf[34]);
		}
	}
}

static void ecma119_write_dir_record_joliet(struct ecma119_write_target *t,
					    uint8_t *buf,
					    struct iso_tree_node *node,
					    enum RecordType type)
{
	uint8_t len_dr, len_fi, flags;
	uint8_t vol_seq_num = t->volnum;
	uint32_t block, size;

	if (type == RECORD_TYPE_NORMAL) {
		if (S_ISDIR(node->attrib.st_mode)) {
			block = GET_DIR_INF(node)->joliet_block;
			size = GET_DIR_INF(node)->joliet_len;
			flags = 2;
		} else {
			block = node->block;
			size = node->attrib.st_size;
			flags = 0;
		}
		len_fi = node_namelen_joliet(node);
		len_dr = 34 + len_fi;
		iso_struct_pack(ecma119_dir_record_fmt, buf,
				&len_dr,
				&zero,
				&block,
				&size,
				&t->now,
				&flags,
				&zero,
				&zero,
				&vol_seq_num,
				&len_fi);
		iso_struct_pack_long(&buf[33], len_fi, '<', 'B', 0, 0, 0,
				     node->name.joliet);
	} else if (type == RECORD_TYPE_PARENT && node->parent) {
		struct iso_tree_dir *p = GET_DIR_INF(node)->real_parent;
		struct dir_write_info *inf = GET_DIR_INF(p);
		ecma119_write_dir_record_noname(t, buf, node, type,
				inf->joliet_block,
				inf->joliet_len,
				1);
	} else {
		struct dir_write_info *inf = GET_DIR_INF(node);
		ecma119_write_dir_record_noname(t, buf, node, type,
						inf->joliet_block,
						inf->joliet_len,
						1);
	}
}

/* this writes a directory record for a file whose RecordType is not
 * RECORD_TYPE_NORMAL. Since this implies that we don't need to write a file
 * id, the only difference between Joliet and non-Joliet records is whetheror
 * not we write the SUSP fields. */
static void ecma119_write_dir_record_noname(struct ecma119_write_target *t,
					    uint8_t *buf,
					    struct iso_tree_node *node,
					    enum RecordType type,
					    uint32_t block,
					    uint32_t size,
					    int joliet)
{
	int file_id;
	uint8_t len_dr;
	uint8_t flags = 2;
	uint8_t len_fi = 1;
	uint8_t zero = 0;
	struct dir_write_info *inf = GET_DIR_INF(node);

	assert( type != RECORD_TYPE_NORMAL );
	switch(type) {
	case RECORD_TYPE_ROOT:
		file_id = 0;
		len_dr = 34;
		break;
	case RECORD_TYPE_SELF:
		file_id = 0;
		len_dr = 34 + (joliet ? 0 : inf->self_susp.non_CE_len);
		break;
	case RECORD_TYPE_PARENT:
		file_id = 1;
		len_dr = 34 + (joliet ? 0 : inf->parent_susp.non_CE_len);
		break;
	case RECORD_TYPE_NORMAL: /* shut up warning */
		assert(0);
	}

	assert(iso_struct_calcsize(ecma119_dir_record_fmt) == 33);
	iso_struct_pack(ecma119_dir_record_fmt, buf,
			&len_dr,
			&zero,
			&block,
			&size,
			&t->now,
			&flags,		/* file flags */
			&zero,
			&zero,
			&len_fi,	/* vol seq number */
			&len_fi);	/* len_fi */
	buf[33] = file_id;
}

static void ecma119_setup_path_tables_iso(struct ecma119_write_target *t)
{
	int i, j, cur;
	struct iso_tree_node **children;

	t->pathlist = calloc(1, sizeof(void*) * t->dirlist_len);
	t->pathlist[0] = TARGET_ROOT(t);
	t->path_table_size = 10; /* root directory record */

	cur = 1;
	for (i=0; i<t->dirlist_len; i++) {
		struct iso_tree_dir *dir = t->pathlist[i];
		children = ecma119_mangle_names(dir);
		for (j=0; j<dir->nchildren + dir->nfiles; j++) {
			if (S_ISDIR(children[j]->attrib.st_mode)) {
				int len = 8 + node_namelen_iso(children[j]);
				t->pathlist[cur++] = ISO_DIR(children[j]);
				t->path_table_size += len + len % 2;
			}
		}
		free(children);
	}
}

static void ecma119_setup_path_tables_joliet(struct ecma119_write_target *t)
{
	int i, j, cur;
	struct iso_tree_node **children;

	t->pathlist_joliet = calloc(1, sizeof(void*) * t->dirlist_len);
	t->pathlist_joliet[0] = TARGET_ROOT(t);
	t->path_table_size_joliet = 10; /* root directory record */

	cur = 1;
	for (i=0; i<t->dirlist_len; i++) {
		struct iso_tree_dir *dir = t->pathlist_joliet[i];
		struct dir_write_info *inf = GET_DIR_INF(dir);
		children = ecma119_sort_joliet(dir);
		for (j=0; j<inf->real_nchildren + dir->nfiles; j++) {
			if (S_ISDIR(children[j]->attrib.st_mode)) {
				int len = 8 + node_namelen_joliet(children[j]);
				t->pathlist_joliet[cur++] =
					ISO_DIR(children[j]);
				t->path_table_size_joliet += len + len % 2;
			}
		}
	}
}
