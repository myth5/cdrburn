/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include "init.h"
#include "sg.h"
#include "error.h"
#include "libburn.h"
#include "drive.h"

int burn_running = 0;

int burn_initialize(void)
{
	if (burn_running)
		return 1;

	burn_running = 1;
	return 1;
}

void burn_finish(void)
{
	assert(burn_running);

	burn_wait_all();

	burn_drive_free();

	burn_running = 0;
}
