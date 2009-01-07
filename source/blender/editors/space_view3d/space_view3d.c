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
#include "DNA_view3d_types.h"

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"
#include "BLI_rand.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_screen.h"
#include "BKE_utildefines.h"

#include "ED_space_api.h"
#include "ED_screen.h"

#include "BIF_gl.h"

#include "WM_api.h"
#include "WM_types.h"

#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"

#include "view3d_intern.h"	// own include

/* ******************** default callbacks for view3d space ***************** */

static SpaceLink *view3d_new(const bContext *C)
{
	Scene *scene= CTX_data_scene(C);
	ARegion *ar;
	View3D *vd;
	
	vd= MEM_callocN(sizeof(View3D), "initview3d");
	vd->spacetype= SPACE_VIEW3D;
	vd->blockscale= 0.7f;
	vd->lay= vd->layact= 1;
	if(scene) {
		vd->lay= vd->layact= scene->lay;
		vd->camera= scene->camera;
	}
	vd->scenelock= 1;
	vd->grid= 1.0f;
	vd->gridlines= 16;
	vd->gridsubdiv = 10;
	vd->drawtype= OB_WIRE;
	
	vd->gridflag |= V3D_SHOW_X;
	vd->gridflag |= V3D_SHOW_Y;
	vd->gridflag |= V3D_SHOW_FLOOR;
	vd->gridflag &= ~V3D_SHOW_Z;
	
	vd->depths= NULL;
	
	/* XXX move view data to region? */
	vd->viewquat[0]= 1.0f;
	vd->viewquat[1]= vd->viewquat[2]= vd->viewquat[3]= 0.0f;
	vd->persp= 1;
	vd->view= 7;
	vd->dist= 10.0;
	vd->lens= 35.0f;
	vd->near= 0.01f;
	vd->far= 500.0f;
	
	/* header */
	ar= MEM_callocN(sizeof(ARegion), "header for view3d");
	
	BLI_addtail(&vd->regionbase, ar);
	ar->regiontype= RGN_TYPE_HEADER;
	ar->alignment= RGN_ALIGN_BOTTOM;
	
	/* main area */
	ar= MEM_callocN(sizeof(ARegion), "main area for view3d");
	
	BLI_addtail(&vd->regionbase, ar);
	ar->regiontype= RGN_TYPE_WINDOW;
	
	return (SpaceLink *)vd;
}

/* not spacelink itself */
static void view3d_free(SpaceLink *sl)
{
	View3D *vd= (View3D *) sl;
	
	if(vd->bgpic) {
		if(vd->bgpic->ima) vd->bgpic->ima->id.us--;
		MEM_freeN(vd->bgpic);
	}
	
	if(vd->localvd) MEM_freeN(vd->localvd);
	if(vd->clipbb) MEM_freeN(vd->clipbb);
	if(vd->depths) {
		if(vd->depths->depths) MEM_freeN(vd->depths->depths);
		MEM_freeN(vd->depths);
		vd->depths= NULL;
	}
	
// XXX	retopo_free_view_data(vd);
	
	if(vd->properties_storage) MEM_freeN(vd->properties_storage);
	if(vd->ri) { 
// XXX		BIF_view3d_previewrender_free(vd);
	}
	
}


/* spacetype; init callback */
static void view3d_init(struct wmWindowManager *wm, ScrArea *sa)
{

}

static SpaceLink *view3d_duplicate(SpaceLink *sl)
{
	View3D *v3do= (View3D *)sl;
	View3D *v3dn= MEM_dupallocN(sl);
	
	/* clear or remove stuff from old */
	
// XXX	BIF_view3d_previewrender_free(v3do);
	v3do->depths= NULL;
	v3do->retopo_view_data= NULL;
	
	if(v3do->localvd) {
// XXX		restore_localviewdata(v3do);
		v3do->localvd= NULL;
		v3do->properties_storage= NULL;
		v3do->localview= 0;
		v3do->lay &= 0xFFFFFF;
	}
	
	/* copy or clear inside new stuff */

	if(v3dn->bgpic) {
		v3dn->bgpic= MEM_dupallocN(v3dn->bgpic);
		if(v3dn->bgpic->ima) v3dn->bgpic->ima->id.us++;
	}
	v3dn->clipbb= MEM_dupallocN(v3dn->clipbb);
	v3dn->ri= NULL;
	v3dn->properties_storage= NULL;
	
	return (SpaceLink *)v3dn;
}

static void view3d_main_area_draw(const bContext *C, ARegion *ar)
{
	/* draw entirely, view changes should be handled here */
	ScrArea *sa= CTX_wm_area(C);
	View3D *v3d= sa->spacedata.first;	/* XXX get from region */
	
	drawview3dspace(CTX_wm_screen(C), CTX_data_scene(C), ar, v3d);
}

/* add handlers, stuff you only do once or on area/region changes */
static void view3d_main_area_init(wmWindowManager *wm, ARegion *ar)
{
	ListBase *keymap;
	
	/* own keymap */
	keymap= WM_keymap_listbase(wm, "View3D", SPACE_VIEW3D, 0);
	WM_event_add_keymap_handler(&ar->handlers, keymap);
	
	/* object ops. */
	keymap= WM_keymap_listbase(wm, "Object Non-modal", 0, 0);
	WM_event_add_keymap_handler(&ar->handlers, keymap);
	
	/* object modal ops default */
	keymap= WM_keymap_listbase(wm, "Object Mode", 0, 0);
	WM_event_add_keymap_handler(&ar->handlers, keymap);
	
}

static void view3d_modal_keymaps(wmWindowManager *wm, ARegion *ar, int stype)
{
	ListBase *keymap;
	
	keymap= WM_keymap_listbase(wm, "Object Mode", 0, 0);
	if(stype==NS_MODE_OBJECT)
		WM_event_add_keymap_handler(&ar->handlers, keymap);
	else
		WM_event_remove_keymap_handler(&ar->handlers, keymap);
	
	
	keymap= WM_keymap_listbase(wm, "EditMesh", 0, 0);
	if(stype==NS_EDITMODE_MESH)
		WM_event_add_keymap_handler(&ar->handlers, keymap);
	else
		WM_event_remove_keymap_handler(&ar->handlers, keymap);
	
}

static void view3d_main_area_listener(ARegion *ar, wmNotifier *wmn)
{
	/* context changes */
	switch(wmn->category) {
		case NC_SCENE:
			switch(wmn->data) {
				case ND_FRAME:
				case ND_OB_ACTIVE:
				case ND_OB_SELECT:
					ED_region_tag_redraw(ar);
					break;
				case ND_MODE:
					view3d_modal_keymaps(wmn->wm, ar, wmn->subtype);
					ED_region_tag_redraw(ar);
					break;
			}
			break;
		case NC_OBJECT:
			switch(wmn->data) {
				case ND_BONE_ACTIVE:
				case ND_BONE_SELECT:
				case ND_TRANSFORM:
				case ND_GEOM_SELECT:
				case ND_DRAW:
				case ND_MODIFIER:
					ED_region_tag_redraw(ar);
					break;
			}
		case NC_MATERIAL:
			switch(wmn->data) {
				case ND_SHADING_DRAW:
					ED_region_tag_redraw(ar);
					break;
			}
		case NC_LAMP:
			switch(wmn->data) {
				case ND_LIGHTING_DRAW:
					ED_region_tag_redraw(ar);
					break;
			}
	}
}

/* concept is to retrieve cursor type context-less */
static void view3d_main_area_cursor(wmWindow *win, ScrArea *sa, ARegion *ar)
{
	Scene *scene= win->screen->scene;

	if(scene->obedit) {
		WM_cursor_set(win, CURSOR_EDIT);
	}
	else {
		WM_cursor_set(win, CURSOR_STD);
	}
}

/* add handlers, stuff you only do once or on area/region changes */
static void view3d_header_area_init(wmWindowManager *wm, ARegion *ar)
{
	UI_view2d_region_reinit(&ar->v2d, V2D_COMMONVIEW_HEADER, ar->winx, ar->winy);
}

static void view3d_header_area_draw(const bContext *C, ARegion *ar)
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
	
	view3d_header_buttons(C, ar);
	
	/* restore view matrix? */
	UI_view2d_view_restore(C);
}

static void view3d_header_area_listener(ARegion *ar, wmNotifier *wmn)
{
	/* context changes */
	switch(wmn->category) {
		case NC_SCENE:
			switch(wmn->data) {
				case ND_FRAME:
				case ND_OB_ACTIVE:
				case ND_OB_SELECT:
				case ND_MODE:
					ED_region_tag_redraw(ar);
					break;
			}
			break;
	}
}


static int view3d_context(const bContext *C, const bContextDataMember *member, bContextDataResult *result)
{
	View3D *v3d= (View3D*)CTX_wm_space_data(C);
	Scene *scene= CTX_data_scene(C);
	Base *base;

	if(ELEM(member, CTX_data_selected_objects, CTX_data_selected_bases)) {
		for(base=scene->base.first; base; base=base->next) {
			if((base->flag & SELECT) && (base->lay & v3d->lay)) {
				if((base->object->restrictflag & OB_RESTRICT_VIEW)==0) {
					if(member == CTX_data_selected_objects)
						CTX_data_list_add(result, base->object);
					else
						CTX_data_list_add(result, base);
				}
			}
		}

		return 1;
	}
	else if(ELEM(member, CTX_data_visible_objects, CTX_data_visible_bases)) {
		for(base=scene->base.first; base; base=base->next) {
			if(base->lay & v3d->lay) {
				if((base->object->restrictflag & OB_RESTRICT_VIEW)==0) {
					if(member == CTX_data_selected_objects)
						CTX_data_list_add(result, base->object);
					else
						CTX_data_list_add(result, base);
				}
			}
		}
		
		return 1;
	}
	else if(member == CTX_data_active_base) {
		if(scene->basact && (scene->basact->lay & v3d->lay))
			if((scene->basact->object->restrictflag & OB_RESTRICT_VIEW)==0)
				CTX_data_pointer_set(result, scene->basact);

		return 1;
	}
	else if(member == CTX_data_active_object) {
		if(scene->basact && (scene->basact->lay & v3d->lay))
			if((scene->basact->object->restrictflag & OB_RESTRICT_VIEW)==0)
				CTX_data_pointer_set(result, scene->basact->object);

		return 1;
	}

	return 0;
}

/* only called once, from space/spacetypes.c */
void ED_spacetype_view3d(void)
{
	SpaceType *st= MEM_callocN(sizeof(SpaceType), "spacetype view3d");
	ARegionType *art;
	
	st->spaceid= SPACE_VIEW3D;
	
	st->new= view3d_new;
	st->free= view3d_free;
	st->init= view3d_init;
	st->duplicate= view3d_duplicate;
	st->operatortypes= view3d_operatortypes;
	st->keymap= view3d_keymap;
	st->context= view3d_context;
	
	/* regions: main window */
	art= MEM_callocN(sizeof(ARegionType), "spacetype view3d region");
	art->regionid = RGN_TYPE_WINDOW;
	art->draw= view3d_main_area_draw;
	art->init= view3d_main_area_init;
	art->listener= view3d_main_area_listener;
	art->cursor= view3d_main_area_cursor;
	BLI_addhead(&st->regiontypes, art);
	
	/* regions: header */
	art= MEM_callocN(sizeof(ARegionType), "spacetype view3d region");
	art->regionid = RGN_TYPE_HEADER;
	art->minsizey= HEADERY;
	art->keymapflag= ED_KEYMAP_UI|ED_KEYMAP_VIEW2D;
	art->listener= view3d_header_area_listener;
	
	art->init= view3d_header_area_init;
	art->draw= view3d_header_area_draw;
	
	BLI_addhead(&st->regiontypes, art);
	
	BKE_spacetype_register(st);
}

