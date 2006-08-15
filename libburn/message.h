/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#ifndef __MESSAGE
#define __MESSAGE

#include "libburn.h"

void burn_message_clear_queue(void);

void burn_message_info_new(struct burn_drive *drive,
			   enum burn_message_info message);

void burn_message_warning_new(struct burn_drive *drive,
			      enum burn_message_info message);

void burn_message_error_new(struct burn_drive *drive,
			    enum burn_message_info message);

#endif
