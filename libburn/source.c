/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/* Copyright (c) 2004 - 2006 Derek Foreman, Ben Jansens
   Copyright (c) 2006 - 2010 Thomas Schmitt <scdbackup@gmx.net>
   Provided under GPL version 2 or later.
*/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif


#include <stdlib.h>
#include <string.h>
#include "libburn.h"
#include "source.h"
#include "structure.h"

void burn_source_free(struct burn_source *src)
{
	if (--src->refcount < 1) {
		if (src->free_data)
			src->free_data(src);
		free(src);
	}
}

enum burn_source_status burn_track_set_source(struct burn_track *t,
					      struct burn_source *s)
{
	s->refcount++;
	t->source = s;

	/* ts A61031 */
	t->open_ended = (s->get_size(s) <= 0);

	return BURN_SOURCE_OK;
}

struct burn_source *burn_source_new(void)
{
	struct burn_source *out;

	out = calloc(1, sizeof(struct burn_source));

	/* ts A70825 */
	if (out == NULL)
		return NULL;
	memset((char *) out, 0, sizeof(struct burn_source));

	out->refcount = 1;
	return out;
}


/* ts A71223 */
int burn_source_cancel(struct burn_source *src)
{
	if(src->read == NULL)
		if(src->version > 0)
			if(src->cancel != NULL)
				src->cancel(src);
	return 1;
}


/* ts B00922 */
int burn_source_read(struct burn_source *src, unsigned char *buffer, int size)
{
	int ret;

	if (src->read != NULL)
		 ret = src->read(src, buffer, size);
	else
		 ret = src->read_xt(src, buffer, size);
	return ret;
}
