/**
 * $Id$
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef BIF_DRAWTEXT_H
#define BIF_DRAWTEXT_H

struct ScrArea;
struct SpaceText;
struct Text;

void unlink_text(struct Text *text);

void free_textspace(struct SpaceText *st);

int txt_file_modified(struct Text *text);
void txt_write_file(struct Text *text);
void add_text_fs(char *file);

void free_txt_data(void);
void pop_space_text(struct SpaceText *st);

void get_format_string(struct SpaceText *st);
void do_brackets(void);

#endif

