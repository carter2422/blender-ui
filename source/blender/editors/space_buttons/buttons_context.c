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
 * The Original Code is Copyright (C) 2009 Blender Foundation.
 * All rights reserved.
 *
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>
#include <string.h>

#include "MEM_guardedalloc.h"

#include "DNA_armature_types.h"
#include "DNA_lamp_types.h"
#include "DNA_material_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_particle_types.h"
#include "DNA_texture_types.h"
#include "DNA_world_types.h"

#include "BLI_listbase.h"

#include "BKE_context.h"
#include "BKE_material.h"
#include "BKE_modifier.h"
#include "BKE_particle.h"
#include "BKE_screen.h"
#include "BKE_utildefines.h"
#include "BKE_world.h"

#include "RNA_access.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "buttons_intern.h"	// own include

typedef struct ButsContextPath {
	PointerRNA ptr[8];
	int len;
	int worldtex;
} ButsContextPath;

static int set_pointer_type(ButsContextPath *path, bContextDataResult *result, StructRNA *type)
{
	PointerRNA *ptr;
	int a;

	for(a=0; a<path->len; a++) {
		ptr= &path->ptr[a];

		if(RNA_struct_is_a(ptr->type, type)) {
			CTX_data_pointer_set(result, ptr->id.data, ptr->type, ptr->data);
			return 1;
		}
	}

	return 0;
}

static PointerRNA *get_pointer_type(ButsContextPath *path, StructRNA *type)
{
	PointerRNA *ptr;
	int a;

	for(a=0; a<path->len; a++) {
		ptr= &path->ptr[a];

		if(RNA_struct_is_a(ptr->type, type))
			return ptr;
	}

	return NULL;
}

/************************* Creating the Path ************************/

static int buttons_context_path_scene(ButsContextPath *path)
{
	PointerRNA *ptr= &path->ptr[path->len-1];

	/* this one just verifies */
	return RNA_struct_is_a(ptr->type, &RNA_Scene);
}

static int buttons_context_path_world(ButsContextPath *path)
{
	Scene *scene;
	PointerRNA *ptr= &path->ptr[path->len-1];

	/* if we already have a (pinned) world, we're done */
	if(RNA_struct_is_a(ptr->type, &RNA_World)) {
		return 1;
	}
	/* if we have a scene, use the scene's world */
	else if(buttons_context_path_scene(path)) {
		scene= path->ptr[path->len-1].data;

		RNA_id_pointer_create(&scene->world->id, &path->ptr[path->len]);
		path->len++;

		return 1;
	}

	/* no path to a world possible */
	return 0;
}


static int buttons_context_path_object(ButsContextPath *path)
{
	Scene *scene;
	Object *ob;
	PointerRNA *ptr= &path->ptr[path->len-1];

	/* if we already have a (pinned) object, we're done */
	if(RNA_struct_is_a(ptr->type, &RNA_Object)) {
		return 1;
	}
	/* if we have a scene, use the scene's active object */
	else if(buttons_context_path_scene(path)) {
		scene= path->ptr[path->len-1].data;
		ob= (scene->basact)? scene->basact->object: NULL;

		if(ob) {
			RNA_id_pointer_create(&ob->id, &path->ptr[path->len]);
			path->len++;

			return 1;
		}
	}

	/* no path to a object possible */
	return 0;
}

static int buttons_context_path_data(ButsContextPath *path, int type)
{
	Object *ob;
	PointerRNA *ptr= &path->ptr[path->len-1];

	/* if we already have a data, we're done */
	if(RNA_struct_is_a(ptr->type, &RNA_Mesh) && (type == -1 || type == OB_MESH)) return 1;
	else if(RNA_struct_is_a(ptr->type, &RNA_Curve) && (type == -1 || ELEM3(type, OB_CURVE, OB_SURF, OB_FONT))) return 1;
	else if(RNA_struct_is_a(ptr->type, &RNA_Armature) && (type == -1 || type == OB_ARMATURE)) return 1;
	else if(RNA_struct_is_a(ptr->type, &RNA_MetaBall) && (type == -1 || type == OB_MBALL)) return 1;
	else if(RNA_struct_is_a(ptr->type, &RNA_Lattice) && (type == -1 || type == OB_LATTICE)) return 1;
	else if(RNA_struct_is_a(ptr->type, &RNA_Camera) && (type == -1 || type == OB_CAMERA)) return 1;
	else if(RNA_struct_is_a(ptr->type, &RNA_Lamp) && (type == -1 || type == OB_LAMP)) return 1;
	/* try to get an object in the path, no pinning supported here */
	else if(buttons_context_path_object(path)) {
		ob= path->ptr[path->len-1].data;

		if(ob && (type == -1 || type == ob->type)) {
			RNA_id_pointer_create(ob->data, &path->ptr[path->len]);
			path->len++;

			return 1;
		}
	}

	/* no path to data possible */
	return 0;
}

static int buttons_context_path_modifier(ButsContextPath *path)
{
	Object *ob;

	if(buttons_context_path_object(path)) {
		ob= path->ptr[path->len-1].data;

		if(ob && ELEM4(ob->type, OB_MESH, OB_CURVE, OB_FONT, OB_SURF))
			return 1;
	}

	return 0;
}

static int buttons_context_path_material(ButsContextPath *path)
{
	Object *ob;
	PointerRNA *ptr= &path->ptr[path->len-1];
	Material *ma;

	/* if we already have a (pinned) material, we're done */
	if(RNA_struct_is_a(ptr->type, &RNA_Material)) {
		return 1;
	}
	/* if we have an object, use the object material slot */
	else if(buttons_context_path_object(path)) {
		ob= path->ptr[path->len-1].data;

		if(ob && ob->type && (ob->type<OB_LAMP)) {
			ma= give_current_material(ob, ob->actcol);
			RNA_id_pointer_create(&ma->id, &path->ptr[path->len]);
			path->len++;
			return 1;
		}
	}

	/* no path to a material possible */
	return 0;
}

static Bone *find_active_bone(Bone *bone)
{
	Bone *active;

	for(; bone; bone=bone->next) {
		if(bone->flag & BONE_ACTIVE)
			return bone;

		active= find_active_bone(bone->childbase.first);
		if(active)
			return active;
	}

	return NULL;
}

static int buttons_context_path_bone(ButsContextPath *path)
{
	bArmature *arm;
	Bone *bone;

	/* if we have an armature, get the active bone */
	if(buttons_context_path_data(path, OB_ARMATURE)) {
		arm= path->ptr[path->len-1].data;
		bone= find_active_bone(arm->bonebase.first);

		if(bone) {
			RNA_pointer_create(&arm->id, &RNA_Bone, bone, &path->ptr[path->len]);
			path->len++;
			return 1;
		}
	}

	/* no path to a bone possible */
	return 0;
}

static int buttons_context_path_particle(ButsContextPath *path)
{
	Object *ob;
	ParticleSystem *psys;

	/* if we have an object, get the active particle system */
	if(buttons_context_path_object(path)) {
		ob= path->ptr[path->len-1].data;

		if(ob && ob->type == OB_MESH) {
			psys= psys_get_current(ob);

			RNA_pointer_create(&ob->id, &RNA_ParticleSystem, psys, &path->ptr[path->len]);
			path->len++;
			return 1;
		}
	}

	/* no path to a particle system possible */
	return 0;
}

static int buttons_context_path_texture(ButsContextPath *path)
{
	Material *ma;
	Lamp *la;
	World *wo;
	MTex *mtex;
	Tex *tex;
	PointerRNA *ptr= &path->ptr[path->len-1];

	/* if we already have a (pinned) texture, we're done */
	if(RNA_struct_is_a(ptr->type, &RNA_Texture)) {
		return 1;
	}
	/* try world */
	else if(path->worldtex && buttons_context_path_world(path)) {
		wo= path->ptr[path->len-1].data;

		if(wo) {
			mtex= wo->mtex[(int)wo->texact];
			tex= (mtex)? mtex->tex: NULL;

			RNA_id_pointer_create(&tex->id, &path->ptr[path->len]);
			path->len++;
			return 1;
		}
	}
	/* try material */
	else if(buttons_context_path_material(path)) {
		ma= path->ptr[path->len-1].data;

		if(ma) {
			mtex= ma->mtex[(int)ma->texact];
			tex= (mtex)? mtex->tex: NULL;

			RNA_id_pointer_create(&tex->id, &path->ptr[path->len]);
			path->len++;
			return 1;
		}
	}
	/* try lamp */
	else if(buttons_context_path_data(path, OB_LAMP)) {
		la= path->ptr[path->len-1].data;

		if(la) {
			mtex= la->mtex[(int)la->texact];
			tex= (mtex)? mtex->tex: NULL;

			RNA_id_pointer_create(&tex->id, &path->ptr[path->len]);
			path->len++;
			return 1;
		}
	}
	/* TODO: material nodes, brush */

	/* no path to a particle system possible */
	return 0;
}

static int buttons_context_path(const bContext *C, ButsContextPath *path, int mainb, int worldtex)
{
	SpaceButs *sbuts= (SpaceButs*)CTX_wm_space_data(C);
	ID *id;
	int found;

	memset(path, 0, sizeof(*path));
	path->worldtex= worldtex;

	/* if some ID datablock is pinned, set the root pointer */
	if(sbuts->pinid) {
		id= sbuts->pinid;

		RNA_id_pointer_create(id, &path->ptr[0]);
		path->len++;
	}

	/* no pinned root, use scene as root */
	if(path->len == 0) {
		id= (ID*)CTX_data_scene(C);
		RNA_id_pointer_create(id, &path->ptr[0]);
		path->len++;
	}

	/* now for each buttons context type, we try to construct a path,
	 * tracing back recursively */
	switch(mainb) {
		case BCONTEXT_SCENE:
			found= buttons_context_path_scene(path);
			break;
		case BCONTEXT_WORLD:
			found= buttons_context_path_world(path);
			break;
		case BCONTEXT_OBJECT:
		case BCONTEXT_PHYSICS:
		case BCONTEXT_CONSTRAINT:
			found= buttons_context_path_object(path);
			break;
		case BCONTEXT_MODIFIER:
			found= buttons_context_path_modifier(path);
			break;
		case BCONTEXT_DATA:
			found= buttons_context_path_data(path, -1);
			break;
		case BCONTEXT_PARTICLE:
			found= buttons_context_path_particle(path);
			break;
		case BCONTEXT_MATERIAL:
			found= buttons_context_path_material(path);
			break;
		case BCONTEXT_TEXTURE:
			found= buttons_context_path_texture(path);
			break;
		case BCONTEXT_BONE:
			found= buttons_context_path_bone(path);
			break;
		default:
			found= 0;
			break;
	}

	return found;
}

void buttons_context_compute(const bContext *C, SpaceButs *sbuts)
{
	ButsContextPath *path;
	PointerRNA *ptr;
	int a, worldtex, flag= 0;

	if(!sbuts->path)
		sbuts->path= MEM_callocN(sizeof(ButsContextPath), "ButsContextPath");
	
	path= sbuts->path;
	worldtex= (sbuts->flag & SB_WORLD_TEX);
	
	/* for each context, see if we can compute a valid path to it, if
	 * this is the case, we know we have to display the button */
	for(a=0; a<BCONTEXT_TOT; a++) {
		if(buttons_context_path(C, path, a, worldtex)) {
			flag |= (1<<a);

			/* setting icon for data context */
			if(a == BCONTEXT_DATA) {
				ptr= &path->ptr[path->len-1];

				if(ptr->type)
					sbuts->dataicon= RNA_struct_ui_icon(ptr->type);
				else
					sbuts->dataicon= ICON_EMPTY_DATA;
			}
		}
	}

	/* in case something becomes invalid, change */
	if((flag & (1 << sbuts->mainb)) == 0) {
		if(flag & BCONTEXT_OBJECT) {
			sbuts->mainb= BCONTEXT_OBJECT;
		}
		else {
			for(a=0; a<BCONTEXT_TOT; a++) {
				if(flag & (1 << a)) {
					sbuts->mainb= a;
					break;
				}
			}
		}
	}

	buttons_context_path(C, path, sbuts->mainb, worldtex);

	if(!(flag & (1 << sbuts->mainb))) {
		if(flag & (1 << BCONTEXT_OBJECT))
			sbuts->mainb= BCONTEXT_OBJECT;
		else
			sbuts->mainb= BCONTEXT_SCENE;
	}

	sbuts->pathflag= flag;
}

/************************* Context Callback ************************/

int buttons_context(const bContext *C, const char *member, bContextDataResult *result)
{
	SpaceButs *sbuts= (SpaceButs*)CTX_wm_space_data(C);
	ButsContextPath *path= sbuts?sbuts->path:NULL;

	if(!path)
		return 0;

	/* here we handle context, getting data from precomputed path */

	if(CTX_data_equals(member, "world")) {
		set_pointer_type(path, result, &RNA_World);
		return 1;
	}
	else if(CTX_data_equals(member, "object")) {
		set_pointer_type(path, result, &RNA_Object);
		return 1;
	}
	else if(CTX_data_equals(member, "mesh")) {
		set_pointer_type(path, result, &RNA_Mesh);
		return 1;
	}
	else if(CTX_data_equals(member, "armature")) {
		set_pointer_type(path, result, &RNA_Armature);
		return 1;
	}
	else if(CTX_data_equals(member, "lattice")) {
		set_pointer_type(path, result, &RNA_Lattice);
		return 1;
	}
	else if(CTX_data_equals(member, "curve")) {
		set_pointer_type(path, result, &RNA_Curve);
		return 1;
	}
	else if(CTX_data_equals(member, "meta_ball")) {
		set_pointer_type(path, result, &RNA_MetaBall);
		return 1;
	}
	else if(CTX_data_equals(member, "lamp")) {
		set_pointer_type(path, result, &RNA_Lamp);
		return 1;
	}
	else if(CTX_data_equals(member, "camera")) {
		set_pointer_type(path, result, &RNA_Camera);
		return 1;
	}
	else if(CTX_data_equals(member, "material")) {
		set_pointer_type(path, result, &RNA_Material);
		return 1;
	}
	else if(CTX_data_equals(member, "texture")) {
		set_pointer_type(path, result, &RNA_Texture);
		return 1;
	}
	else if(CTX_data_equals(member, "material_slot")) {
		PointerRNA *ptr= get_pointer_type(path, &RNA_Object);

		if(ptr) {
			Object *ob= ptr->data;

			if(ob && ob->type && (ob->type<OB_LAMP))
				CTX_data_pointer_set(result, &ob->id, &RNA_MaterialSlot, ob->mat+ob->actcol-1);
		}

		return 1;
	}
	else if(CTX_data_equals(member, "texture_slot")) {
		PointerRNA *ptr;

		if((ptr=get_pointer_type(path, &RNA_Material))) {
			Material *ma= ptr->data;

			if(ma)
				CTX_data_pointer_set(result, &ma->id, &RNA_TextureSlot, ma->mtex[(int)ma->texact]);
		}
		else if((ptr=get_pointer_type(path, &RNA_Lamp))) {
			Lamp *la= ptr->data;

			if(la)
				CTX_data_pointer_set(result, &la->id, &RNA_TextureSlot, la->mtex[(int)la->texact]);
		}
		else if((ptr=get_pointer_type(path, &RNA_World))) {
			World *wo= ptr->data;

			if(wo)
				CTX_data_pointer_set(result, &wo->id, &RNA_TextureSlot, wo->mtex[(int)wo->texact]);
		}
		else if((ptr=get_pointer_type(path, &RNA_Brush))) { /* how to get this into context? */
			Brush *br= ptr->data;

			if(br)
				CTX_data_pointer_set(result, &br->id, &RNA_TextureSlot, br->mtex[(int)br->texact]);
		}

		return 1;
	}
	else if(CTX_data_equals(member, "bone")) {
		set_pointer_type(path, result, &RNA_Bone);
		return 1;
	}
	else if(CTX_data_equals(member, "particle_system")) {
		set_pointer_type(path, result, &RNA_ParticleSystem);
		return 1;
	}
	else if(CTX_data_equals(member, "cloth")) {
		set_pointer_type(path, result, &RNA_ClothModifier);
		return 1;
	}
	else if(CTX_data_equals(member, "soft_body")) {
		PointerRNA *ptr= get_pointer_type(path, &RNA_Object);

		if(ptr && ptr->data) {
			Object *ob= ptr->data;
			CTX_data_pointer_set(result, &ob->id, &RNA_SoftBodySettings, ob->soft);
			return 1;
		}
	}
	else if(CTX_data_equals(member, "fluid")) {
		PointerRNA *ptr= get_pointer_type(path, &RNA_Object);

		if(ptr && ptr->data) {
			Object *ob= ptr->data;
			ModifierData *md= modifiers_findByType(ob, eModifierType_Fluidsim);
			CTX_data_pointer_set(result, &ob->id, &RNA_FluidSimulationModifier, md);
			return 1;
		}
	}

	return 0;
}

/************************* Drawing the Path ************************/

static void pin_cb(bContext *C, void *arg1, void *arg2)
{
	SpaceButs *sbuts= (SpaceButs*)CTX_wm_space_data(C);
	ButsContextPath *path= sbuts->path;
	PointerRNA *ptr;
	int a;

	if(sbuts->flag & SB_PIN_CONTEXT) {
		if(path->len) {
			for(a=path->len-1; a>=0; a--) {
				ptr= &path->ptr[a];

				if(ptr->id.data) {
					sbuts->pinid= ptr->id.data;
					break;
				}
			}
		}
	}
	else
		sbuts->pinid= NULL;
	
	ED_area_tag_redraw(CTX_wm_area(C));
}

void buttons_context_draw(const bContext *C, uiLayout *layout)
{
	SpaceButs *sbuts= (SpaceButs*)CTX_wm_space_data(C);
	ButsContextPath *path= sbuts->path;
	uiLayout *row;
	uiBlock *block;
	uiBut *but;
	PointerRNA *ptr;
	PropertyRNA *nameprop;
	char namebuf[128], *name;
	int a, icon;

	if(!path)
		return;

	row= uiLayoutRow(layout, 0);
	uiLayoutSetAlignment(row, UI_LAYOUT_ALIGN_LEFT);

	block= uiLayoutGetBlock(row);
	uiBlockSetEmboss(block, UI_EMBOSSN);
	but= uiDefIconButBitC(block, ICONTOG, SB_PIN_CONTEXT, 0, (sbuts->flag & SB_PIN_CONTEXT)? ICON_PINNED: ICON_UNPINNED, 0, 0, UI_UNIT_X, UI_UNIT_Y, &sbuts->flag, 0, 0, 0, 0, "Follow context or keep fixed datablock displayed.");
	uiButSetFunc(but, pin_cb, NULL, NULL);

	for(a=0; a<path->len; a++) {
		ptr= &path->ptr[a];

		if(ptr->data) {
			icon= RNA_struct_ui_icon(ptr->type);
			nameprop= RNA_struct_name_property(ptr->type);

#if 0
			if(sbuts->mainb != BCONTEXT_SCENE && ptr->type == &RNA_Scene) {
				uiItemL(row, "", icon); /* save some space */
			}
			else
#endif
			if(nameprop) {
				name= RNA_property_string_get_alloc(ptr, nameprop, namebuf, sizeof(namebuf));

				uiItemL(row, name, icon);

				if(name != namebuf)
					MEM_freeN(name);
			}
			else
				uiItemL(row, "", icon);
		}
	}
}

static void buttons_panel_context(const bContext *C, Panel *pa)
{
	buttons_context_draw(C, pa->layout);
}

void buttons_context_register(ARegionType *art)
{
	PanelType *pt;

	pt= MEM_callocN(sizeof(PanelType), "spacetype buttons panel context");
	strcpy(pt->idname, "BUTTONS_PT_context");
	strcpy(pt->label, "Context");
	pt->draw= buttons_panel_context;
	BLI_addtail(&art->paneltypes, pt);
}

