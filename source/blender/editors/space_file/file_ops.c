/**
 * $Id:
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
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Andrea Weikert (c) 2008 Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "BKE_context.h"
#include "BKE_screen.h"
#include "BKE_global.h"

#include "BLI_blenlib.h"
#include "BLI_storage_types.h"
#ifdef WIN32
#include "BLI_winstuff.h"
#endif
#include "DNA_space_types.h"
#include "DNA_userdef_types.h"

#include "ED_space_api.h"
#include "ED_screen.h"
#include "ED_fileselect.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "UI_view2d.h"

#include "WM_api.h"
#include "WM_types.h"

#include "file_intern.h"
#include "filelist.h"
#include "fsmenu.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* for events */
#define NOTACTIVE			0
#define ACTIVATE			1
#define INACTIVATE			2

/* ---------- FILE SELECTION ------------ */

static int find_file_mouse_hor(SpaceFile *sfile, struct ARegion* ar, short x, short y)
{
	float fx,fy;
	int offsetx, offsety;
	int columns;
	int active_file = -1;
	int numfiles = filelist_numfiles(sfile->files);
	View2D* v2d = &ar->v2d;

	UI_view2d_region_to_view(v2d, x, y, &fx, &fy);

	offsetx = (fx - (v2d->cur.xmin+sfile->tile_border_x))/(sfile->tile_w + 2*sfile->tile_border_x);
	offsety = (v2d->tot.ymax - sfile->tile_border_y - fy)/(sfile->tile_h + 2*sfile->tile_border_y);
	columns = (v2d->cur.xmax - v2d->cur.xmin) / (sfile->tile_w+ 2*sfile->tile_border_x);
	active_file = offsetx + columns*offsety;

	if ( (active_file < 0) || (active_file >= numfiles) )
	{
		active_file = -1;
	}
	return active_file;
}


static int find_file_mouse_vert(SpaceFile *sfile, struct ARegion* ar, short x, short y)
{
	int offsetx, offsety;
	float fx,fy;
	int active_file = -1;
	int numfiles = filelist_numfiles(sfile->files);
	int rows;
	View2D* v2d = &ar->v2d;

	UI_view2d_region_to_view(v2d, x, y, &fx, &fy);
	
	offsetx = (fx-sfile->tile_border_x)/(sfile->tile_w + sfile->tile_border_x);
	offsety = (v2d->cur.ymax-fy-sfile->tile_border_y)/(sfile->tile_h + sfile->tile_border_y);
	rows = (v2d->cur.ymax - v2d->cur.ymin - 2*sfile->tile_border_y) / (sfile->tile_h+sfile->tile_border_y);
	active_file = rows*offsetx + offsety;
	if ( (active_file < 0) || (active_file >= numfiles) )
	{
		active_file = -1;
	}
	return active_file;
}

static void file_deselect_all(SpaceFile* sfile)
{
	int numfiles = filelist_numfiles(sfile->files);
	int i;

	for ( i=0; i < numfiles; ++i) {
		struct direntry* file = filelist_file(sfile->files, i);
		if (file && (file->flags & ACTIVE)) {
			file->flags &= ~ACTIVE;
		}
	}
}

static void file_select(SpaceFile* sfile, FileSelectParams* params, ARegion* ar, const rcti* rect, short val)
{
	int first_file = -1;
	int last_file = -1;
	int act_file;
	short selecting = (val == LEFTMOUSE);

	int numfiles = filelist_numfiles(sfile->files);

	params->selstate = NOTACTIVE;
	if (params->display) {
		first_file = find_file_mouse_hor(sfile, ar, rect->xmin, rect->ymax);
		last_file = find_file_mouse_hor(sfile, ar, rect->xmax, rect->ymin);
	} else {
		first_file = find_file_mouse_vert(sfile, ar, rect->xmin, rect->ymax);
		last_file = find_file_mouse_vert(sfile, ar, rect->xmax, rect->ymin);
	}
	
	/* select all valid files between first and last indicated */
	if ( (first_file >= 0) && (first_file < numfiles) && (last_file >= 0) && (last_file < numfiles) ) {
		for (act_file = first_file; act_file <= last_file; act_file++) {
			struct direntry* file = filelist_file(sfile->files, act_file);
			if (selecting) 
				file->flags |= ACTIVE;
			else
				file->flags &= ~ACTIVE;
		}
	}
	
	/* make the last file active */
	if (last_file >= 0 && last_file < numfiles) {
		struct direntry* file = filelist_file(sfile->files, last_file);
		params->active_file = last_file;

		if(file && S_ISDIR(file->type)) {
			/* the path is too long and we are not going up! */
			if (strcmp(file->relname, ".") &&
				strcmp(file->relname, "..") &&
				strlen(params->dir) + strlen(file->relname) >= FILE_MAX ) 
			{
				// XXX error("Path too long, cannot enter this directory");
			} else {
				if (strcmp(file->relname, "..")==0) {
					/* avoids /../../ */
					BLI_parent_dir(params->dir);
				} else {
					strcat(params->dir, file->relname);
					strcat(params->dir,"/");
					params->file[0] = '\0';
					BLI_cleanup_dir(G.sce, params->dir);
				}
				filelist_setdir(sfile->files, params->dir);
				filelist_free(sfile->files);
				params->active_file = -1;
			}
		}
		else if (file)
		{
			if (file->relname) {
				BLI_strncpy(params->file, file->relname, FILE_MAXFILE);
				/* XXX
				if(event==MIDDLEMOUSE && filelist_gettype(sfile->files)) 
					imasel_execute(sfile);
				*/
			}
			
		}	
		/* XXX
		if(BIF_filelist_gettype(sfile->files)==FILE_MAIN) {
			active_imasel_object(sfile);
		}
		*/
	}
}



static int file_border_select_exec(bContext *C, wmOperator *op)
{
	ARegion *ar= CTX_wm_region(C);
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	short val;
	rcti rect;

	val= RNA_int_get(op->ptr, "event_type");
	rect.xmin= RNA_int_get(op->ptr, "xmin");
	rect.ymin= RNA_int_get(op->ptr, "ymin");
	rect.xmax= RNA_int_get(op->ptr, "xmax");
	rect.ymax= RNA_int_get(op->ptr, "ymax");

	file_select(sfile, sfile->params, ar, &rect, val );
	WM_event_add_notifier(C, NC_WINDOW, NULL);
	return OPERATOR_FINISHED;
}

void ED_FILE_OT_border_select(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Activate/Select File";
	ot->idname= "ED_FILE_OT_border_select";
	
	/* api callbacks */
	ot->invoke= WM_border_select_invoke;
	ot->exec= file_border_select_exec;
	ot->modal= WM_border_select_modal;

	/* rna */
	RNA_def_int(ot->srna, "event_type", 0, INT_MIN, INT_MAX, "Event Type", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "xmin", 0, INT_MIN, INT_MAX, "X Min", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "xmax", 0, INT_MIN, INT_MAX, "X Max", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "ymin", 0, INT_MIN, INT_MAX, "Y Min", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "ymax", 0, INT_MIN, INT_MAX, "Y Max", "", INT_MIN, INT_MAX);

	ot->poll= ED_operator_file_active;
}

static int file_select_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	ARegion *ar= CTX_wm_region(C);
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	short val;
	rcti rect;

	rect.xmin = rect.xmax = event->x - ar->winrct.xmin;
	rect.ymin = rect.ymax = event->y - ar->winrct.ymin;
	val = event->val;

	/* single select, deselect all selected first */
	file_deselect_all(sfile);
	file_select(sfile, sfile->params, ar, &rect, val );
	WM_event_add_notifier(C, NC_WINDOW, NULL);
	return OPERATOR_FINISHED;
}

void ED_FILE_OT_select(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Activate/Select File";
	ot->idname= "ED_FILE_OT_select";
	
	/* api callbacks */
	ot->invoke= file_select_invoke;

	/* rna */

	ot->poll= ED_operator_file_active;
}

static int file_select_all_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	ScrArea *sa= CTX_wm_area(C);
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	int numfiles = filelist_numfiles(sfile->files);
	int i;
	int select = 1;

	/* if any file is selected, deselect all first */
	for ( i=0; i < numfiles; ++i) {
		struct direntry* file = filelist_file(sfile->files, i);
		if (file && (file->flags & ACTIVE)) {
			file->flags &= ~ACTIVE;
			select = 0;
			ED_area_tag_redraw(sa);
		}
	}
	/* select all only if previously no file was selected */
	if (select) {
		for ( i=0; i < numfiles; ++i) {
			struct direntry* file = filelist_file(sfile->files, i);
			if(file && !S_ISDIR(file->type)) {
				file->flags |= ACTIVE;
				ED_area_tag_redraw(sa);
			}
		}
	}
	return OPERATOR_FINISHED;
}

void ED_FILE_OT_select_all(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Select/Deselect all files";
	ot->idname= "ED_FILE_OT_select_all";
	
	/* api callbacks */
	ot->invoke= file_select_all_invoke;

	/* rna */

	ot->poll= ED_operator_file_active;
}

/* ---------- BOOKMARKS ----------- */

static void set_active_bookmark(FileSelectParams* params, struct ARegion* ar, short x, short y)
{
	int nentries = fsmenu_get_nentries();
	float fx, fy;
	short posy;

	UI_view2d_region_to_view(&ar->v2d, x, y, &fx, &fy);

	posy = ar->v2d.cur.ymax - 2*TILE_BORDER_Y - fy;
	params->active_bookmark = ((float)posy / (U.fontsize*3.0f/2.0f));
	if (params->active_bookmark < 0 || params->active_bookmark > nentries) {
		params->active_bookmark = -1;
	}
}

static void file_select_bookmark(SpaceFile* sfile, ARegion* ar, short x, short y)
{
	if (BLI_in_rcti(&ar->v2d.mask, x, y)) {
		char *selected;
		set_active_bookmark(sfile->params, ar, x, y);
		selected= fsmenu_get_entry(sfile->params->active_bookmark);			
		/* which string */
		if (selected) {
			FileSelectParams* params = sfile->params;
			BLI_strncpy(params->dir, selected, sizeof(params->dir));
			BLI_cleanup_dir(G.sce, params->dir);
			filelist_free(sfile->files);	
			filelist_setdir(sfile->files, params->dir);
			params->file[0] = '\0';			
			params->active_file = -1;
		}
	}
}

static int bookmark_select_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	ScrArea *sa= CTX_wm_area(C);
	ARegion *ar= CTX_wm_region(C);	
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);

	short x, y;

	x = event->x - ar->winrct.xmin;
	y = event->y - ar->winrct.ymin;

	file_select_bookmark(sfile, ar, x, y);
	ED_area_tag_redraw(sa);
	return OPERATOR_FINISHED;
}

void ED_FILE_OT_select_bookmark(wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Select Directory";
	ot->idname= "ED_FILE_OT_select_bookmark";
	
	/* api callbacks */
	ot->invoke= bookmark_select_invoke;
	ot->poll= ED_operator_file_active;
}

static int loadimages_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	ScrArea *sa= CTX_wm_area(C);
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	if (sfile->files) {
		filelist_loadimage_timer(sfile->files);
		if (filelist_changed(sfile->files)) {
			ED_area_tag_redraw(sa);
		}
	}

	return OPERATOR_FINISHED;
}

void ED_FILE_OT_loadimages(wmOperatorType *ot)
{
	
	/* identifiers */
	ot->name= "Load Images";
	ot->idname= "ED_FILE_OT_loadimages";
	
	/* api callbacks */
	ot->invoke= loadimages_invoke;
	
	ot->poll= ED_operator_file_active;
}

int file_hilight_set(SpaceFile *sfile, ARegion *ar, int mx, int my)
{
	FileSelectParams* params;
	int numfiles, actfile;
	
	if(sfile==NULL || sfile->files==NULL) return 0;
	
	numfiles = filelist_numfiles(sfile->files);
	params = ED_fileselect_get_params(sfile);
	
	if (params->display) {
		actfile = find_file_mouse_hor(sfile, ar, mx , my);
	} else {
		actfile = find_file_mouse_vert(sfile, ar, mx, my);
	}
	
	if (actfile >= 0 && actfile < numfiles ) {
		params->active_file=actfile;
		return 1;
	}
	return 0;
}

static int file_highlight_invoke(bContext *C, wmOperator *op, wmEvent *event)
{
	ARegion *ar= CTX_wm_region(C);
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	
	if( file_hilight_set(sfile, ar, event->x - ar->winrct.xmin, event->y - ar->winrct.ymin)) {
		ED_area_tag_redraw(CTX_wm_area(C));
	}
	
	return OPERATOR_FINISHED;
}

void ED_FILE_OT_highlight(struct wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Highlight File";
	ot->idname= "ED_FILE_OT_highlight";
	
	/* api callbacks */
	ot->invoke= file_highlight_invoke;
	ot->poll= ED_operator_file_active;
}

int file_cancel_exec(bContext *C, wmOperator *unused)
{
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	
	if(sfile->op) {
		WM_operator_free(sfile->op);
		sfile->op = NULL;
	}
	ED_screen_full_prevspace(C);
	
	return OPERATOR_FINISHED;
}

void ED_FILE_OT_cancel(struct wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Cancel File Load";
	ot->idname= "ED_FILE_OT_cancel";
	
	/* api callbacks */
	ot->exec= file_cancel_exec;
	ot->poll= ED_operator_file_active;
}


int file_load_exec(bContext *C, wmOperator *unused)
{
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	char name[FILE_MAX];
	
	ED_screen_full_prevspace(C);
	
	if(sfile->op) {
		wmOperator *op= sfile->op;
		
		/* if load .blend, all UI pointers after exec are invalid! */
		/* but, operator can be freed still */
		
		sfile->op = NULL;
		BLI_strncpy(name, sfile->params->dir, sizeof(name));
		strcat(name, sfile->params->file);
		RNA_string_set(op->ptr, "filename", name);
		
		op->type->exec(C, op);
		
		WM_operator_free(op);
	}
				
	return OPERATOR_FINISHED;
}

void ED_FILE_OT_load(struct wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Load File";
	ot->idname= "ED_FILE_OT_load";
	
	/* api callbacks */
	ot->exec= file_load_exec;
	ot->poll= ED_operator_file_active; /* <- important, handler is on window level */
}

int file_parent_exec(bContext *C, wmOperator *unused)
{
	SpaceFile *sfile= (SpaceFile*)CTX_wm_space_data(C);
	
	if(sfile->params) {
		BLI_parent_dir(sfile->params->dir);
		filelist_setdir(sfile->files, sfile->params->dir);
		filelist_free(sfile->files);
		sfile->params->active_file = -1;
	}		
	ED_area_tag_redraw(CTX_wm_area(C));

	return OPERATOR_FINISHED;

}

void ED_FILE_OT_parent(struct wmOperatorType *ot)
{
	/* identifiers */
	ot->name= "Parent File";
	ot->idname= "ED_FILE_OT_parent";
	
	/* api callbacks */
	ot->exec= file_parent_exec;
	ot->poll= ED_operator_file_active; /* <- important, handler is on window level */
}



