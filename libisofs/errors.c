/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include "errors.h"
#include <stdio.h>

void iso_warn(enum iso_warnings w)
{
	printf("WARNING: %u\n", w);
}

void iso_error(enum iso_errors e)
{
	printf("ERROR: %u\n", e);
}
