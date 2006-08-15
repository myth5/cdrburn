/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include "writer.h"
#include "util.h"
#include "volume.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define APPLICATION_ID "LIBBURN SUITE (C) 2006 M.DANIC/L.BIDDELL/A.NARAYANAN"

enum DirType {
	DIR_TYPE_SELF,
	DIR_TYPE_PARENT,
	DIR_TYPE_NORMAL,
	DIR_TYPE_ROOT /* the dir record in the volume descriptor */
};

enum SUSPType {
	SUSP_TYPE_SP = 1 << 0,
	SUSP_TYPE_CE = 1 << 1,
	SUSP_TYPE_NM = 1 << 2
};

static int get_path_table_size(struct iso_tree_dir **ddir);
static void iso_next_state(struct iso_write_target *target);
static int iso_source_get_size(struct burn_source *src);
static void iso_source_free(struct burn_source *src);
static void iso_target_layout(struct iso_write_target *target);
static void iso_dir_layout(struct iso_write_target *target,
			   struct iso_tree_dir **dir);
static void iso_file_layout(struct iso_write_target *target,
			    struct iso_tree_dir **dir);
static int iso_write_system_area(struct iso_write_target *t,
				 unsigned char *buffer,
				 enum burn_source_status *err);
static int iso_write_pri_volume(struct iso_write_target *t,
				unsigned char *buffer,
				enum burn_source_status *err);
static int iso_write_volume_terminator(struct iso_write_target *t,
				       unsigned char *buffer,
				       enum burn_source_status *err);
static int write_path_table_record(unsigned char *buffer,
				   struct iso_tree_dir **ddir, int *position,
				   int lsb);
static int write_path_table_records(unsigned char *buffer,
				    struct iso_tree_dir **ddir,
				    int level, int *position, int lsb);
static int iso_write_path_table(struct iso_write_target *t,
				unsigned char *buffer,
				enum burn_source_status *err, int lsb);
static int iso_write_dir_record(struct iso_write_target *target,
				unsigned char *buffer,
				struct iso_tree_dir **dir, enum DirType type);
static int iso_write_file_record(struct iso_write_target *t,
				 unsigned char *buffer,
				 struct iso_tree_file **ffile, 
				 enum DirType type);
static void iso_write_file_id(unsigned char *buf, int size,
			      struct iso_tree_file **f);
static struct iso_tree_dir **find_dir_at_block(struct iso_tree_dir **ddir, 
					       int block);
static struct iso_tree_file **find_file_at_block(struct iso_tree_dir **ddir, 
						 int block);
static void write_child_records(struct iso_write_target *t,
				unsigned char *buffer, 
				enum burn_source_status *err, 
				struct iso_tree_dir *dir);
static int iso_write_dir_records(struct iso_write_target *t,
				 unsigned char *buffer,
				 enum burn_source_status *err);
static int get_directory_record_length(const char *isoname);
static int iso_write_er_area(struct iso_write_target *t, 
			     unsigned char *buffer,
			     enum burn_source_status *err);
static int copy_file_to_buffer(FILE * fd, unsigned char *buffer, int length);
static int iso_write_files(struct iso_write_target *t,
			   unsigned char *buffer,
			   enum burn_source_status *err);

int get_path_table_size(struct iso_tree_dir **ddir)
{
	const char *isoname;
	int i, size = 0;
	struct iso_tree_dir *dir = *ddir;

	assert(dir);

	isoname = iso_tree_dir_get_name(ddir, ISO_FILE_NAME_ISO);

	if (isoname) {
		/* a path table record is 8 bytes + the length of the
		   directory identifier */
		size += (8 + strlen(isoname));
	} else {
		/* if iso_tree_dir_get_name is NULL, this is the root dir and
		   will have a path record of 10 bytes */
		size += 10;
	}

	/* pad the field if the directory identifier is an odd number of
	   bytes */
	if (size % 2)
		++size;

	for (i = 0; i < dir->nchildren; i++)
		size += get_path_table_size(dir->children[i].me);

	return size;
}

struct burn_source *iso_source_new(struct iso_volumeset *volumeset, int volume)
{
	struct burn_source *src;
	struct iso_write_target *t;

	if (!volumeset)
		return NULL;
	if (volume >= volumeset->volumeset_size)
		return NULL;

	t = malloc(sizeof(struct iso_write_target));

	iso_tree_sort(volumeset->root);

	t->volset = volumeset;
	t->volset->refcount++;
	t->volume = volume;
	t->now = time(NULL);
	t->phys_sector_size = 2048;
	iso_target_layout(t);

	t->state = ISO_WRITE_BEFORE;
	iso_next_state(t);	/* prepare for the first state */

	src = malloc(sizeof(struct burn_source));
	src->refcount = 1;
	src->free_data = iso_source_free;
	src->read = iso_source_generate;
	src->read_sub = NULL;
	src->get_size = iso_source_get_size;
	src->data = t;

	return src;
}

void iso_source_free(struct burn_source *src)
{
	struct iso_write_target *target = src->data;

	assert(src->read == iso_source_generate &&
	       src->read_sub == NULL && src->get_size == iso_source_get_size);

	iso_volumeset_free(target->volset);
	free(target);

	return;
}

static int get_susp_length(struct RRInfo *info, enum SUSPType fields)
{
	int len = 0;
	
	if (fields & SUSP_TYPE_SP)
		len += 7;
	if (fields & SUSP_TYPE_CE)
		len += 28;
	if (info->px[0] != 0)
		len += info->px[2];
	if (info->tf)
		len += info->tf[2];
	if (info->nm && (fields & SUSP_TYPE_NM))
		len += info->nm[2];
	
	return len;
}

int get_directory_record_length(const char *isoname)
{
	int size;

	/* size = the length of the filename + 33 bytes (9.1) */
	size = 33 + strlen(isoname);
	/* if size is odd, pad it with a single byte (9.1.12) */
	if (size % 2)
		++size;

	return size;
}

/* fill in the 'logical_block' and 'size' values for each directory */
void iso_dir_layout(struct iso_write_target *target,
		    struct iso_tree_dir **ddir)
{
	int i, length, sectors, rr;
	const char *name;
	struct iso_tree_dir *dir = *ddir;

	rr = iso_volumeset_get_rr(target->volset);
	/* Each directory record starts with a record pointing to itself
	   and another pointing to its parent; combined, they are 68 bytes.
	   If RR is in use, calculate the susp length as well. */
	length = 68;
	if (rr) {
		/* if this is the root dir recored, we need to include the
		   length of the CE and SP fields */
		if (ddir == target->volset->root) {
			length += get_susp_length(&dir->rr, SUSP_TYPE_SP | SUSP_TYPE_CE);
			length += get_susp_length(&dir->rr, 0);
		} else {
			length += get_susp_length(&dir->rr, 0);
			length += get_susp_length(&dir->parent->rr, 0);
		}
	}

	/* calculate the length of this directory */
	name = iso_tree_dir_get_name(ddir, ISO_FILE_NAME_ISO);
	if (name)
		length += get_directory_record_length(name);
	else
		length += get_directory_record_length("");
	if (rr)
		length += get_susp_length(&dir->rr, SUSP_TYPE_NM);
	
	for (i = 0; i < dir->nfiles; ++i) {
		name = iso_tree_file_get_name(dir->files[i].me,
					      ISO_FILE_NAME_ISO);
		length += get_directory_record_length(name);
		if (rr)
			length += get_susp_length(&dir->files[i].rr, SUSP_TYPE_NM);
	}

	for (i = 0; i < dir->nchildren; ++i) {
		name = iso_tree_dir_get_name(dir->children[i].me,
					     ISO_FILE_NAME_ISO);
		length += get_directory_record_length(name);
		if (rr)
			length += get_susp_length(&dir->children[i].rr, SUSP_TYPE_NM);
	}

	/* dir->size is the number of bytes needed to hold the directory
	   record for each file and subdirectory of 'dir', padded to use up
	   all of the bytes of a physical sector. */
	sectors = length / target->phys_sector_size;
	if (length % target->phys_sector_size)
		sectors++;
	dir->size = sectors * target->phys_sector_size;
	dir->logical_block = target->logical_block;
	target->logical_block += sectors;

	/* now calculate the lengths of its children */
	for (i = 0; i < dir->nchildren; ++i) {
		iso_dir_layout(target, dir->children[i].me);
	}
	
	return;
}

/* fill in the 'logical_block' and 'size' values for each file */
void iso_file_layout(struct iso_write_target *target,
		     struct iso_tree_dir **ddir)
{
	int i, sectors;
	struct iso_tree_dir *dir = *ddir;

	for (i = 0; i < dir->nfiles; ++i) {
		dir->files[i].logical_block = target->logical_block;
		sectors = dir->files[i].size / target->phys_sector_size;
		if (dir->files[i].size % target->phys_sector_size)
			sectors++;
		/* ensure we move past this logical block if the file was empty */
		else if (!dir->files[i].size)
			sectors++;
		target->logical_block += sectors;
	}

	for (i = 0; i < dir->nchildren; ++i)
		iso_file_layout(target, dir->children[i].me);

	return;
}

void iso_target_layout(struct iso_write_target *target)
{
	/* logical block numbering starts at 1, not 0 */
	target->logical_block = 1;
	/* system area */
	target->logical_block += 16;
	/* primary volume descriptor */
	target->logical_block++;
	/* volume descriptor terminator */
	target->logical_block++;
	
	target->logical_block_size = 2048;
	target->path_table_size = get_path_table_size(target->volset->root);
	/* put a 1-block buffer before each path table */
	target->l_path_table_pos = 19;
	target->logical_block += 2;
	target->total_size += 2 * target->phys_sector_size;
	/* put a 1-block buffer before each path table */
	target->m_path_table_pos = 21;
	target->logical_block += 2;
	
	iso_dir_layout(target, target->volset->root);
	/* iso_write_dir_records() puts a 1-block buffer after the directory 
	   section, so increment this accordingly */
	target->logical_block++;
	
	/* the ce area only takes up one block */
	target->susp_er_pos = target->logical_block++;
	
	iso_file_layout(target, target->volset->root);
	
	target->volume_space_size = target->logical_block;
	target->total_size = target->volume_space_size * target->phys_sector_size;
	
	return;
}

int iso_source_get_size(struct burn_source *src)
{
	struct iso_write_target *target = src->data;

	assert(src->read == iso_source_generate &&
	       src->read_sub == NULL && src->get_size == iso_source_get_size);

	return target->total_size;
}

void iso_next_state(struct iso_write_target *target)
{
	switch (++target->state) {
	case ISO_WRITE_BEFORE:
		/* this should never occur */
		assert(0);
		break;
	case ISO_WRITE_SYSTEM_AREA:
		target->state_data.system_area.sectors = 0;
		break;
	case ISO_WRITE_PRI_VOL_DESC:
		target->total_size = target->logical_block - 1;
		break;
	case ISO_WRITE_VOL_DESC_TERMINATOR:
		break;
	case ISO_WRITE_L_PATH_TABLE:
		target->state_data.path_tables.sectors = 0;
		break;
	case ISO_WRITE_M_PATH_TABLE:
		target->state_data.path_tables.sectors = 0;
		break;
	case ISO_WRITE_DIR_RECORDS:
		target->state_data.dir_records.sectors = 0;
		break;
	case ISO_WRITE_ER_AREA:
		break;
	case ISO_WRITE_FILES:
		target->state_data.dir_records.sectors = 0;
		break;
	case ISO_WRITE_DONE:
		break;
	}

	return;
}

int iso_source_generate(struct burn_source *src, unsigned char *buffer,
			int size)
{
	int next = 0;
	enum burn_source_status err = BURN_SOURCE_OK;
	struct iso_write_target *target = src->data;

	assert(src->read == iso_source_generate &&
	       src->read_sub == NULL && src->get_size == iso_source_get_size);

	/* make sure the app didn't fuck up badly. */
	if (size != target->phys_sector_size)
		return BURN_SOURCE_FAILED;	/* XXX better code */

	/* make sure 'buffer' doesn't have anything in it */
	memset(buffer, 0, size);

	switch (target->state) {
	case ISO_WRITE_BEFORE:
		/* this should never occur */
		assert(0);
	case ISO_WRITE_SYSTEM_AREA:
		next = iso_write_system_area(target, buffer, &err);
		break;
	case ISO_WRITE_PRI_VOL_DESC:
		/* set target->logical_block to the logical block containing
		   the root directory record */
		target->logical_block = 23;
		next = iso_write_pri_volume(target, buffer, &err);
		break;
	case ISO_WRITE_VOL_DESC_TERMINATOR:
		next = iso_write_volume_terminator(target, buffer, &err);
		break;
	case ISO_WRITE_L_PATH_TABLE:
		next = iso_write_path_table(target, buffer, &err, 1);
		break;
	case ISO_WRITE_M_PATH_TABLE:
		next = iso_write_path_table(target, buffer, &err, 0);
		break;
	case ISO_WRITE_DIR_RECORDS:
		next = iso_write_dir_records(target, buffer, &err);
		break;
	case ISO_WRITE_ER_AREA:
		next = iso_write_er_area(target, buffer, &err);
		break;
	case ISO_WRITE_FILES:
		next = iso_write_files(target, buffer, &err);
		break;
	case ISO_WRITE_DONE:
		err = BURN_SOURCE_EOF;
		break;
	}

	if (next)
		iso_next_state(target);

	return err;
}

/* writes 16 sectors of '0' */
int iso_write_system_area(struct iso_write_target *t,
			  unsigned char *buffer, enum burn_source_status *err)
{
	struct iso_state_system_area *state = &t->state_data.system_area;

	memset(buffer, 0, t->phys_sector_size);
	return (++state->sectors == 16);
}

/* writes the primary volume descriptor */
int iso_write_pri_volume(struct iso_write_target *t,
			 unsigned char *buffer, enum burn_source_status *err)
{
	/* volume descriptor type (8.4.1) */
	buffer[0] = 1;
	/* standard identifier (8.4.2) */
	memcpy(&buffer[1], "CD001", 5);
	/* volume descriptor version (8.4.3) */
	buffer[6] = 1;
	/* unused field (8.4.4) */
	buffer[7] = 0;
	/* system identifier (8.4.5) */
	/* XXX mkisofs puts "LINUX" here */
	memset(&buffer[8], ' ', 32);
	/* volume identifier (8.4.6) */
	iso_d_strcpy(&buffer[40], 32, t->volset->volume_id[t->volume]);
	/* unused field (8.4.7) */
	memset(&buffer[72], 0, 8);
	/* volume space size (8.4.8) */
	iso_bb(&buffer[80], t->volume_space_size, 4);
	/* unused field (8.4.9) */
	memset(&buffer[88], 0, 32);
	/* volume set size (8.4.10) */
	iso_bb(&buffer[120], t->volset->volumeset_size, 2);
	/* volume sequence number (8.4.11) */
	iso_bb(&buffer[124], t->volume + 1, 2);
	/* logical block size (8.4.12) */
	iso_bb(&buffer[128], t->logical_block_size, 2);
	/* path table size (8.4.13) */
	iso_bb(&buffer[132], t->path_table_size, 4);
	/* location of occurance of type l path table (8.4.14) */
	iso_lsb(&buffer[140], t->l_path_table_pos, 4);
	/* location of optional occurance of type l path table (8.4.15) */
	iso_lsb(&buffer[144], 0, 4);
	/* location of occurance of type m path table (8.4.16) */
	iso_msb(&buffer[148], t->m_path_table_pos, 4);
	/* location of optional occurance of type m path table (8.4.17) */
	iso_msb(&buffer[152], 0, 4);
	/* directory record for root directory (8.4.18) */
	iso_write_dir_record(t, &buffer[156], t->volset->root, DIR_TYPE_ROOT);
	/* volume set identifier (8.4.19) */
	iso_d_strcpy(&buffer[190], 128, t->volset->volumeset_id);
	/* publisher identifier (8.4.20) */
	iso_a_strcpy(&buffer[318], 128, t->volset->publisher_id);
	/* data preparer identifier (8.4.21) */
	iso_a_strcpy(&buffer[446], 128, t->volset->data_preparer_id);
	/* application identifier (8.4.22) */
	iso_a_strcpy(&buffer[574], 128, APPLICATION_ID);
	/* copyright file identifier (8.4.23) */
	iso_write_file_id(&buffer[702], 37, t->volset->copyright_file);
	/* abstract file identifier (8.4.24) */
	iso_write_file_id(&buffer[739], 37, t->volset->abstract_file);
	/* bibliographic file identifier (8.4.25) */
	iso_write_file_id(&buffer[776], 37, t->volset->bibliographic_file);
	/* volume creation date and time (8.4.26) */
	/* XXX is this different for multisession? */
	iso_datetime_17(&buffer[813], t->now);
	/* volume modification date and time (8.4.27) */
	iso_datetime_17(&buffer[830], t->now);
	/* volume expiration date and time (8.4.28) */
	iso_datetime_17(&buffer[847], t->volset->expiration_time);
	/* volume effective date and time (8.4.29) */
	iso_datetime_17(&buffer[864],
			t->volset->effective_time == (time_t) - 1 ?
			t->now : t->volset->effective_time);
	/* file structure version (8.4.30) */
	buffer[881] = 1;
	/* reserved for future standardization (8.4.31) */
	buffer[882] = 0;
	/* application use (8.4.32) */
	/* XXX put something in here? does mkisofs? */
	memset(&buffer[883], 0, 512);
	/* reserved for future standardization (8.4.33) */
	memset(&buffer[1395], 0, 653);

	return 1;
}

void iso_write_file_id(unsigned char *buf, int size, struct iso_tree_file **f)
{
	if (!f)
		memset(buf, ' ', size);
	else
		iso_filecpy(buf, size,
			    iso_tree_file_get_name(f, ISO_FILE_NAME_ISO),
			    (*f)->version);
}

/* write the contents of the SP System Use Entry */
/* SUSP-112 5.3 */
static void fill_sp_sue(unsigned char *sp)
{
	sp[0] = 'S';
	sp[1] = 'P';
	sp[2] = 7;
	sp[3] = 1;
	sp[4] = 190;
	sp[5] = 239;
	sp[6] = 0;
	
	return;
}

/* write the contents of the CE System Use Entry */
/* SUSP-112 5.1 */
static void fill_ce_sue(struct iso_write_target *t,
			unsigned char *ce)
{
	ce[0] = 'C';
	ce[1] = 'E';
	ce[2] = 28;
	ce[3] = 1;
	iso_bb(&ce[4], 0, 4);
	iso_bb(&ce[12], t->susp_er_pos, 4);
	/* the length of the rock-ridge er area is 185, and that's all we
	 * store in the ce area right now */
	iso_bb(&ce[20], 185, 4); 
	
	return;
}

/* return a string containing the System Use Sharing Protocol information 
 * for 'ddir'. */
static unsigned char* write_susp_info(struct iso_write_target *t,
			     struct RRInfo *rr, 
			     enum SUSPType type,
			     int *len)
{
	unsigned char *buffer = NULL;
	
	*len = 0;
	
	/* check to see if susp is even needed (right now, only the rock-ridge
	 * extensions use it). */
	if (iso_volumeset_get_rr(t->volset)) {
		int char_size, size = 0, offset = 0;
		int len_sp, len_ce, len_px, len_tf, len_nm;
		unsigned char *sp, *ce;
		
		char_size = sizeof (unsigned char *);
		len_sp = len_ce = 0;
		
		if (type & SUSP_TYPE_SP) {
			len_sp = 7; /* 7 bytes for the SP System Use Entry */
			sp = malloc(char_size * len_sp);
			fill_sp_sue(sp);
			size += len_sp;
		} else {
			sp = NULL;
		}

		if (type & SUSP_TYPE_CE) {
			len_ce = 28; /* 28 bytes for the CE System Use Entry */
			ce = malloc(char_size * len_ce);
			fill_ce_sue(t, ce);
			size += len_ce;
		} else {
			ce = NULL;
		}
		
		if (rr->px[0] != 0) {
			/* the 3rd byte of the PX field holds its length */
			len_px = rr->px[2];
			size += len_px;
		} else {
			len_px = 0;
		}
		
		if (rr->tf) {
			/* the 3rd byte of the TF field holds its length */
			len_tf = rr->tf[2];
			size += len_tf;
		} else {
			len_tf = 0;
		}
		
		if ((type & SUSP_TYPE_NM) && rr->nm) {
			/* the 3rd byte of the NM field holds its length */
			len_nm = rr->nm[2];
			size += len_nm;
		} else {
			len_nm = 0;
		}
		
		/* fill 'buffer' with the susp info */
		buffer = malloc(char_size * (size + 1));
		if (sp) {
			memcpy(buffer + offset, sp, len_sp);
			offset += len_sp;
		}
		if (ce) {
			memcpy(buffer + offset, ce, len_ce);
			offset += len_ce;
		}
		if (len_px) {
			memcpy(buffer + offset, rr->px, len_px);
			offset += len_px;
		}
		if (len_tf) {
			memcpy(buffer + offset, rr->tf, len_tf);
			offset += len_tf;
		}
		if (len_nm) {
			memcpy(buffer + offset, rr->nm, len_nm);
			offset += len_nm;
		}
		buffer[offset] = '\0';
		*len = offset;
	}
	
	return buffer;
}

int iso_write_dir_record(struct iso_write_target *t,
			 unsigned char *buffer,
			 struct iso_tree_dir **ddir, enum DirType type)
{
	int len_fi, len_susp, sz;
	const char *fi = NULL;
	unsigned char *susp; /* the contents, if any, of the "system use sharing protocol" buffer */
	struct iso_tree_dir *dir = *ddir;

	if (type != DIR_TYPE_NORMAL)
		len_fi = 1;
	else {
		assert(dir->parent);
		fi = iso_tree_dir_get_name(ddir, ISO_FILE_NAME_ISO);
		len_fi = strlen(fi);
	}

	/* determine if this is the root directory record */
	if ((*t->volset->root == dir) && (type == DIR_TYPE_SELF))
		susp = write_susp_info(t, &dir->rr, SUSP_TYPE_SP | SUSP_TYPE_CE, &len_susp);
	/* determine if this a SELF or PARENT directory record */
	else if (type != DIR_TYPE_NORMAL)
		susp = write_susp_info(t, &dir->rr, 0, &len_susp);
	/* none of the above == a normal directory record */
	else
		susp = write_susp_info(t, &dir->rr, SUSP_TYPE_NM, &len_susp);
	
	sz = 33 + len_fi + len_susp + !(len_fi % 2);

	/* length of directory record (9.1.1) */
	buffer[0] = sz;
	/* extended attribute record length (9.1.2) */
	buffer[1] = 0;		/* XXX allow for extended attribute records */
	/* location of extent (9.1.3) */
	iso_bb(&buffer[2], dir->logical_block, 4);
	/* data length (9.1.4) */
	iso_bb(&buffer[10], dir->size, 4);
	/* recording date and time (9.1.5) */
	iso_datetime_7(&buffer[18], t->now);
	/* file flags (9.1.6) */
	buffer[25] = ISO_FILE_FLAG_DIRECTORY;
	/* file unit size (9.1.7) */
	buffer[26] = 0;		/* XXX support this shit? */
	/* interleave gap size (9.1.8) */
	buffer[27] = 0;		/* XXX support this shit? */
	/* volume sequence number (9.1.9) */
	iso_bb(&buffer[28], dir->volume + 1, 2);
	/* length of file identifier (9.1.10) */
	buffer[32] = len_fi;
	/* file identifier */
	switch (type) {
	case DIR_TYPE_ROOT:
	case DIR_TYPE_SELF:
		buffer[33] = 0;
		break;
	case DIR_TYPE_PARENT:
		buffer[33] = 1;
		break;
	case DIR_TYPE_NORMAL:
		iso_d_strcpy(&buffer[33], len_fi, fi);
		break;
	}
	if (!(len_fi % 2))
		buffer[33 + len_fi] = 0;
	/* system use sharing protocol */
	if (susp)
	{
		int offset;
		if (!(len_fi % 2))
			offset = 34;
		else
			offset = 33;
		memcpy(&buffer[offset + len_fi], susp, len_susp);
	}
	
	return sz;
}

/* write the file record as per (9.1) */
int iso_write_file_record(struct iso_write_target *t,
			  unsigned char *buffer,
			  struct iso_tree_file **ffile, enum DirType type)
{
	int len_fi, len_susp;
	const char *fi = NULL;
	unsigned char *susp; /* the contents, if any, of the "system use sharing protocol" buffer */
	int sz;
	struct iso_tree_file *file = *ffile;

	if (type != DIR_TYPE_NORMAL)
		len_fi = 1;
	else {
		fi = iso_tree_file_get_name(ffile, ISO_FILE_NAME_ISO);
		len_fi = strlen(fi);
	}

	susp = write_susp_info(t, &file->rr, SUSP_TYPE_NM, &len_susp);
	
	sz = 33 + len_fi + len_susp + !(len_fi % 2);

	/* length of directory record (9.1.1) */
	buffer[0] = sz;
	/* extended attribute record length (9.1.2) */
	buffer[1] = 0;		/* XXX allow for extended attribute records */
	/* location of extent (9.1.3) */
	iso_bb(&buffer[2], file->logical_block, 4);
	/* data length (9.1.4) */
	iso_bb(&buffer[10], file->size, 4);
	/* recording date and time (9.1.5) */
	iso_datetime_7(&buffer[18], t->now);
	/* file flags (9.1.6) */
	buffer[25] = ISO_FILE_FLAG_NORMAL;
	/* file unit size (9.1.7) */
	buffer[26] = 0;		/* XXX support this shit? */
	/* interleave gap size (9.1.8) */
	buffer[27] = 0;		/* XXX support this shit? */
	/* volume sequence number (9.1.9) */
	iso_bb(&buffer[28], file->volume + 1, 2);
	/* length of file identifier (9.1.10) */
	buffer[32] = len_fi;
	/* file identifier */
	switch (type) {
	case DIR_TYPE_ROOT:
	case DIR_TYPE_SELF:
		buffer[33] = 0;
		break;
	case DIR_TYPE_PARENT:
		buffer[33] = 1;
		break;
	case DIR_TYPE_NORMAL:
		memcpy(&buffer[33], fi, len_fi);
		break;
	}
	if (!(len_fi % 2))
		buffer[33 + len_fi] = 0;
	/* system use sharing protocol */
	if (susp)
	{
		int offset;
		if (!(len_fi % 2))
			offset = 34;
		else
			offset = 33;
		memcpy(&buffer[offset + len_fi], susp, len_susp);
	}
	
	return sz;
}

/* writes the volume descriptor set terminator */
int iso_write_volume_terminator(struct iso_write_target *t,
				unsigned char *buffer, 
				enum burn_source_status *err)
{
	/* volume descriptor type (8.3.1) */
	buffer[0] = 255;
	/* standard identifier (8.3.2) */
	memcpy(&buffer[1], "CD001", 5);
	/* volume descriptor version (8.3.3) */
	buffer[6] = 1;
	/* reserved for future standardization (8.3.4) */
	memset(&buffer[7], 0, 2041);

	return 1;
}

/* write the path record for 'ddir' into 'buffer' */
int write_path_table_record(unsigned char *buffer,
			    struct iso_tree_dir **ddir, int *position, int lsb)
{
	int len, bytes_written;
	short parent_position;
	const char *name;
	struct iso_tree_dir *dir = *ddir;

	/* set the position in the path table of this directory */
	dir->position = *position;
	/* increment the position for the next path table record */
	*position += 1;
	if (dir->parent)
		parent_position = dir->parent->position;
	else
		parent_position = 1;

	name = iso_tree_dir_get_name(ddir, ISO_FILE_NAME_ISO);
	if (name)
		len = strlen(name);
	else
		len = 1;

	/* the directory identifier length (9.4.1) */
	buffer[0] = len;
	/* the extended attribute record length (9.4.2) */
	buffer[1] = 0;
	/* location of extent (9.4.3) */
	if (lsb)
		iso_lsb(&buffer[2], dir->logical_block, 4);
	else
		iso_msb(&buffer[2], dir->logical_block, 4);
	/* parent directory record (9.4.4) */
	if (lsb)
		iso_lsb(&buffer[6], parent_position, 2);
	else
		iso_msb(&buffer[6], parent_position, 2);
	/* the directory identifier (9.4.5) */
	if (name)
		memcpy(&buffer[8], name, len);
	else
		memset(&buffer[8], 0, len);
	/* padding field (9.4.6) */
	if (len % 2) {
		bytes_written = 9 + len;
		buffer[bytes_written] = 0;
	} else
		bytes_written = 8 + len;

	return bytes_written;
}

/* recursive function used to write the path records for each level
 * of the file system sequentially, i.e. root, then all children of
 * root, then all grandchildren of root, then all great-grandchildren
 * of root, etc. */
int write_path_table_records(unsigned char *buffer,
			     struct iso_tree_dir **ddir,
			     int level, int *position, int lsb)
{
	int i, offset;
	struct iso_tree_dir *dir = *ddir;

	/* ISO9660 only allows directories to be nested 8-deep */
	if (level >= 8)
		return 0;

	if (!level) {
		offset = write_path_table_record(buffer, ddir, position, lsb);
	} else {
		offset = 0;
		for (i = 0; i < dir->nchildren; ++i) {
			offset += write_path_table_records(buffer + offset,
							   dir->children[i].me,
							   level - 1,
							   position, lsb);
		}
	}

	return offset;
}

/* writes set of path table records */
int iso_write_path_table(struct iso_write_target *t,
			 unsigned char *buffer,
			 enum burn_source_status *err, int lsb)
{
	int i, offset, position;
	struct iso_state_path_tables *state = &t->state_data.path_tables;

	if (state->sectors) {
		offset = 0;
		position = 1;

		for (i = 0; i < 8; ++i) {
			offset += write_path_table_records(buffer + offset,
							   t->volset->root,
							   i, &position, lsb);
		}
	} else {
		/* empty buffer sector */
		memset(buffer, 0, t->phys_sector_size);
	}

	return (++state->sectors == 2);
}

struct iso_tree_dir **find_dir_at_block(struct iso_tree_dir **ddir, int block)
{
	int i;
	struct iso_tree_dir *dir = *ddir;
	struct iso_tree_dir **to_write = NULL;

	if (dir->logical_block == block)
		to_write = ddir;

	for (i = 0; (i < dir->nchildren) && !to_write; ++i) {
		to_write = find_dir_at_block(dir->children[i].me, block);
	}

	return to_write;
}

struct iso_tree_file **find_file_at_block(struct iso_tree_dir **ddir, int block)
{
	int i;
	struct iso_tree_dir *dir = *ddir;
	struct iso_tree_file **to_write = NULL;

	for (i = 0; (i < dir->nfiles) && !to_write; ++i) {
		if (dir->files[i].logical_block == block)
			to_write = dir->files[i].me;
	}

	for (i = 0; (i < dir->nchildren) && !to_write; ++i) {
		to_write = find_file_at_block(dir->children[i].me, block);
	}

	return to_write;
}

/* write the records for children of 'dir' */
void write_child_records(struct iso_write_target *t,
			 unsigned char *buffer,
			 enum burn_source_status *err, struct iso_tree_dir *dir)
{
	int file_counter, dir_counter, order, offset;

	file_counter = dir_counter = offset = 0;

	while ((file_counter < dir->nfiles) || (dir_counter < dir->nchildren)) {
		/* if there are files and dirs, compare them to figure out
		   which comes 1st alphabetically */
		if ((file_counter < dir->nfiles) &&
		    (dir_counter < dir->nchildren)) {
			const char *dirname, *filename;

			dirname = iso_tree_dir_get_name
				(dir->children[dir_counter].me,
				 ISO_FILE_NAME_ISO);
			filename = iso_tree_file_get_name
				(dir->files[file_counter].me,
				 ISO_FILE_NAME_ISO);
			order = strcmp(dirname, filename);
		} else {
			if (file_counter < dir->nfiles)
				/* only files are left */
				order = 1;
			else
				/* only dirs are left */
				order = -1;
		}

		if (order < 0) {
			offset += iso_write_dir_record(t,
						       buffer + offset,
						       dir->
						       children[dir_counter].
						       me, DIR_TYPE_NORMAL);
			dir_counter++;
		} else {
			offset += iso_write_file_record(t,
							buffer + offset,
							dir->
							files[file_counter].me,
							DIR_TYPE_NORMAL);
			file_counter++;
		}

	}

	return;
}

/* write out the next directory record */
int iso_write_dir_records(struct iso_write_target *t,
			  unsigned char *buffer, enum burn_source_status *err)
{
	int finished, offset;
	struct iso_tree_dir **ddir, *dir;
	struct iso_state_dir_records *state = &t->state_data.dir_records;
	
	finished = 0;		/* set to 1 when all directory records have
				   been written */

	if (state->sectors++) {
		/* t->logical_block is the next block to write.  find the dir
		   record which belongs there and write it out.  if none
		   exists, we're done writing the directory records. */
		
		ddir = find_dir_at_block(t->volset->root, t->logical_block);
		
		if (ddir) {
			/* 1) write the record for this directory 2) write the 
			   record for the parent directory 3) write the
			   records for all child files and directories */
			dir = *ddir;
			offset = iso_write_dir_record(t,
						      buffer,
						      ddir, DIR_TYPE_SELF);
			if (!dir->parent) {
				/* this is the root directory */
				offset += iso_write_dir_record(t,
							       buffer + offset,
							       ddir,
							       DIR_TYPE_PARENT);
			} else {
				offset += iso_write_dir_record(t,
							       buffer + offset,
							       dir->parent->me,
							       DIR_TYPE_PARENT);
			}

			write_child_records(t, buffer + offset, err, dir);

			/* dir->size should always be a multiple of
			   t->phys_sector_size */
			t->logical_block += (dir->size / t->phys_sector_size);
		} else {
			/* we just wrote out 2048 0's, so we still need to
			   increment this */
			t->logical_block++;
			finished = 1;
		}
	}

	return finished;
}

/* write the 'ER' area of the disc (RRIP-112: 4.3) */
int iso_write_er_area(struct iso_write_target *t, 
		      unsigned char *buffer,
		      enum burn_source_status *err)
{	
	if (iso_volumeset_get_rr(t->volset)) {
		char *text;
		int len;
		
		buffer[0] = 'E';
		buffer[1] = 'R';
		buffer[2] = 237;
		buffer[3] = 1;
		buffer[4] = 10;
		buffer[5] = 84;
		buffer[6] = 135;
		buffer[7] = 1;
		text = strdup(""
			"RRIP_1991ATHE ROCK RIDGE INTERCHANGE PROTOCOL "
			"PROVIDES SUPPORT FOR POSIX FILE SYSTEM "
			"SEMANTICSPLEASE CONTACT DISC PUBLISHER FOR "
			"SPECIFICATION SOURCE.  SEE PUBLISHER IDENTIFIER IN "
			"PRIMARY VOLUME DESCRIPTOR FOR CONTACT INFORMATION");
		len = strlen (text);
		
		memcpy (buffer + 8, text, len);
		free (text);
		
		/* next logical block... */
		t->logical_block++;
	}
	
	return 1;
}

int copy_file_to_buffer(FILE * fd, unsigned char *buffer, int length)
{
	int read;

	read = fread(buffer, 1, length, fd);

	return read;
}

int iso_write_files(struct iso_write_target *t,
		    unsigned char *buffer, enum burn_source_status *err)
{
	FILE *fd;
	int finished, sectors;
	struct iso_tree_file **ffile, *file;
	struct iso_state_files *state = &t->state_data.files;
	
	finished = 0;

	if (state->sectors) {
		/* continue writing the file at 't->logical_block' to
		   'buffer', then incremenet 't->logical_block' to the next
		   file section. */

		fd = t->current_fd;
		file = *(t->current_file);

		/* copy one sector of 'ffile' into 'buffer' */
		state->sectors +=
			copy_file_to_buffer(fd, buffer, t->phys_sector_size);

		if (feof(fd)) {
			fclose(fd);
			state->sectors = 0;
			sectors = file->size / t->phys_sector_size;
			if (file->size % t->phys_sector_size)
				sectors++;
			t->logical_block += sectors;
		}
	} else {
		/* start writing the file at 't->logical_block' to 'buffer'.
		   if none exists, we are done. */
		ffile = find_file_at_block(t->volset->root, t->logical_block);
		if (ffile) {
			t->current_file = ffile;
			file = *ffile;
			fd = fopen(file->path, "r");
			t->current_fd = fd;

			if (fd) {
				state->sectors += copy_file_to_buffer(fd,
								      buffer,
								      t->
								      phys_sector_size);
			}

			if (feof(fd)) {
				fclose(fd);
				fd = NULL;
			}

			if (!fd) {
				state->sectors = 0;
				sectors = file->size / t->phys_sector_size;
				if (file->size % t->phys_sector_size)
					sectors++;
				/* ensure we move past this logical block if the file was empty */
				else if (!file->size)
					sectors++;
				t->logical_block += sectors;
			}
		} else
			finished = 1;
	}

	return finished;
}
