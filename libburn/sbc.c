/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

/* scsi block commands */

#include <scsi/scsi.h>
#include <string.h>
#include <scsi/sg.h>

#include "transport.h"
#include "sbc.h"
#include "options.h"

/* spc command set */
static char SBC_LOAD[] = { 0x1b, 0, 0, 0, 3, 0 };
static char SBC_UNLOAD[] = { 0x1b, 0, 0, 0, 2, 0 };

void sbc_load(struct burn_drive *d)
{
	struct command c;

	memcpy(c.opcode, SBC_LOAD, sizeof(SBC_LOAD));
	c.retry = 1;
	c.oplen = sizeof(SBC_LOAD);
	c.dir = NO_TRANSFER;
	c.page = NULL;
	d->issue_command(d, &c);
}

void sbc_eject(struct burn_drive *d)
{
	struct command c;

	c.page = NULL;
	memcpy(c.opcode, SBC_UNLOAD, sizeof(SBC_UNLOAD));
	c.oplen = 1;
	c.oplen = sizeof(SBC_UNLOAD);
	c.page = NULL;
	c.dir = NO_TRANSFER;
	d->issue_command(d, &c);
}
