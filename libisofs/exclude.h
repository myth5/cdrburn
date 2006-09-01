/***************************************************************************
 *            exclude.h
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

#ifndef ISO_EXCLUDE_H
#define ISO_EXCLUDE_H

/**
 * Add a path to ignore when adding a directory recursively.
 *
 * \param path The path, on the local filesystem, of the file.
 */
int
iso_exclude_lookup(const char *path);

#endif /* ISO_EXCLUDE */
