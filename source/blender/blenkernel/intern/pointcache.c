/**
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
* Contributor(s): Campbell Barton <ideasman42@gmail.com>
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "BKE_pointcache.h"

#include "BKE_utildefines.h"
#include "BKE_global.h"
#include "BKE_library.h"

#include "BLI_blenlib.h"
#include "BKE_utildefines.h"
#include "blendef.h"

/* needed for directory lookup */
#ifndef WIN32
  #include <dirent.h>
#else
  #include "BLI_winstuff.h"
#endif

/*	Takes an Object ID and returns a unique name
	- id: object id
	- cfra: frame for the cache, can be negative
	- stack_index: index in the modifier stack. we can have cache for more then one stack_index
*/

int PTCache_path(char *filename)
{
	sprintf(filename, PTCache_PATH);
	BLI_convertstringcode(filename, G.sce, 0);
	return strlen(filename);
}

int PTCache_id_filename(struct ID *id, char *filename, int cfra, int stack_index, short do_path, short do_ext)
{
	int len=0;
	char *idname;
	char *newname;
	filename[0] = '\0';
	newname = filename;
	
	/* start with temp dir */
	if (do_path) {
		len = PTCache_path(filename);
		newname += len;
	}
	idname = (id->name+2);
	/* convert chars to hex so they are always a valid file */
	while('\0' != *idname) {
		sprintf(newname, "%02X", (char)(*idname++));
		newname+=2;
		len += 2;
	}
	
	if (do_ext) {
		sprintf(newname, "_%06d_%02d"PTCache_EXT, cfra, stack_index); /* always 6 chars */
		len += 16;
	}
	
	return len; /* make sure the above string is always 16 chars */
}

/* youll need to close yourself after! */
FILE *PTCache_id_fopen(struct ID *id, char mode, int cfra, int stack_index)
{
	/* mode is same as fopen's modes */
	FILE *fp;
	char filename[(FILE_MAXDIR+FILE_MAXFILE)*2];

	PTCache_id_filename(id, filename, cfra, stack_index, 1, 1);

	if (mode=='r') {
		if (!BLI_exists(filename)) {
			// printf("Error, file does not exist '%s'\n", filename);
			return NULL;
		}
 		fp = fopen(filename, "rb");
	} else if (mode=='w') {
		BLI_make_existing_file(filename); /* will create the dir if needs be, same as //textures is created */
		fp = fopen(filename, "wb");
	}

 	if (!fp) {
 		printf("Error creating file filename '%s'\n", filename);
 		return NULL;
 	}
 	
 	return fp;
}

/* youll need to close yourself after! */
void PTCache_id_clear(struct ID *id, int cfra, int stack_index)
{
	int len; /* store the length of the string */

	/* mode is same as fopen's modes */
	DIR *dir; 
	struct dirent *de;
	char path[FILE_MAX];
	char filename[(FILE_MAXDIR+FILE_MAXFILE)*2];
	char path_full[(FILE_MAXDIR+FILE_MAXFILE)*2];

	PTCache_path(path);
	len = PTCache_id_filename(id, filename, cfra, stack_index, 0, 0); /* no path */
	
	/* clear all files in the temp dir with the prefix of the ID and the ".bphys" suffix */
	
	dir = opendir(path);
	if (dir==NULL)
		return;
	
	while ((de = readdir(dir)) != NULL) {
		//if (S_ISREG(status.st_mode)) { /* is file */
			if (strstr(de->d_name, PTCache_EXT)) { /* do we have the right extension?*/
				if (strncmp(filename, de->d_name, len ) == 0) { /* do we have the right prefix */
					BLI_join_dirfile(path_full, path, de->d_name);
					BLI_delete(path_full, 0, 0);
				}
			}
		//}
	}
	
	closedir(dir);
	return;
}

