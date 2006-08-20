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

/* ts A60813 : wether to use O_EXCL and/or O_NONBLOCK in libburn/sg.c */
int burn_sg_open_o_excl = 1;

/* O_NONBLOCK was hardcoded in enumerate_ata() which i hardly use.
   For enumerate_sg() it seems ok.
   So it should stay default mode until enumerate_ata() without O_NONBLOCK
   has been thoroughly tested. */
int burn_sg_open_o_nonblock = 1;

/* wether to take a busy drive as an error */
/* Caution: this is implemented by a rough hack and eventually leads
	    to unconditional abort of the process  */
int burn_sg_open_abort_busy = 0;

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


/* ts A60813 */
/** Set parameters for behavior on opening device files. To be called early
    after burn_initialize() and before any bus scan. But not mandatory at all.
    @param exclusive Try to open only devices which are not marked as busy
                     and try to mark them busy if opened sucessfully. (O_EXCL)
                     There are kernels which simply don't care about O_EXCL.
                     Some have it off, some have it on, some are switchable.
    @param blocking  Try to wait for drives which do not open immediately but
                     also do not return an error as well. (O_NONBLOCK)
                     This might stall indefinitely with /dev/hdX hard disks.
    @param abort_on_busy  Unconditionally abort process when a non blocking
                          exclusive opening attempt indicates a busy drive.
                          Use this only after thorough tests with your app.
    Parameter value 1 enables a feature, 0 disables.
    Default is (1,0,0). Have a good reason before you change it.
*/
void burn_preset_device_open(int exclusive, int blocking, int abort_on_busy)
{
	assert(burn_running);

	burn_sg_open_o_excl= !!exclusive;
	burn_sg_open_o_nonblock= !blocking;
	burn_sg_open_abort_busy= !!abort_on_busy;
}

