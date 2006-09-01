/***************************************************************************
 *            hash.c
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

#include <stdlib.h>
#include <string.h>

#include "hash.h"

static unsigned int
iso_hash_path(const char *path)
{
	unsigned int hash_num=0;
	const char *c;

	c=path;
	while(*c)
		hash_num = (hash_num << 15) + (hash_num << 3) + (hash_num >> 3) + *c++;

	return hash_num % HASH_NODES;
}

int
iso_hash_lookup(struct iso_hash_node **table, const char *path)
{
	struct iso_hash_node *node;
	unsigned int hash_num;

	hash_num = iso_hash_path(path);

	node=table[hash_num];

	if (!node)
		return 0;

	if (!strcmp(path, node->path))
		return 1;	

	while (node->next) {
		node=node->next;

		if (!strcmp(path, node->path))
			return 1;
	}

	return 0;
 }

static struct iso_hash_node*
iso_hash_node_new (const char *path)
{
	struct iso_hash_node *node;

	/*create an element to be inserted in the hash table */
	node=malloc(sizeof(struct iso_hash_node));
	node->path=strdup(path);
	node->next=NULL;

	return node;
}

int
iso_hash_insert(struct iso_hash_node **table, const char *path)
{
	struct iso_hash_node *node;
	unsigned int hash_num;

	/* find the hash number */
	hash_num = iso_hash_path(path);

	/* insert it */
	node = table[hash_num];

	/* unfortunately, we can't safely consider that a path
	 * won't be twice in the hash table so make sure it
	 * doesn't already exists */
	if (!node) {
		table[hash_num]=iso_hash_node_new(path);
		return 1;
	}

	/* if it's already in, we don't do anything */
	if (!strcmp(path, node->path))
		return 0;

	while (node->next) {
		node = node->next;

		/* if it's already in, we don't do anything */
		if (!strcmp (path, node->path))
			return 0;
	}

	node->next = iso_hash_node_new(path);
	return 1;
}

static void
iso_hash_node_free(struct iso_hash_node *node)
{
	free(node->path);
	free(node);
}

int
iso_hash_remove(struct iso_hash_node **table,  const char *path)
{
	unsigned int hash_num;
	struct iso_hash_node *node;

	hash_num = iso_hash_path(path);

	node=table[hash_num];
	if (!node)
		return 0;

	if (!strcmp(path, node->path)) {
		table[hash_num]=node->next;
		iso_hash_node_free(node);
		return 1;	
	}

	while (node->next) {
		struct iso_hash_node *prev;

		prev = node;
		node = node->next;

		if (!strcmp (path, node->path)) {
			prev->next=node->next;
			iso_hash_node_free(node);
			return 1;
		}
	}

	return 0;
}

void 
iso_hash_empty(struct iso_hash_node **table)
{
	int i;

	for (i=0; i < HASH_NODES; i++) {
		struct iso_hash_node *node;

		node=table[i];
		if (!node)
			continue;

		table[i]=NULL;

		do {
			struct iso_hash_node *next;

			next=node->next;
			iso_hash_node_free(node);
			node=next;
		} while (node);
	}
}
 
