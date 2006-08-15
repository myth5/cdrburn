/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __ERRORS
#define __ERRORS

enum iso_warnings
{
	ISO_WARNING_FOO
};

enum iso_errors
{
	ISO_ERROR_FOO
};

void iso_warn(enum iso_warnings w);
void iso_error(enum iso_errors e);

#endif /* __ERRORS */
