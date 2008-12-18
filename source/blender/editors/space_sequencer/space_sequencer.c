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
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <string.h>
#include <stdio.h>

#include "DNA_object_types.h"
#include "DNA_space_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"
#include "BLI_rand.h"

#include "BKE_colortools.h"
#include "BKE_context.h"
#include "BKE_screen.h"

#include "ED_space_api.h"
#include "ED_screen.h"

#include "BIF_gl.h"

#include "WM_api.h"
#include "WM_types.h"

#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"

#include "ED_markers.h"

#include "sequencer_intern.h"	// own include

/* ******************** default callbacks for sequencer space ***************** */

static SpaceLink *sequencer_new(void)
{
	ARegion *ar;
	SpaceSeq *sseq;
	
	sseq= MEM_callocN(sizeof(SpaceSeq), "initsequencer");
	sseq->spacetype= SPACE_SEQ;
	sseq->zoom= 4;
	sseq->chanshown = 0;
	
	
	/* header */
	ar= MEM_callocN(sizeof(ARegion), "header for sequencer");
	
	BLI_addtail(&sseq->regionbase, ar);
	ar->regiontype= RGN_TYPE_HEADER;
	ar->alignment= RGN_ALIGN_BOTTOM;
	
	/* main area */
	ar= MEM_callocN(sizeof(ARegion), "main area for sequencer");
	
	BLI_addtail(&sseq->regionbase, ar);
	ar->regiontype= RGN_TYPE_WINDOW;
	
	
	/* seq space goes from (0,8) to (250, 0) */
	
	ar->v2d.tot.xmin= 0.0f;
	ar->v2d.tot.ymin= 0.0f;
	ar->v2d.tot.xmax= 250.0f;
	ar->v2d.tot.ymax= 8.0f;
	
	ar->v2d.cur= ar->v2d.tot;
	
	ar->v2d.min[0]= 10.0f;
	ar->v2d.min[1]= 4.0f;
	
	ar->v2d.max[0]= MAXFRAMEF;
	ar->v2d.max[1]= MAXSEQ;
	
	ar->v2d.minzoom= 0.01f;
	ar->v2d.maxzoom= 100.0f;
	
	ar->v2d.scroll |= (V2D_SCROLL_BOTTOM|V2D_SCROLL_SCALE_HORIZONTAL);
	ar->v2d.scroll |= (V2D_SCROLL_LEFT|V2D_SCROLL_SCALE_VERTICAL);
	ar->v2d.keepzoom= 0;
	ar->v2d.keeptot= 0;
	
	
	
	return (SpaceLink *)sseq;
}

/* not spacelink itself */
static void sequencer_free(SpaceLink *sl)
{	
//	SpaceSeq *sseq= (SpaceSequencer*) sl;
	
// XXX	if(sseq->gpd) free_gpencil_data(sseq->gpd);

}


/* spacetype; init callback */
static void sequencer_init(struct wmWindowManager *wm, ScrArea *sa)
{

}

static SpaceLink *sequencer_duplicate(SpaceLink *sl)
{
	SpaceSeq *sseqn= MEM_dupallocN(sl);
	
	/* clear or remove stuff from old */
// XXX	sseq->gpd= gpencil_data_duplicate(sseq->gpd);

	return (SpaceLink *)sseqn;
}



/* add handlers, stuff you only do once or on area/region changes */
static void sequencer_main_area_init(wmWindowManager *wm, ARegion *ar)
{
	ListBase *keymap;
	
	UI_view2d_region_reinit(&ar->v2d, V2D_COMMONVIEW_CUSTOM, ar->winx, ar->winy);
	
	/* own keymap */
	keymap= WM_keymap_listbase(wm, "Sequencer", SPACE_SEQ, 0);	/* XXX weak? */
	WM_event_add_keymap_handler_bb(&ar->handlers, keymap, &ar->v2d.mask, &ar->winrct);
}

static void sequencer_main_area_draw(const bContext *C, ARegion *ar)
{
	/* draw entirely, view changes should be handled here */
	// SpaceSeq *sseq= (SpaceSeq*)CTX_wm_space_data(C);
	View2D *v2d= &ar->v2d;
	float col[3];
	
	/* clear and setup matrix */
	UI_GetThemeColor3fv(TH_BACK, col);
	glClearColor(col[0], col[1], col[2], 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	UI_view2d_view_ortho(C, v2d);
		
	/* data... */
	
	
	/* reset view matrix */
	UI_view2d_view_restore(C);
	
	/* scrollers? */
}

void sequencer_operatortypes(void)
{
	
}

void sequencer_keymap(struct wmWindowManager *wm)
{
	
}

/* add handlers, stuff you only do once or on area/region changes */
static void sequencer_header_area_init(wmWindowManager *wm, ARegion *ar)
{
	UI_view2d_region_reinit(&ar->v2d, V2D_COMMONVIEW_HEADER, ar->winx, ar->winy);
}

static void sequencer_header_area_draw(const bContext *C, ARegion *ar)
{
	float col[3];
	
	/* clear */
	if(ED_screen_area_active(C))
		UI_GetThemeColor3fv(TH_HEADER, col);
	else
		UI_GetThemeColor3fv(TH_HEADERDESEL, col);
	
	glClearColor(col[0], col[1], col[2], 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	/* set view2d view matrix for scrolling (without scrollers) */
	UI_view2d_view_ortho(C, &ar->v2d);
	
	sequencer_header_buttons(C, ar);
	
	/* restore view matrix? */
	UI_view2d_view_restore(C);
}

static void sequencer_main_area_listener(ARegion *ar, wmNotifier *wmn)
{
	/* context changes */
}

/* only called once, from space/spacetypes.c */
void ED_spacetype_sequencer(void)
{
	SpaceType *st= MEM_callocN(sizeof(SpaceType), "spacetype sequencer");
	ARegionType *art;
	
	st->spaceid= SPACE_SEQ;
	
	st->new= sequencer_new;
	st->free= sequencer_free;
	st->init= sequencer_init;
	st->duplicate= sequencer_duplicate;
	st->operatortypes= sequencer_operatortypes;
	st->keymap= sequencer_keymap;
	
	/* regions: main window */
	art= MEM_callocN(sizeof(ARegionType), "spacetype sequencer region");
	art->regionid = RGN_TYPE_WINDOW;
	art->init= sequencer_main_area_init;
	art->draw= sequencer_main_area_draw;
	art->listener= sequencer_main_area_listener;
	art->keymapflag= ED_KEYMAP_VIEW2D;

	BLI_addhead(&st->regiontypes, art);
	
	/* regions: header */
	art= MEM_callocN(sizeof(ARegionType), "spacetype sequencer region");
	art->regionid = RGN_TYPE_HEADER;
	art->minsizey= HEADERY;
	art->keymapflag= ED_KEYMAP_UI|ED_KEYMAP_VIEW2D;
	
	art->init= sequencer_header_area_init;
	art->draw= sequencer_header_area_draw;
	
	BLI_addhead(&st->regiontypes, art);
	
	
	BKE_spacetype_register(st);
}

