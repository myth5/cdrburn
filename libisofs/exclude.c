/***************************************************************************
 *            exclude.c
 *
 *  Copyright  2006  Philippe Rouquier
 *  Bonfire-app@wanadoo.fr
 ***************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "hash.h"
#include "exclude.h"

static struct iso_hash_node *table[HASH_NODES]={0,};
static int num=0;

void
iso_exclude_add_path(const char *path)
{
	if (!path)
		return;

	num += iso_hash_insert(table, path);
}

void
iso_exclude_remove_path(const char *path)
{
	if (!num || !path)
		return;

	num -= iso_hash_remove(table, path);
}

void
iso_exclude_empty(void)
{
	if (!num)
		return;

	iso_hash_empty(table);
	num=0;
}

int
iso_exclude_lookup(const char *path)
{
	if (!num || !path)
		return 0;

	return iso_hash_lookup(table, path);
}
