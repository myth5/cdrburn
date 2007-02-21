#include "libburn.h"
#include "options.h"
#include "drive.h"
#include "transport.h"

/* ts A61007 */
/* #include <a ssert.h> */

#include <stdlib.h>
#include <string.h>

#include "libdax_msgs.h"
extern struct libdax_msgs *libdax_messenger;


struct burn_write_opts *burn_write_opts_new(struct burn_drive *drive)
{
	struct burn_write_opts *opts;

	opts = malloc(sizeof(struct burn_write_opts));
	if (opts == NULL) {
		libdax_msgs_submit(libdax_messenger, -1, 0x00020111,
			LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
			"Could not allocate new auxiliary object", 0, 0);
		return NULL;
	}
	opts->drive = drive;
	opts->refcount = 1;
	opts->write_type = BURN_WRITE_TAO;
	opts->block_type = BURN_BLOCK_MODE1;
	opts->toc_entry = NULL;
	opts->toc_entries = 0;
	opts->simulate = 0;
	opts->underrun_proof = drive->mdata->underrun_proof;
	opts->perform_opc = 1;
	opts->obs = -1;
	opts->obs_pad = 0;
	opts->start_byte = -1;
	opts->fill_up_media = 0;
	opts->has_mediacatalog = 0;
	opts->format = BURN_CDROM;
	opts->multi = 0;
	opts->control = 0;
	return opts;
}

void burn_write_opts_free(struct burn_write_opts *opts)
{
	if (--opts->refcount <= 0)
		free(opts);
}

struct burn_read_opts *burn_read_opts_new(struct burn_drive *drive)
{
	struct burn_read_opts *opts;

	opts = malloc(sizeof(struct burn_read_opts));
	opts->drive = drive;
	opts->refcount = 1;
	opts->raw = 0;
	opts->c2errors = 0;
	opts->subcodes_audio = 0;
	opts->subcodes_data = 0;
	opts->hardware_error_recovery = 0;
	opts->report_recovered_errors = 0;
	opts->transfer_damaged_blocks = 0;
	opts->hardware_error_retries = 3;

	return opts;
}

void burn_read_opts_free(struct burn_read_opts *opts)
{
	if (--opts->refcount <= 0)
		free(opts);
}

int burn_write_opts_set_write_type(struct burn_write_opts *opts,
				   enum burn_write_types write_type,
				   int block_type)
{
	int sector_get_outmode(enum burn_write_types write_type,
				enum burn_block_types block_type);
	int spc_block_type(enum burn_block_types b);
	
	/* ts A61007 */
	if (! ( (write_type == BURN_WRITE_SAO && block_type == BURN_BLOCK_SAO)
		 || (opts->drive->block_types[write_type] & block_type) ) ) {
bad_combination:;
		libdax_msgs_submit(libdax_messenger, -1, 0x00020112,
			LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
			"Bad combination of write_type and block_type", 0, 0);
		return 0;
	}
	/*  ts A61007 : obsoleting Assert in sector.c:get_outmode() */
	if (sector_get_outmode(write_type, (enum burn_block_types) block_type)
		 == -1)
		goto bad_combination;
	/*  ts A61007 : obsoleting Assert in spc.c:spc_block_type() */
	if (spc_block_type((enum burn_block_types) block_type) == -1)
		goto bad_combination;

	opts->write_type = write_type;
	opts->block_type = block_type;
	return 1;

	/* a ssert(0); */
}

void burn_write_opts_set_toc_entries(struct burn_write_opts *opts, int count,
				     struct burn_toc_entry *toc_entries)
{
	opts->toc_entries = count;
	opts->toc_entry = malloc(count * sizeof(struct burn_toc_entry));
	memcpy(opts->toc_entry, &toc_entries,
	       sizeof(struct burn_toc_entry) * count);
}

void burn_write_opts_set_format(struct burn_write_opts *opts, int format)
{
	opts->format = format;
}

int burn_write_opts_set_simulate(struct burn_write_opts *opts, int sim)
{
	if (opts->drive->mdata->simulate) {
		opts->simulate = sim;
		return 1;
	}
	return 0;
}

int burn_write_opts_set_underrun_proof(struct burn_write_opts *opts,
				       int underrun_proof)
{
	if (opts->drive->mdata->underrun_proof) {
		opts->underrun_proof = underrun_proof;
		return 1;
	}
	return 0;
}

void burn_write_opts_set_perform_opc(struct burn_write_opts *opts, int opc)
{
	opts->perform_opc = opc;
}

void burn_write_opts_set_has_mediacatalog(struct burn_write_opts *opts,
					  int has_mediacatalog)
{
	opts->has_mediacatalog = has_mediacatalog;
}

void burn_write_opts_set_mediacatalog(struct burn_write_opts *opts,
				      unsigned char mediacatalog[13])
{
	memcpy(opts->mediacatalog, &mediacatalog, 13);
}


/* ts A61106 */
void burn_write_opts_set_multi(struct burn_write_opts *opts, int multi)
{
	opts->multi = !!multi;
}


/* ts A61222 */
void burn_write_opts_set_start_byte(struct burn_write_opts *opts, off_t value)
{
	opts->start_byte = value;
}


/* ts A70207 API */
/** @param flag Bitfield for control purposes:
                bit0= do not choose type but check the one that is already set
                bit1= do not issue error messages via burn_msgs queue
*/
enum burn_write_types burn_write_opts_auto_write_type(
		struct burn_write_opts *opts, struct burn_disc *disc,
		char reasons[1024], int flag)
{
	struct burn_multi_caps *caps = NULL;
	struct burn_drive *d = opts->drive;
	struct burn_disc_mode_demands demands;
	enum burn_write_types wt;
	int ret, would_do_sao = 0;
	char *reason_pt;

	reasons[0] = 0;

	if (d->status != BURN_DISC_BLANK &&
	    d->status != BURN_DISC_APPENDABLE){
		if (d->status == BURN_DISC_FULL)
			strcat(reasons, "MEDIA: closed or not recordable, ");
		else
			strcat(reasons,"MEDIA: no writeable media detected, ");
		if (!(flag & 3))
			libdax_msgs_submit(libdax_messenger, d->global_index,
				0x0002013a,
				LIBDAX_MSGS_SEV_FATAL, LIBDAX_MSGS_PRIO_HIGH,
				"No suitable media detected", 0, 0);
		return BURN_WRITE_NONE;
	}
	ret = burn_disc_get_write_mode_demands(disc, opts, &demands,
			 			!!opts->fill_up_media);
	if (ret <= 0) {
		strcat(reasons, "cannot recognize job demands, ");
		{wt = BURN_WRITE_NONE; goto ex;}
	}
	if (demands.exotic_track && !d->current_is_cd_profile) {
		if (demands.audio)
			strcat(reasons, "audio track prohibited by non-CD, ");
		else
			strcat(reasons, "exotic track prohibited by non-CD, ");
		{wt = BURN_WRITE_NONE; goto ex;}
	}
	if ((flag & 1) && opts->write_type != BURN_WRITE_SAO)
		goto try_tao;
	reason_pt = reasons + strlen(reasons);
	strcat(reasons, "SAO: ");
	if (d->status != BURN_DISC_BLANK) {
		strcat(reasons, "write type SAO works only on blank media, ");
		goto try_tao;
	}
	burn_disc_free_multi_caps(&caps);
	ret = burn_disc_get_multi_caps(d, BURN_WRITE_SAO, &caps, 0);
	if (ret < 0) {
no_caps:;
		strcat(reasons, "cannot inquire write mode capabilities, ");
		{wt = BURN_WRITE_NONE; goto ex;}
	} else if (ret == 0) {
		strcat(reasons, "no SAO offered by drive and media, ");
		goto no_sao;
	}
	if ((opts->multi || demands.multi_session) &&
	    !caps->multi_session)
		strcat(reasons, "multi session capability lacking, ");
	if (demands.will_append)
		strcat(reasons, "appended session capability lacking, ");
	if (demands.multi_track && !caps->multi_track)
		strcat(reasons, "multi track capability lacking, ");
	if (demands.unknown_track_size == 1)
		strcat(reasons, "track size unpredictable, ");
	if (demands.mixed_mode)
		strcat(reasons, "tracks of different modes mixed, ");
	if (demands.exotic_track)
		strcat(reasons, "non-audio, non-data track, ");
	else if (d->current_is_cd_profile)
		if ((d->block_types[BURN_WRITE_TAO] & demands.block_types) !=
			demands.block_types)
			strcat(reasons, "drive dislikes block type, ");
	if (d->current_is_cd_profile && opts->fill_up_media)
		strcat(reasons, "cd sao cannot do media fill up yet, ");
	if (strcmp(reason_pt, "SAO: ") != 0)
		goto no_sao;
	would_do_sao = 1;
	if (demands.unknown_track_size == 2 && !(flag & 1)) {
		strcat(reasons, "would have to use default track sizes, ");
		goto no_sao;
	} else if (caps->might_do_sao == 3 && !(flag & 1))
		goto try_tao;
do_sao:;
	if (!(flag & 1))
		burn_write_opts_set_write_type(
					opts, BURN_WRITE_SAO, BURN_BLOCK_SAO);
	{wt = BURN_WRITE_SAO; goto ex;}
no_sao:;
try_tao:;
	if ((flag & 1) && opts->write_type != BURN_WRITE_TAO)
		goto try_raw;
	reason_pt = reasons + strlen(reasons);
	strcat(reasons, "TAO: ");
	burn_disc_free_multi_caps(&caps);
	ret = burn_disc_get_multi_caps(d, BURN_WRITE_TAO, &caps, 0);
	if (ret < 0)
		goto no_caps;
	if (ret == 0) {	
		strcat(reasons, "no TAO offered by drive and media, ");
		goto no_tao;
	}
	if ((opts->multi || demands.multi_session) && !caps->multi_session)
		strcat(reasons, "multi session capability lacking, ");
	if (demands.multi_track && !caps->multi_track)
		strcat(reasons, "multi track capability lacking, ");
	if (demands.exotic_track)
		strcat(reasons, "non-audio, non-data track, ");
	if (d->current_is_cd_profile)
		if ((d->block_types[BURN_WRITE_TAO] & demands.block_types) !=
			demands.block_types)
			strcat(reasons, "drive dislikes block type, ");
	if (strcmp(reason_pt, "TAO: ") != 0)
		goto no_tao;
	/* ( TAO data/audio block size will be handled automatically ) */
	if (!(flag & 1))
		burn_write_opts_set_write_type(
				opts, BURN_WRITE_TAO, BURN_BLOCK_MODE1);
	{wt = BURN_WRITE_TAO; goto ex;}
no_tao:;
	if (would_do_sao && !(flag & 1))
		goto do_sao;
	if (!d->current_is_cd_profile)
		goto no_write_mode;
try_raw:;
	if ((flag & 1) && opts->write_type != BURN_WRITE_RAW)
		goto no_write_mode;

	if (!(flag & 1)) /* For now: no automatic raw write modes */
		goto no_write_mode;

	reason_pt = reasons + strlen(reasons);
	strcat(reasons, "RAW: ");
	if (!d->current_is_cd_profile)
		strcat(reasons, "write type RAW prohibited by non-CD, ");
	else if (d->status != BURN_DISC_BLANK)
		strcat(reasons, "write type RAW works only on blank media, ");
	else if ((d->block_types[BURN_WRITE_TAO] & demands.block_types) !=
						    	demands.block_types)
		strcat(reasons, "drive dislikes block type, ");
	if (strcmp(reason_pt, "RAW: ") != 0)
		goto no_write_mode;

	/*  For now: no setting of raw write modes */

	{wt = BURN_WRITE_RAW; goto ex;}

no_write_mode:;
	wt = BURN_WRITE_NONE;
ex:;
	burn_disc_free_multi_caps(&caps);
	if (wt == BURN_WRITE_NONE && !(flag & 3)) {
		libdax_msgs_submit(libdax_messenger, d->global_index,
			0x0002012b,
			LIBDAX_MSGS_SEV_SORRY, LIBDAX_MSGS_PRIO_HIGH,
			"Drive offers no suitable write mode with this job",
			0, 0);
	}
	return wt;
}


/* ts A70213 : new API function */
void burn_write_opts_set_fillup(struct burn_write_opts *opts,int fill_up_media)
{
	opts->fill_up_media = !!fill_up_media;
	return;
}


void burn_read_opts_set_raw(struct burn_read_opts *opts, int raw)
{
	opts->raw = raw;
}

void burn_read_opts_set_c2errors(struct burn_read_opts *opts, int c2errors)
{
	opts->c2errors = c2errors;
}

void burn_read_opts_read_subcodes_audio(struct burn_read_opts *opts,
					int subcodes_audio)
{
	opts->subcodes_audio = subcodes_audio;
}

void burn_read_opts_read_subcodes_data(struct burn_read_opts *opts,
				       int subcodes_data)
{
	opts->subcodes_data = subcodes_data;
}

void burn_read_opts_set_hardware_error_recovery(struct burn_read_opts *opts,
						int hardware_error_recovery)
{
	opts->hardware_error_recovery = hardware_error_recovery;
}

void burn_read_opts_report_recovered_errors(struct burn_read_opts *opts,
					    int report_recovered_errors)
{
	opts->report_recovered_errors = report_recovered_errors;
}

void burn_read_opts_transfer_damaged_blocks(struct burn_read_opts *opts,
					    int transfer_damaged_blocks)
{
	opts->transfer_damaged_blocks = transfer_damaged_blocks;
}

void burn_read_opts_set_hardware_error_retries(struct burn_read_opts *opts,
					       unsigned char
					       hardware_error_retries)
{
	opts->hardware_error_retries = hardware_error_retries;
}

