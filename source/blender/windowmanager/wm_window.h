/**
 * $Id: wm_window.h
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
 * The Original Code is Copyright (C) 2007 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef WM_WINDOW_H
#define WM_WINDOW_H

struct bScreen;

/* *************** internal api ************** */
void		wm_ghost_init			(bContext *C);

wmWindow	*wm_window_new			(bContext *C, struct bScreen *screen);
void		wm_window_free			(bContext *C, wmWindow *win);
void		wm_window_add_ghostwindows	(wmWindowManager *wm);
void		wm_window_process_events	(int wait_for_event);

void		wm_window_make_drawable(bContext *C, wmWindow *win);

void		wm_window_raise			(wmWindow *win);
void		wm_window_lower			(wmWindow *win);
void		wm_window_set_size		(wmWindow *win, int width, int height);
void		wm_window_get_size		(wmWindow *win, int *width_r, int *height_r);
void		wm_window_get_position	(wmWindow *win, int *posx_r, int *posy_r);
void		wm_window_set_title		(wmWindow *win, char *title);
void		wm_window_swap_buffers	(wmWindow *win);

wmWindow	*wm_window_copy			(bContext *C, wmWindow *winorig);
wmWindow	*wm_window_rip			(bContext *C, wmWindow *winorig);

/* *************** window operators ************** */
int			wm_window_duplicate_op	(bContext *C, wmOperator *op);
int			wm_window_rip_op	(bContext *C, wmOperator *op, struct wmEvent *event);
int			wm_window_fullscreen_toggle_op(bContext *C, wmOperator *op);
int			wm_exit_blender_op(bContext *C, wmOperator *op);


#endif /* WM_WINDOW_H */

