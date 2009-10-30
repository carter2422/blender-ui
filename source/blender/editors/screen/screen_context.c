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
 * ***** END GPL LICENSE BLOCK *****
 */

#include <stdlib.h>
#include <string.h>

#include "DNA_object_types.h"
#include "DNA_armature_types.h"
#include "DNA_action_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"

#include "BKE_context.h"
#include "BKE_utildefines.h"
#include "BKE_global.h"
#include "BKE_action.h"

#include "RNA_access.h"

#include "ED_object.h"
#include "ED_armature.h"

int ed_screen_context(const bContext *C, const char *member, bContextDataResult *result)
{
	bScreen *sc= CTX_wm_screen(C);
	Scene *scene= sc->scene;
	Base *base;

	Object *obact= CTX_data_active_object(C);
	Object *obedit= CTX_data_edit_object(C);

	if(CTX_data_dir(member)) {
		static const char *dir[] = {
			"scene", "selected_objects", "selected_bases",
			"selected_editable_objects", "selected_editable_bases",
			"visible_bones", "editable_bones", "selected_bones", "selected_editable_bones",
			"visible_pchans", "selected_pchans", "active_bone", "active_pchan",
			"active_base", "active_object", "edit_object",
			"sculpt_object", "vertex_paint_object", "weight_paint_object",
			"texture_paint_object", "brush", "particle_edit_object", NULL};

		CTX_data_dir_set(result, dir);
		return 1;
	}
	else if(CTX_data_equals(member, "scene")) {
		CTX_data_id_pointer_set(result, &scene->id);
		return 1;
	}
	else if(CTX_data_equals(member, "selected_objects") || CTX_data_equals(member, "selected_bases")) {
		int selected_objects= CTX_data_equals(member, "selected_objects");

		for(base=scene->base.first; base; base=base->next) {
			if((base->flag & SELECT) && (base->lay & scene->lay)) {
				if(selected_objects)
					CTX_data_id_list_add(result, &base->object->id);
				else
					CTX_data_list_add(result, &scene->id, &RNA_UnknownType, base);
			}
		}

		return 1;
	}
	else if(CTX_data_equals(member, "selected_editable_objects") || CTX_data_equals(member, "selected_editable_bases")) {
		int selected_editable_objects= CTX_data_equals(member, "selected_editable_objects");

		for(base=scene->base.first; base; base=base->next) {
			if((base->flag & SELECT) && (base->lay & scene->lay)) {
				if((base->object->restrictflag & OB_RESTRICT_VIEW)==0) {
					if(0==object_is_libdata(base->object)) {
						if(selected_editable_objects)
							CTX_data_id_list_add(result, &base->object->id);
						else
							CTX_data_list_add(result, &scene->id, &RNA_UnknownType, base);
					}
				}
			}
		}

		return 1;
	}
	else if(CTX_data_equals(member, "visible_bones") || CTX_data_equals(member, "editable_bones")) {
		bArmature *arm= (obedit) ? obedit->data : NULL;
		EditBone *ebone, *flipbone=NULL;
		int editable_bones= CTX_data_equals(member, "editable_bones");
		
		if (arm && arm->edbo) {
			/* Attention: X-Axis Mirroring is also handled here... */
			for (ebone= arm->edbo->first; ebone; ebone= ebone->next) {
				/* first and foremost, bone must be visible and selected */
				if (EBONE_VISIBLE(arm, ebone)) {
					/* Get 'x-axis mirror equivalent' bone if the X-Axis Mirroring option is enabled
					 * so that most users of this data don't need to explicitly check for it themselves.
					 * 
					 * We need to make sure that these mirrored copies are not selected, otherwise some
					 * bones will be operated on twice.
					 */
					if (arm->flag & ARM_MIRROR_EDIT)
						flipbone = ED_armature_bone_get_mirrored(arm->edbo, ebone);
					
					/* if we're filtering for editable too, use the check for that instead, as it has selection check too */
					if (editable_bones) {
						/* only selected + editable */
						if (EBONE_EDITABLE(ebone)) {
							CTX_data_list_add(result, &arm->id, &RNA_UnknownType, ebone);
						
							if ((flipbone) && !(flipbone->flag & BONE_SELECTED))
								CTX_data_list_add(result, &arm->id, &RNA_UnknownType, flipbone);
						}
					}
					else {
						/* only include bones if visible */
						CTX_data_list_add(result, &arm->id, &RNA_UnknownType, ebone);
						
						if ((flipbone) && EBONE_VISIBLE(arm, flipbone)==0)
							CTX_data_list_add(result, &arm->id, &RNA_UnknownType, flipbone);
					}
				}
			}	
			
			return 1;
		}
	}
	else if(CTX_data_equals(member, "selected_bones") || CTX_data_equals(member, "selected_editable_bones")) {
		bArmature *arm= (obedit) ? obedit->data : NULL;
		EditBone *ebone, *flipbone=NULL;
		int selected_editable_bones= CTX_data_equals(member, "selected_editable_bones");
		
		if (arm && arm->edbo) {
			/* Attention: X-Axis Mirroring is also handled here... */
			for (ebone= arm->edbo->first; ebone; ebone= ebone->next) {
				/* first and foremost, bone must be visible and selected */
				if (EBONE_VISIBLE(arm, ebone) && (ebone->flag & BONE_SELECTED)) {
					/* Get 'x-axis mirror equivalent' bone if the X-Axis Mirroring option is enabled
					 * so that most users of this data don't need to explicitly check for it themselves.
					 * 
					 * We need to make sure that these mirrored copies are not selected, otherwise some
					 * bones will be operated on twice.
					 */
					if (arm->flag & ARM_MIRROR_EDIT)
						flipbone = ED_armature_bone_get_mirrored(arm->edbo, ebone);
					
					/* if we're filtering for editable too, use the check for that instead, as it has selection check too */
					if (selected_editable_bones) {
						/* only selected + editable */
						if (EBONE_EDITABLE(ebone)) {
							CTX_data_list_add(result, &arm->id, &RNA_UnknownType, ebone);
						
							if ((flipbone) && !(flipbone->flag & BONE_SELECTED))
								CTX_data_list_add(result, &arm->id, &RNA_UnknownType, flipbone);
						}
					}
					else {
						/* only include bones if selected */
						CTX_data_list_add(result, &arm->id, &RNA_UnknownType, ebone);
						
						if ((flipbone) && !(flipbone->flag & BONE_SELECTED))
							CTX_data_list_add(result, &arm->id, &RNA_UnknownType, flipbone);
					}
				}
			}	
			
			return 1;
		}
	}
	else if(CTX_data_equals(member, "visible_pchans")) {
		bArmature *arm= (obact) ? obact->data : NULL;
		bPoseChannel *pchan;
		
		if (obact && arm) {
			for (pchan= obact->pose->chanbase.first; pchan; pchan= pchan->next) {
				/* ensure that PoseChannel is on visible layer and is not hidden in PoseMode */
				if ((pchan->bone) && (arm->layer & pchan->bone->layer) && !(pchan->bone->flag & BONE_HIDDEN_P)) {
					CTX_data_list_add(result, &obact->id, &RNA_PoseChannel, pchan);
				}
			}
			
			return 1;
		}
	}
	else if(CTX_data_equals(member, "selected_pchans")) {
		bArmature *arm= (obact) ? obact->data : NULL;
		bPoseChannel *pchan;
		
		if (obact && arm) {
			for (pchan= obact->pose->chanbase.first; pchan; pchan= pchan->next) {
				/* ensure that PoseChannel is on visible layer and is not hidden in PoseMode */
				if ((pchan->bone) && (arm->layer & pchan->bone->layer) && !(pchan->bone->flag & BONE_HIDDEN_P)) {
					if (pchan->bone->flag & (BONE_SELECTED|BONE_ACTIVE)) 
						CTX_data_list_add(result, &obact->id, &RNA_PoseChannel, pchan);
				}
			}
			
			return 1;
		}
	}
	else if(CTX_data_equals(member, "active_bone")) {
		bArmature *arm= (obedit) ? obedit->data : NULL;
		EditBone *ebone;
		
		if (arm && arm->edbo) {
			for (ebone= arm->edbo->first; ebone; ebone= ebone->next) {
				if (EBONE_VISIBLE(arm, ebone)) {
					if (ebone->flag & BONE_ACTIVE) {
						CTX_data_pointer_set(result, &arm->id, &RNA_UnknownType, ebone);
						
						return 1;
					}
				}
			}
		}
		
	}
	else if(CTX_data_equals(member, "active_pchan")) {
		bPoseChannel *pchan;
		
		pchan= get_active_posechannel(obact);
		if (pchan) {
			CTX_data_pointer_set(result, &obact->id, &RNA_PoseChannel, pchan);
			return 1;
		}
	}
	else if(CTX_data_equals(member, "active_base")) {
		base= CTX_data_active_base(C); /* not used in many places so get here */
		if(base)
			CTX_data_pointer_set(result, &scene->id, &RNA_UnknownType, base);

		return 1;
	}
	else if(CTX_data_equals(member, "active_object")) {
		if(obact)
			CTX_data_id_pointer_set(result, &obact->id);

		return 1;
	}
	else if(CTX_data_equals(member, "object")) {
		if(obact)
			CTX_data_id_pointer_set(result, &obact->id);

		return 1;
	}
	else if(CTX_data_equals(member, "edit_object")) {
		/* convenience for now, 1 object per scene in editmode */
		if(obedit)
			CTX_data_id_pointer_set(result, &obedit->id);
		
		return 1;
	}
	else if(CTX_data_equals(member, "sculpt_object")) {
		if(obact && (obact->mode & OB_MODE_SCULPT))
			CTX_data_id_pointer_set(result, &obact->id);

		return 1;
	}
	else if(CTX_data_equals(member, "vertex_paint_object")) {
		if(obact && (obact->mode & OB_MODE_VERTEX_PAINT))
			CTX_data_id_pointer_set(result, &obact->id);

		return 1;
	}
	else if(CTX_data_equals(member, "weight_paint_object")) {
		if(obact && (obact->mode & OB_MODE_WEIGHT_PAINT))
			CTX_data_id_pointer_set(result, &obact->id);

		return 1;
	}
	else if(CTX_data_equals(member, "texture_paint_object")) {
		if(obact && (obact->mode & OB_MODE_TEXTURE_PAINT))
			CTX_data_id_pointer_set(result, &obact->id);

		return 1;
	}
	else if(CTX_data_equals(member, "particle_edit_object")) {
		if(obact && (obact->mode & OB_MODE_PARTICLE_EDIT))
			CTX_data_id_pointer_set(result, &obact->id);

		return 1;
	}

	return 0;
}

