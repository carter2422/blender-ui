/* writefile.c
 *
 * .blend file writing
 *
 * $Id$
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
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

/*
FILEFORMAT: IFF-style structure  (but not IFF compatible!)

start file:
	BLENDER_V100	12 bytes  (versie 1.00)
					V = big endian, v = little endian
					_ = 4 byte pointer, - = 8 byte pointer

datablocks:		also see struct BHead
	<bh.code>			4 chars
	<bh.len>			int,  len data after BHead
	<bh.old>			void,  old pointer
	<bh.SDNAnr>			int
	<bh.nr>				int, in case of array: amount of structs
	data
	...
	...

Almost all data in Blender are structures. Each struct saved
gets a BHead header.  With BHead the struct can be linked again
and compared with StructDNA .

WRITE

Preferred writing order: (not really a must, but why would you do it random?)
Any case: direct data is ALWAYS after the lib block

(Local file data)
- for each LibBlock
	- write LibBlock
	- write associated direct data
(External file data)
- per library
	- write library block
	- per LibBlock
		- write the ID of LibBlock
- write FileGlobal (some global vars)
- write SDNA
- write USER if filename is ~/.B.blend
*/

/* for version 2.2+
Important to know is that 'streaming' has been added to files, for Blender Publisher
*/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "zlib.h"

#ifndef WIN32
#include <unistd.h>
#else
#include "winsock2.h"
#include "BLI_winstuff.h"
#include <io.h>
#include <process.h> // for getpid
#endif

#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nla.h" //  __NLA is defined

#include "DNA_armature_types.h"
#include "DNA_action_types.h"
#include "DNA_actuator_types.h"
#include "DNA_brush_types.h"
#include "DNA_camera_types.h"
#include "DNA_color_types.h"
#include "DNA_constraint_types.h"
#include "DNA_controller_types.h"
#include "DNA_curve_types.h"
#include "DNA_customdata_types.h"
#include "DNA_effect_types.h"
#include "DNA_group_types.h"
#include "DNA_image_types.h"
#include "DNA_ipo_types.h"
#include "DNA_fileglobal_types.h"
#include "DNA_key_types.h"
#include "DNA_lattice_types.h"
#include "DNA_listBase.h" /* for Listbase, the type of samples, ...*/
#include "DNA_lamp_types.h"
#include "DNA_meta_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_material_types.h"
#include "DNA_modifier_types.h"
#include "DNA_nla_types.h"
#include "DNA_node_types.h"
#include "DNA_object_types.h"
#include "DNA_object_force.h"
#include "DNA_oops_types.h"
#include "DNA_packedFile_types.h"
#include "DNA_property_types.h"
#include "DNA_scene_types.h"
#include "DNA_sdna_types.h"
#include "DNA_sequence_types.h"
#include "DNA_sensor_types.h"
#include "DNA_space_types.h"
#include "DNA_screen_types.h"
#include "DNA_sound_types.h"
#include "DNA_texture_types.h"
#include "DNA_text_types.h"
#include "DNA_view3d_types.h"
#include "DNA_vfont_types.h"
#include "DNA_userdef_types.h"
#include "DNA_world_types.h"

#include "MEM_guardedalloc.h" // MEM_freeN
#include "BLI_blenlib.h"
#include "BLI_linklist.h"

#include "BKE_action.h"
#include "BKE_bad_level_calls.h" // build_seqar (from WHILE_SEQ) free_oops error
#include "BKE_curve.h"
#include "BKE_constraint.h"
#include "BKE_global.h" // for G
#include "BKE_library.h" // for  set_listbasepointers
#include "BKE_main.h" // G.main
#include "BKE_node.h"
#include "BKE_packedFile.h" // for packAll
#include "BKE_screen.h" // for waitcursor
#include "BKE_scene.h" // for do_seq
#include "BKE_sound.h" /* ... and for samples */
#include "BKE_utildefines.h" // for defines
#include "BKE_modifier.h"
#include "BKE_idprop.h"
#ifdef WITH_VERSE
#include "BKE_verse.h"
#include "BIF_verse.h"
#endif

#include "GEN_messaging.h"

#include "BLO_writefile.h"
#include "BLO_readfile.h"
#include "BLO_undofile.h"

#include "readfile.h"
#include "genfile.h"

#include <errno.h>

/* ********* my write, buffered writing with minimum 50k chunks ************ */

typedef struct {
	struct SDNA *sdna;

	int file;
	unsigned char *buf;
	MemFile *compare, *current;
	
	int tot, count, error, memsize;
} WriteData;

static WriteData *writedata_new(int file)
{
	extern unsigned char DNAstr[];	/* DNA.c */
	extern int DNAlen;

	WriteData *wd= MEM_callocN(sizeof(*wd), "writedata");

		/* XXX, see note about this in readfile.c, remove
		 * once we have an xp lock - zr
		 */

	if (wd == NULL) return NULL;

	wd->sdna= dna_sdna_from_data(DNAstr, DNAlen, 0);

	wd->file= file;

	wd->buf= MEM_mallocN(100000, "wd->buf");

	return wd;
}

static void writedata_do_write(WriteData *wd, void *mem, int memlen)
{
	if ((wd == NULL) || wd->error || (mem == NULL) || memlen < 1) return;
	if (wd->error) return;

	/* memory based save */
	if(wd->current) {
		add_memfilechunk(NULL, wd->current, mem, memlen);
	}
	else {
		if (write(wd->file, mem, memlen) != memlen)
			wd->error= 1;
		
	}
}

static void writedata_free(WriteData *wd)
{
	dna_freestructDNA(wd->sdna);

	MEM_freeN(wd->buf);
	MEM_freeN(wd);
}

/***/

int mywfile;

/**
 * Low level WRITE(2) wrapper that buffers data
 * @param adr Pointer to new chunk of data
 * @param len Length of new chunk of data
 * @warning Talks to other functions with global parameters
 */
 
#define MYWRITE_FLUSH	NULL

static void mywrite( WriteData *wd, void *adr, int len)
{
	if (wd->error) return;

	if(adr==MYWRITE_FLUSH) {
		if(wd->count) {
			writedata_do_write(wd, wd->buf, wd->count);
			wd->count= 0;
		}
		return;
	}

	wd->tot+= len;
	
	if(len>50000) {
		if(wd->count) {
			writedata_do_write(wd, wd->buf, wd->count);
			wd->count= 0;
		}
		writedata_do_write(wd, adr, len);
		return;
	}
	if(len+wd->count>99999) {
		writedata_do_write(wd, wd->buf, wd->count);
		wd->count= 0;
	}
	memcpy(&wd->buf[wd->count], adr, len);
	wd->count+= len;

}

/**
 * BeGiN initializer for mywrite
 * @param file File descriptor
 * @param write_flags Write parameters
 * @warning Talks to other functions with global parameters
 */
static WriteData *bgnwrite(int file, MemFile *compare, MemFile *current, int write_flags)
{
	WriteData *wd= writedata_new(file);

	if (wd == NULL) return NULL;

	wd->compare= compare;
	wd->current= current;
	/* this inits comparing */
	add_memfilechunk(compare, NULL, NULL, 0);
	
	return wd;
}

/**
 * END the mywrite wrapper
 * @return 1 if write failed
 * @return unknown global variable otherwise
 * @warning Talks to other functions with global parameters
 */
static int endwrite(WriteData *wd)
{
	int err;

	if (wd->count) {
		writedata_do_write(wd, wd->buf, wd->count);
		wd->count= 0;
	}
	
	err= wd->error;
	writedata_free(wd);
/* blender gods may live forever but this parent pointer died in the statement above
if(wd->current) printf("undo size %d\n", wd->current->size);
*/
	return err;
}

/* ********** WRITE FILE ****************** */

static void writestruct(WriteData *wd, int filecode, char *structname, int nr, void *adr)
{
	BHead bh;
	short *sp;

	if(adr==NULL || nr==0) return;

	/* init BHead */
	bh.code= filecode;
	bh.old= adr;
	bh.nr= nr;

	bh.SDNAnr= dna_findstruct_nr(wd->sdna, structname);
	if(bh.SDNAnr== -1) {
		printf("error: can't find SDNA code <%s>\n", structname);
		return;
	}
	sp= wd->sdna->structs[bh.SDNAnr];

	bh.len= nr*wd->sdna->typelens[sp[0]];

	if(bh.len==0) return;

	mywrite(wd, &bh, sizeof(BHead));
	mywrite(wd, adr, bh.len);
}

static void writedata(WriteData *wd, int filecode, int len, void *adr)	/* do not use for structs */
{
	BHead bh;

	if(adr==0) return;
	if(len==0) return;

	len+= 3;
	len-= ( len % 4);

	/* init BHead */
	bh.code= filecode;
	bh.old= adr;
	bh.nr= 1;
	bh.SDNAnr= 0;
	bh.len= len;

	mywrite(wd, &bh, sizeof(BHead));
	if(len) mywrite(wd, adr, len);
}

/* *************** writing some direct data structs used in more code parts **************** */
/*These functions are used by blender's .blend system for file saving/loading.*/
void IDP_WriteProperty_OnlyData(IDProperty *prop, void *wd);
void IDP_WriteProperty(IDProperty *prop, void *wd);

void IDP_WriteArray(IDProperty *prop, void *wd)
{
	/*REMEMBER to set totalen to len in the linking code!!*/
	if (prop->data.pointer) {
		writedata(wd, DATA, MEM_allocN_len(prop->data.pointer), prop->data.pointer);
	}
}

void IDP_WriteString(IDProperty *prop, void *wd)
{
	/*REMEMBER to set totalen to len in the linking code!!*/
	writedata(wd, DATA, prop->len+1, prop->data.pointer);
}

void IDP_WriteGroup(IDProperty *prop, void *wd)
{
	IDProperty *loop;

	for (loop=prop->data.group.first; loop; loop=loop->next) {
		IDP_WriteProperty(loop, wd);
	}
}

/* Functions to read/write ID Properties */
void IDP_WriteProperty_OnlyData(IDProperty *prop, void *wd)
{
	switch (prop->type) {
		case IDP_GROUP:
			IDP_WriteGroup(prop, wd);
			break;
		case IDP_STRING:
			IDP_WriteString(prop, wd);
			break;
		case IDP_ARRAY:
			IDP_WriteArray(prop, wd);
			break;
	}
}

void IDP_WriteProperty(IDProperty *prop, void *wd)
{
	writestruct(wd, DATA, "IDProperty", 1, prop);
	IDP_WriteProperty_OnlyData(prop, wd);
}

static void write_curvemapping(WriteData *wd, CurveMapping *cumap)
{
	int a;
	
	writestruct(wd, DATA, "CurveMapping", 1, cumap);
	for(a=0; a<CM_TOT; a++)
		writestruct(wd, DATA, "CurveMapPoint", cumap->cm[a].totpoint, cumap->cm[a].curve);
}

/* this is only direct data, tree itself should have been written */
static void write_nodetree(WriteData *wd, bNodeTree *ntree)
{
	bNode *node;
	bNodeSocket *sock;
	bNodeLink *link;
	
	/* for link_list() speed, we write per list */
	
	for(node= ntree->nodes.first; node; node= node->next)
		writestruct(wd, DATA, "bNode", 1, node);

	for(node= ntree->nodes.first; node; node= node->next) {
		if(node->storage) {
			/* could be handlerized at some point, now only 1 exception still */
			if(ntree->type==NTREE_SHADER && (node->type==SH_NODE_CURVE_VEC || node->type==SH_NODE_CURVE_RGB))
				write_curvemapping(wd, node->storage);
			else if(ntree->type==NTREE_COMPOSIT && (node->type==CMP_NODE_TIME || node->type==CMP_NODE_CURVE_VEC || node->type==CMP_NODE_CURVE_RGB))
				write_curvemapping(wd, node->storage);
			else 
				writestruct(wd, DATA, node->typeinfo->storagename, 1, node->storage);
		}
		for(sock= node->inputs.first; sock; sock= sock->next)
			writestruct(wd, DATA, "bNodeSocket", 1, sock);
		for(sock= node->outputs.first; sock; sock= sock->next)
			writestruct(wd, DATA, "bNodeSocket", 1, sock);
	}
	
	for(link= ntree->links.first; link; link= link->next)
		writestruct(wd, DATA, "bNodeLink", 1, link);
}

static void write_scriptlink(WriteData *wd, ScriptLink *slink)
{
	writedata(wd, DATA, sizeof(void *)*slink->totscript, slink->scripts);
	writedata(wd, DATA, sizeof(short)*slink->totscript, slink->flag);
}

static void write_renderinfo(WriteData *wd)		/* for renderdaemon */
{
	Scene *sce;
	int data[8];

	sce= G.main->scene.first;
	while(sce) {
		if(sce->id.lib==0  && ( sce==G.scene || (sce->r.scemode & R_BG_RENDER)) ) {
			data[0]= sce->r.sfra;
			data[1]= sce->r.efra;

			strncpy((char *)(data+2), sce->id.name+2, 23);

			writedata(wd, REND, 32, data);
		}
		sce= sce->id.next;
	}
}

static void write_userdef(WriteData *wd)
{
	bTheme *btheme;

	writestruct(wd, USER, "UserDef", 1, &U);

	btheme= U.themes.first;
	while(btheme) {
		writestruct(wd, DATA, "bTheme", 1, btheme);
		btheme= btheme->next;
	}
}

static void write_effects(WriteData *wd, ListBase *lb)
{
	Effect *eff;

	eff= lb->first;
	while(eff) {

		switch(eff->type) {
		case EFF_PARTICLE:
			writestruct(wd, DATA, "PartEff", 1, eff);
			break;
		default:
			writedata(wd, DATA, MEM_allocN_len(eff), eff);
		}

		eff= eff->next;
	}
}

static void write_properties(WriteData *wd, ListBase *lb)
{
	bProperty *prop;

	prop= lb->first;
	while(prop) {
		writestruct(wd, DATA, "bProperty", 1, prop);

		if(prop->poin && prop->poin != &prop->data)
			writedata(wd, DATA, MEM_allocN_len(prop->poin), prop->poin);

		prop= prop->next;
	}
}

static void write_sensors(WriteData *wd, ListBase *lb)
{
	bSensor *sens;

	sens= lb->first;
	while(sens) {
		writestruct(wd, DATA, "bSensor", 1, sens);

		writedata(wd, DATA, sizeof(void *)*sens->totlinks, sens->links);

		switch(sens->type) {
		case SENS_NEAR:
			writestruct(wd, DATA, "bNearSensor", 1, sens->data);
			break;
		case SENS_MOUSE:
			writestruct(wd, DATA, "bMouseSensor", 1, sens->data);
			break;
		case SENS_TOUCH:
			writestruct(wd, DATA, "bTouchSensor", 1, sens->data);
			break;
		case SENS_KEYBOARD:
			writestruct(wd, DATA, "bKeyboardSensor", 1, sens->data);
			break;
		case SENS_PROPERTY:
			writestruct(wd, DATA, "bPropertySensor", 1, sens->data);
			break;
		case SENS_COLLISION:
			writestruct(wd, DATA, "bCollisionSensor", 1, sens->data);
			break;
		case SENS_RADAR:
			writestruct(wd, DATA, "bRadarSensor", 1, sens->data);
			break;
		case SENS_RANDOM:
			writestruct(wd, DATA, "bRandomSensor", 1, sens->data);
			break;
		case SENS_RAY:
			writestruct(wd, DATA, "bRaySensor", 1, sens->data);
			break;
		case SENS_MESSAGE:
			writestruct(wd, DATA, "bMessageSensor", 1, sens->data);
			break;
		case SENS_JOYSTICK:
			writestruct(wd, DATA, "bJoystickSensor", 1, sens->data);
			break;
		default:
			; /* error: don't know how to write this file */
		}

		sens= sens->next;
	}
}

static void write_controllers(WriteData *wd, ListBase *lb)
{
	bController *cont;

	cont= lb->first;
	while(cont) {
		writestruct(wd, DATA, "bController", 1, cont);

		writedata(wd, DATA, sizeof(void *)*cont->totlinks, cont->links);

		switch(cont->type) {
		case CONT_EXPRESSION:
			writestruct(wd, DATA, "bExpressionCont", 1, cont->data);
			break;
		case CONT_PYTHON:
			writestruct(wd, DATA, "bPythonCont", 1, cont->data);
			break;
		default:
			; /* error: don't know how to write this file */
		}

		cont= cont->next;
	}
}

static void write_actuators(WriteData *wd, ListBase *lb)
{
	bActuator *act;

	act= lb->first;
	while(act) {
		writestruct(wd, DATA, "bActuator", 1, act);

		switch(act->type) {
		case ACT_ACTION:
			writestruct(wd, DATA, "bActionActuator", 1, act->data);
			break;
		case ACT_SOUND:
			writestruct(wd, DATA, "bSoundActuator", 1, act->data);
			break;
		case ACT_CD:
			writestruct(wd, DATA, "bCDActuator", 1, act->data);
			break;
		case ACT_OBJECT:
			writestruct(wd, DATA, "bObjectActuator", 1, act->data);
			break;
		case ACT_IPO:
			writestruct(wd, DATA, "bIpoActuator", 1, act->data);
			break;
		case ACT_PROPERTY:
			writestruct(wd, DATA, "bPropertyActuator", 1, act->data);
			break;
		case ACT_CAMERA:
			writestruct(wd, DATA, "bCameraActuator", 1, act->data);
			break;
		case ACT_CONSTRAINT:
			writestruct(wd, DATA, "bConstraintActuator", 1, act->data);
			break;
		case ACT_EDIT_OBJECT:
			writestruct(wd, DATA, "bEditObjectActuator", 1, act->data);
			break;
		case ACT_SCENE:
			writestruct(wd, DATA, "bSceneActuator", 1, act->data);
			break;
		case ACT_GROUP:
			writestruct(wd, DATA, "bGroupActuator", 1, act->data);
			break;
		case ACT_RANDOM:
			writestruct(wd, DATA, "bRandomActuator", 1, act->data);
			break;
		case ACT_MESSAGE:
			writestruct(wd, DATA, "bMessageActuator", 1, act->data);
			break;
		case ACT_GAME:
			writestruct(wd, DATA, "bGameActuator", 1, act->data);
			break;
		case ACT_VISIBILITY:
			writestruct(wd, DATA, "bVisibilityActuator", 1, act->data);
			break;
		default:
			; /* error: don't know how to write this file */
		}

		act= act->next;
	}
}

static void write_nlastrips(WriteData *wd, ListBase *nlabase)
{
	bActionStrip *strip;
	bActionModifier *amod;
	
	for (strip=nlabase->first; strip; strip=strip->next)
		writestruct(wd, DATA, "bActionStrip", 1, strip);
	for (strip=nlabase->first; strip; strip=strip->next) {
		for(amod= strip->modifiers.first; amod; amod= amod->next)
			writestruct(wd, DATA, "bActionModifier", 1, amod);
	}
}

static void write_constraints(WriteData *wd, ListBase *conlist)
{
	bConstraint *con;

	for (con=conlist->first; con; con=con->next) {
		/* Write the specific data */
		switch (con->type) {
		case CONSTRAINT_TYPE_NULL:
			break;
		case CONSTRAINT_TYPE_TRACKTO:
			writestruct(wd, DATA, "bTrackToConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_KINEMATIC:
			writestruct(wd, DATA, "bKinematicConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_ROTLIKE:
			writestruct(wd, DATA, "bRotateLikeConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_LOCLIKE:
			writestruct(wd, DATA, "bLocateLikeConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_SIZELIKE:
			writestruct(wd, DATA, "bSizeLikeConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_ACTION:
			writestruct(wd, DATA, "bActionConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_LOCKTRACK:
			writestruct(wd, DATA, "bLockTrackConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_FOLLOWPATH:
			writestruct(wd, DATA, "bFollowPathConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_STRETCHTO:
			writestruct(wd, DATA, "bStretchToConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_MINMAX:
			writestruct(wd, DATA, "bMinMaxConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_LOCLIMIT:
			writestruct(wd, DATA, "bLocLimitConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_ROTLIMIT:
			writestruct(wd, DATA, "bRotLimitConstraint", 1, con->data);
			break;
		case CONSTRAINT_TYPE_SIZELIMIT:
			writestruct(wd, DATA, "bSizeLimitConstraint", 1, con->data);
			break;
		default:
			break;
		}
		/* Write the constraint */
		writestruct(wd, DATA, "bConstraint", 1, con);
	}
}

static void write_pose(WriteData *wd, bPose *pose)
{
	bPoseChannel	*chan;

	/* Write each channel */

	if (!pose)
		return;

	// Write channels
	for (chan=pose->chanbase.first; chan; chan=chan->next) {
		write_constraints(wd, &chan->constraints);
		chan->selectflag= chan->bone->flag & (BONE_SELECTED|BONE_ACTIVE); // gets restored on read, for library armatures
		writestruct(wd, DATA, "bPoseChannel", 1, chan);
	}

	// Write this pose
	writestruct(wd, DATA, "bPose", 1, pose);
}

static void write_defgroups(WriteData *wd, ListBase *defbase)
{
	bDeformGroup	*defgroup;

	for (defgroup=defbase->first; defgroup; defgroup=defgroup->next)
		writestruct(wd, DATA, "bDeformGroup", 1, defgroup);
}

static void write_constraint_channels(WriteData *wd, ListBase *chanbase)
{
	bConstraintChannel *chan;

	for (chan = chanbase->first; chan; chan=chan->next)
		writestruct(wd, DATA, "bConstraintChannel", 1, chan);

}

static void write_modifiers(WriteData *wd, ListBase *modbase)
{
	ModifierData *md;

	if (modbase == NULL) return;
	for (md=modbase->first; md; md= md->next) {
		ModifierTypeInfo *mti = modifierType_getInfo(md->type);
		if (mti == NULL) return;

		writestruct(wd, DATA, mti->structName, 1, md);

		if (md->type==eModifierType_Hook) {
			HookModifierData *hmd = (HookModifierData*) md;

			writedata(wd, DATA, sizeof(int)*hmd->totindex, hmd->indexar);
		}
	}
}

static void write_objects(WriteData *wd, ListBase *idbase)
{
	Object *ob;
	int a;
	
	ob= idbase->first;
	while(ob) {
		if(ob->id.us>0 || wd->current) {
			/* write LibData */
#ifdef WITH_VERSE
			/* pointer at vnode stored in file have to be NULL */
			struct VNode *vnode = (VNode*)ob->vnode;
			if(vnode) ob->vnode = NULL;
#endif
			writestruct(wd, ID_OB, "Object", 1, ob);
#ifdef WITH_VERSE
			if(vnode) ob->vnode = (void*)vnode;
#endif

			/*Write ID Properties -- and copy this comment EXACTLY for easy finding
			  of library blocks that implement this.*/
			if (ob->id.properties) IDP_WriteProperty(ob->id.properties, wd);

			/* direct data */
			writedata(wd, DATA, sizeof(void *)*ob->totcol, ob->mat);
			write_effects(wd, &ob->effect);
			write_properties(wd, &ob->prop);
			write_sensors(wd, &ob->sensors);
			write_controllers(wd, &ob->controllers);
			write_actuators(wd, &ob->actuators);
			write_scriptlink(wd, &ob->scriptlink);
			write_pose(wd, ob->pose);
			write_defgroups(wd, &ob->defbase);
			write_constraints(wd, &ob->constraints);
			write_constraint_channels(wd, &ob->constraintChannels);
			write_nlastrips(wd, &ob->nlastrips);
			
			writestruct(wd, DATA, "PartDeflect", 1, ob->pd);
			writestruct(wd, DATA, "SoftBody", 1, ob->soft);
			if(ob->soft) {
				SoftBody *sb= ob->soft;
				if(sb->keys) {
					writedata(wd, DATA, sizeof(void *)*sb->totkey, sb->keys);
					for(a=0; a<sb->totkey; a++) {
						writestruct(wd, DATA, "SBVertex", sb->totpoint, sb->keys[a]);
					}
				}
			}
			writestruct(wd, DATA, "FluidsimSettings", 1, ob->fluidsimSettings); // NT
			
			write_modifiers(wd, &ob->modifiers);
		}
		ob= ob->id.next;
	}

	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}


static void write_vfonts(WriteData *wd, ListBase *idbase)
{
	VFont *vf;
	PackedFile * pf;

	vf= idbase->first;
	while(vf) {
		if(vf->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_VF, "VFont", 1, vf);
			if (vf->id.properties) IDP_WriteProperty(vf->id.properties, wd);

			/* direct data */

			if (vf->packedfile) {
				pf = vf->packedfile;
				writestruct(wd, DATA, "PackedFile", 1, pf);
				writedata(wd, DATA, pf->size, pf->data);
			}
		}

		vf= vf->id.next;
	}
}

static void write_ipos(WriteData *wd, ListBase *idbase)
{
	Ipo *ipo;
	IpoCurve *icu;

	ipo= idbase->first;
	while(ipo) {
		if(ipo->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_IP, "Ipo", 1, ipo);
			if (ipo->id.properties) IDP_WriteProperty(ipo->id.properties, wd);

			/* direct data */
			icu= ipo->curve.first;
			while(icu) {
				writestruct(wd, DATA, "IpoCurve", 1, icu);
				icu= icu->next;
			}

			icu= ipo->curve.first;
			while(icu) {
				if(icu->bezt)  writestruct(wd, DATA, "BezTriple", icu->totvert, icu->bezt);
				if(icu->bp)  writestruct(wd, DATA, "BPoint", icu->totvert, icu->bp);
				if(icu->driver)  writestruct(wd, DATA, "IpoDriver", 1, icu->driver);
				icu= icu->next;
			}
		}

		ipo= ipo->id.next;
	}

	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_keys(WriteData *wd, ListBase *idbase)
{
	Key *key;
	KeyBlock *kb;

	key= idbase->first;
	while(key) {
		if(key->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_KE, "Key", 1, key);
			if (key->id.properties) IDP_WriteProperty(key->id.properties, wd);

			/* direct data */
			kb= key->block.first;
			while(kb) {
				writestruct(wd, DATA, "KeyBlock", 1, kb);
				if(kb->data) writedata(wd, DATA, kb->totelem*key->elemsize, kb->data);
				kb= kb->next;
			}
		}

		key= key->id.next;
	}
	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_cameras(WriteData *wd, ListBase *idbase)
{
	Camera *cam;

	cam= idbase->first;
	while(cam) {
		if(cam->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_CA, "Camera", 1, cam);
			if (cam->id.properties) IDP_WriteProperty(cam->id.properties, wd);

			/* direct data */
			write_scriptlink(wd, &cam->scriptlink);
		}

		cam= cam->id.next;
	}
}

static void write_mballs(WriteData *wd, ListBase *idbase)
{
	MetaBall *mb;
	MetaElem *ml;

	mb= idbase->first;
	while(mb) {
		if(mb->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_MB, "MetaBall", 1, mb);
			if (mb->id.properties) IDP_WriteProperty(mb->id.properties, wd);

			/* direct data */
			writedata(wd, DATA, sizeof(void *)*mb->totcol, mb->mat);

			ml= mb->elems.first;
			while(ml) {
				writestruct(wd, DATA, "MetaElem", 1, ml);
				ml= ml->next;
			}
		}
		mb= mb->id.next;
	}
}

int amount_of_chars(char *str)
{
	// Since the data is saved as UTF-8 to the cu->str
	// The cu->len is not same as the strlen(cu->str)
	return strlen(str);
}

static void write_curves(WriteData *wd, ListBase *idbase)
{
	Curve *cu;
	Nurb *nu;

	cu= idbase->first;
	while(cu) {
		if(cu->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_CU, "Curve", 1, cu);

			/* direct data */
			writedata(wd, DATA, sizeof(void *)*cu->totcol, cu->mat);
			if (cu->id.properties) IDP_WriteProperty(cu->id.properties, wd);

			if(cu->vfont) {
				writedata(wd, DATA, amount_of_chars(cu->str)+1, cu->str);
				writestruct(wd, DATA, "CharInfo", cu->len, cu->strinfo);
				writestruct(wd, DATA, "TextBox", cu->totbox, cu->tb);				
			}
			else {
				/* is also the order of reading */
				nu= cu->nurb.first;
				while(nu) {
					writestruct(wd, DATA, "Nurb", 1, nu);
					nu= nu->next;
				}
				nu= cu->nurb.first;
				while(nu) {
					if( (nu->type & 7)==CU_BEZIER)
						writestruct(wd, DATA, "BezTriple", nu->pntsu, nu->bezt);
					else {
						writestruct(wd, DATA, "BPoint", nu->pntsu*nu->pntsv, nu->bp);
						if(nu->knotsu) writedata(wd, DATA, KNOTSU(nu)*sizeof(float), nu->knotsu);
						if(nu->knotsv) writedata(wd, DATA, KNOTSV(nu)*sizeof(float), nu->knotsv);
					}
					nu= nu->next;
				}
			}
		}
		cu= cu->id.next;
	}

	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_dverts(WriteData *wd, int count, MDeformVert *dvlist)
{
	if (dvlist) {
		int	i;
		
		/* Write the dvert list */
		writestruct(wd, DATA, "MDeformVert", count, dvlist);
		
		/* Write deformation data for each dvert */
		for (i=0; i<count; i++) {
			if (dvlist[i].dw)
				writestruct(wd, DATA, "MDeformWeight", dvlist[i].totweight, dvlist[i].dw);
		}
	}
}

static void write_customdata(WriteData *wd, int count, CustomData *data)
{
	int i;

	writestruct(wd, DATA, "CustomDataLayer", data->maxlayer, data->layers);

	for (i=0; i<data->totlayer; i++) {
		CustomDataLayer *layer= &data->layers[i];
		char *structname;
		int structnum;

		if (layer->type == CD_MDEFORMVERT) {
			/* layer types that allocate own memory need special handling */
			write_dverts(wd, count, layer->data);
		}
		else {
			CustomData_file_write_info(layer->type, &structname, &structnum);
			if (structnum)
				writestruct(wd, DATA, structname, structnum*count, layer->data);
			else
				printf("error: this CustomDataLayer must not be written to file\n");
		}
	}
}

static void write_oldstyle_tface_242(WriteData *wd, Mesh *me)
{
	MTFace *mtf;
	MCol *mcol;
	TFace *tf;
	int a;

	mtf= me->mtface;
	mcol= me->mcol;
	tf= me->tface;

	for (a=0; a < me->totface; a++, mtf++, tf++) {
		if (me->mcol) {
			memcpy(tf->col, mcol, sizeof(tf->col));
			mcol+=4;
		}
		else
			memset(tf->col, 255, sizeof(tf->col));
		memcpy(tf->uv, mtf->uv, sizeof(tf->uv));

		tf->flag= mtf->flag;
		tf->unwrap= mtf->unwrap;
		tf->mode= mtf->mode;
		tf->tile= mtf->tile;
		tf->tpage= mtf->tpage;
		tf->transp= mtf->transp;
	}

	writestruct(wd, DATA, "TFace", me->totface, me->tface);
}

static void write_meshs(WriteData *wd, ListBase *idbase)
{
	Mesh *mesh;
	MultiresLevel *lvl;

	mesh= idbase->first;
	while(mesh) {
		if(mesh->id.us>0 || wd->current) {
			/* write LibData */
#ifdef WITH_VERSE
			struct VNode *vnode = (VNode*)mesh->vnode;
			if(vnode) {
				/* mesh has to be created from verse geometry node*/
				create_meshdata_from_geom_node(mesh, vnode);
				/* pointer at verse node can't be stored in file */
				mesh->vnode = NULL;
			}
#endif
			/* temporary upward compatibility until 2.43 release */
			if(mesh->mtface)
				mesh->tface= MEM_callocN(sizeof(TFace)*mesh->totface, "Oldstyle TFace");

			writestruct(wd, ID_ME, "Mesh", 1, mesh);
#ifdef WITH_VERSE
			if(vnode) mesh->vnode = (void*)vnode;
#endif

			/* direct data */
			if (mesh->id.properties) IDP_WriteProperty(mesh->id.properties, wd);

			writedata(wd, DATA, sizeof(void *)*mesh->totcol, mesh->mat);

			write_customdata(wd, mesh->pv?mesh->pv->totvert:mesh->totvert, &mesh->vdata);
			write_customdata(wd, mesh->totedge, &mesh->edata);
			write_customdata(wd, mesh->totface, &mesh->fdata);

			/* temporary upward compatibility until 2.43 release */
			if(mesh->mtface) {
				write_oldstyle_tface_242(wd, mesh);
				MEM_freeN(mesh->tface);
				mesh->tface = NULL;
			}

			/* Multires data */
			writestruct(wd, DATA, "Multires", 1, mesh->mr);
			if(mesh->mr) {
				for(lvl= mesh->mr->levels.first; lvl; lvl= lvl->next) {
					writestruct(wd, DATA, "MultiresLevel", 1, lvl);
					writestruct(wd, DATA, "MVert", lvl->totvert, lvl->verts);
					writestruct(wd, DATA, "MultiresFace", lvl->totface, lvl->faces);
					writestruct(wd, DATA, "MultiresEdge", lvl->totedge, lvl->edges);
					writestruct(wd, DATA, "MultiresTexColFace", lvl->totface, lvl->texcolfaces);
				}
			}

			/* PMV data */
			if(mesh->pv) {
				writestruct(wd, DATA, "PartialVisibility", 1, mesh->pv);
				writedata(wd, DATA, sizeof(unsigned int)*mesh->pv->totvert, mesh->pv->vert_map);
				writedata(wd, DATA, sizeof(int)*mesh->pv->totedge, mesh->pv->edge_map);
				writestruct(wd, DATA, "MFace", mesh->pv->totface, mesh->pv->old_faces);
				writestruct(wd, DATA, "MEdge", mesh->pv->totedge, mesh->pv->old_edges);
			}
		}
		mesh= mesh->id.next;
	}
}

static void write_lattices(WriteData *wd, ListBase *idbase)
{
	Lattice *lt;
	
	lt= idbase->first;
	while(lt) {
		if(lt->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_LT, "Lattice", 1, lt);
			if (lt->id.properties) IDP_WriteProperty(lt->id.properties, wd);

			/* direct data */
			writestruct(wd, DATA, "BPoint", lt->pntsu*lt->pntsv*lt->pntsw, lt->def);
			
			write_dverts(wd, lt->pntsu*lt->pntsv*lt->pntsw, lt->dvert);
			
		}
		lt= lt->id.next;
	}
}

static void write_images(WriteData *wd, ListBase *idbase)
{
	Image *ima;
	PackedFile * pf;
	PreviewImage *prv;

	ima= idbase->first;
	while(ima) {
		if(ima->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_IM, "Image", 1, ima);
			if (ima->id.properties) IDP_WriteProperty(ima->id.properties, wd);

			if (ima->packedfile) {
				pf = ima->packedfile;
				writestruct(wd, DATA, "PackedFile", 1, pf);
				writedata(wd, DATA, pf->size, pf->data);
			}

			if (ima->preview) {
				prv = ima->preview;
				writestruct(wd, DATA, "PreviewImage", 1, prv);
				writedata(wd, DATA, prv->w*prv->h*sizeof(unsigned int), prv->rect);
			}
		}
		ima= ima->id.next;
	}
	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_textures(WriteData *wd, ListBase *idbase)
{
	Tex *tex;

	tex= idbase->first;
	while(tex) {
		if(tex->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_TE, "Tex", 1, tex);
			if (tex->id.properties) IDP_WriteProperty(tex->id.properties, wd);

			/* direct data */
			if(tex->plugin) writestruct(wd, DATA, "PluginTex", 1, tex->plugin);
			if(tex->coba) writestruct(wd, DATA, "ColorBand", 1, tex->coba);
			if(tex->env) writestruct(wd, DATA, "EnvMap", 1, tex->env);
		}
		tex= tex->id.next;
	}

	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_materials(WriteData *wd, ListBase *idbase)
{
	Material *ma;
	int a;

	ma= idbase->first;
	while(ma) {
		if(ma->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_MA, "Material", 1, ma);
			
			/*Write ID Properties -- and copy this comment EXACTLY for easy finding
			  of library blocks that implement this.*/
			/*manually set head group property to IDP_GROUP, just in case it hadn't been
			  set yet :) */
			if (ma->id.properties) IDP_WriteProperty(ma->id.properties, wd);

			for(a=0; a<MAX_MTEX; a++) {
				if(ma->mtex[a]) writestruct(wd, DATA, "MTex", 1, ma->mtex[a]);
			}
			
			if(ma->ramp_col) writestruct(wd, DATA, "ColorBand", 1, ma->ramp_col);
			if(ma->ramp_spec) writestruct(wd, DATA, "ColorBand", 1, ma->ramp_spec);
			
			write_scriptlink(wd, &ma->scriptlink);
			
			/* nodetree is integral part of material, no libdata */
			if(ma->nodetree) {
				writestruct(wd, DATA, "bNodeTree", 1, ma->nodetree);
				write_nodetree(wd, ma->nodetree);
			}
		}
		ma= ma->id.next;
	}
}

static void write_worlds(WriteData *wd, ListBase *idbase)
{
	World *wrld;
	int a;

	wrld= idbase->first;
	while(wrld) {
		if(wrld->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_WO, "World", 1, wrld);
			if (wrld->id.properties) IDP_WriteProperty(wrld->id.properties, wd);

			for(a=0; a<MAX_MTEX; a++) {
				if(wrld->mtex[a]) writestruct(wd, DATA, "MTex", 1, wrld->mtex[a]);
			}

			write_scriptlink(wd, &wrld->scriptlink);
		}
		wrld= wrld->id.next;
	}
}

static void write_lamps(WriteData *wd, ListBase *idbase)
{
	Lamp *la;
	int a;

	la= idbase->first;
	while(la) {
		if(la->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_LA, "Lamp", 1, la);
			if (la->id.properties) IDP_WriteProperty(la->id.properties, wd);

			/* direct data */
			for(a=0; a<MAX_MTEX; a++) {
				if(la->mtex[a]) writestruct(wd, DATA, "MTex", 1, la->mtex[a]);
			}

			write_scriptlink(wd, &la->scriptlink);
		}
		la= la->id.next;
	}
}


static void write_scenes(WriteData *wd, ListBase *scebase)
{
	Scene *sce;
	Base *base;
	Editing *ed;
	Sequence *seq;
	MetaStack *ms;
	Strip *strip;
	TimeMarker *marker;
	SceneRenderLayer *srl;
	int a;
	
	sce= scebase->first;
	while(sce) {
		/* write LibData */
		writestruct(wd, ID_SCE, "Scene", 1, sce);
		if (sce->id.properties) IDP_WriteProperty(sce->id.properties, wd);

		/* direct data */
		base= sce->base.first;
		while(base) {
			writestruct(wd, DATA, "Base", 1, base);
			base= base->next;
		}

		writestruct(wd, DATA, "Radio", 1, sce->radio);
		writestruct(wd, DATA, "ToolSettings", 1, sce->toolsettings);

		for(a=0; a<MAX_MTEX; ++a)
			writestruct(wd, DATA, "MTex", 1, sce->sculptdata.mtex[a]);

		ed= sce->ed;
		if(ed) {
			writestruct(wd, DATA, "Editing", 1, ed);

			/* reset write flags too */
			WHILE_SEQ(&ed->seqbase) {
				if(seq->strip) seq->strip->done= 0;
				writestruct(wd, DATA, "Sequence", 1, seq);
			}
			END_SEQ

			WHILE_SEQ(&ed->seqbase) {
				if(seq->strip && seq->strip->done==0) {
					/* write strip with 'done' at 0 because readfile */

					if(seq->plugin) writestruct(wd, DATA, "PluginSeq", 1, seq->plugin);
					if(seq->effectdata) {
						switch(seq->type){
						case SEQ_COLOR:
							writestruct(wd, DATA, "SolidColorVars", 1, seq->effectdata);
							break;
						case SEQ_SPEED:
							writestruct(wd, DATA, "SpeedControlVars", 1, seq->effectdata);
							break;
						case SEQ_WIPE:
							writestruct(wd, DATA, "WipeVars", 1, seq->effectdata);
							break;
						case SEQ_GLOW:
							writestruct(wd, DATA, "GlowVars", 1, seq->effectdata);
							break;
						case SEQ_TRANSFORM:
							writestruct(wd, DATA, "TransformVars", 1, seq->effectdata);
							break;
						}
					}

					strip= seq->strip;
					writestruct(wd, DATA, "Strip", 1, strip);

					if(seq->type==SEQ_IMAGE)
						writestruct(wd, DATA, "StripElem", strip->len, strip->stripdata);
					else if(seq->type==SEQ_MOVIE || seq->type==SEQ_RAM_SOUND || seq->type == SEQ_HD_SOUND)
						writestruct(wd, DATA, "StripElem", 1, strip->stripdata);

					strip->done= 1;
				}
			}
			END_SEQ
				
			/* new; meta stack too, even when its nasty restore code */
			for(ms= ed->metastack.first; ms; ms= ms->next) {
				writestruct(wd, DATA, "MetaStack", 1, ms);
			}
		}

		write_scriptlink(wd, &sce->scriptlink);

		if (sce->r.avicodecdata) {
			writestruct(wd, DATA, "AviCodecData", 1, sce->r.avicodecdata);
			if (sce->r.avicodecdata->lpFormat) writedata(wd, DATA, sce->r.avicodecdata->cbFormat, sce->r.avicodecdata->lpFormat);
			if (sce->r.avicodecdata->lpParms) writedata(wd, DATA, sce->r.avicodecdata->cbParms, sce->r.avicodecdata->lpParms);
		}

		if (sce->r.qtcodecdata) {
			writestruct(wd, DATA, "QuicktimeCodecData", 1, sce->r.qtcodecdata);
			if (sce->r.qtcodecdata->cdParms) writedata(wd, DATA, sce->r.qtcodecdata->cdSize, sce->r.qtcodecdata->cdParms);
		}

		/* writing dynamic list of TimeMarkers to the blend file */
		for(marker= sce->markers.first; marker; marker= marker->next)
			writestruct(wd, DATA, "TimeMarker", 1, marker);
		
		for(srl= sce->r.layers.first; srl; srl= srl->next)
			writestruct(wd, DATA, "SceneRenderLayer", 1, srl);
		
		if(sce->nodetree) {
			writestruct(wd, DATA, "bNodeTree", 1, sce->nodetree);
			write_nodetree(wd, sce->nodetree);
		}
		
		sce= sce->id.next;
	}
	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_screens(WriteData *wd, ListBase *scrbase)
{
	bScreen *sc;
	ScrArea *sa;
	ScrVert *sv;
	ScrEdge *se;

	sc= scrbase->first;
	while(sc) {
		/* write LibData */
		writestruct(wd, ID_SCR, "Screen", 1, sc);
		if (sc->id.properties) IDP_WriteProperty(sc->id.properties, wd);

		/* direct data */
		sv= sc->vertbase.first;
		while(sv) {
			writestruct(wd, DATA, "ScrVert", 1, sv);
			sv= sv->next;
		}

		se= sc->edgebase.first;
		while(se) {
			writestruct(wd, DATA, "ScrEdge", 1, se);
			se= se->next;
		}

		sa= sc->areabase.first;
		while(sa) {
			SpaceLink *sl;
			Panel *pa;

			writestruct(wd, DATA, "ScrArea", 1, sa);

			pa= sa->panels.first;
			while(pa) {
				writestruct(wd, DATA, "Panel", 1, pa);
				pa= pa->next;
			}

			/* space handler scriptlinks */
			write_scriptlink(wd, &sa->scriptlink);

			sl= sa->spacedata.first;
			while(sl) {
				if(sl->spacetype==SPACE_VIEW3D) {
					View3D *v3d= (View3D*) sl;
					writestruct(wd, DATA, "View3D", 1, v3d);
					if(v3d->bgpic) writestruct(wd, DATA, "BGpic", 1, v3d->bgpic);
					if(v3d->localvd) writestruct(wd, DATA, "View3D", 1, v3d->localvd);
					if(v3d->clipbb) writestruct(wd, DATA, "BoundBox", 1, v3d->clipbb);
				}
				else if(sl->spacetype==SPACE_IPO) {
					writestruct(wd, DATA, "SpaceIpo", 1, sl);
				}
				else if(sl->spacetype==SPACE_BUTS) {
					writestruct(wd, DATA, "SpaceButs", 1, sl);
				}
				else if(sl->spacetype==SPACE_FILE) {
					writestruct(wd, DATA, "SpaceFile", 1, sl);
				}
				else if(sl->spacetype==SPACE_SEQ) {
					writestruct(wd, DATA, "SpaceSeq", 1, sl);
				}
				else if(sl->spacetype==SPACE_OOPS) {
					SpaceOops *so= (SpaceOops *)sl;
					Oops *oops;

					/* cleanup */
					oops= so->oops.first;
					while(oops) {
						Oops *oopsn= oops->next;
						if(oops->id==0) {
							BLI_remlink(&so->oops, oops);
							free_oops(oops);
						}
						oops= oopsn;
					}

					/* ater cleanup, because of listbase! */
					writestruct(wd, DATA, "SpaceOops", 1, so);

					oops= so->oops.first;
					while(oops) {
						writestruct(wd, DATA, "Oops", 1, oops);
						oops= oops->next;
					}
					/* outliner */
					if(so->treestore) {
						writestruct(wd, DATA, "TreeStore", 1, so->treestore);
						if(so->treestore->data)
							writestruct(wd, DATA, "TreeStoreElem", so->treestore->usedelem, so->treestore->data);
					}
				}
				else if(sl->spacetype==SPACE_IMAGE) {
					SpaceImage *sima= (SpaceImage *)sl;
					
					writestruct(wd, DATA, "SpaceImage", 1, sl);
					if(sima->cumap)
						write_curvemapping(wd, sima->cumap);					
				}
				else if(sl->spacetype==SPACE_IMASEL) {
					writestruct(wd, DATA, "SpaceImaSel", 1, sl);
				}
				else if(sl->spacetype==SPACE_TEXT) {
					writestruct(wd, DATA, "SpaceText", 1, sl);
				}
				else if(sl->spacetype==SPACE_SCRIPT) {
					writestruct(wd, DATA, "SpaceScript", 1, sl);
				}
				else if(sl->spacetype==SPACE_ACTION) {
					writestruct(wd, DATA, "SpaceAction", 1, sl);
				}
				else if(sl->spacetype==SPACE_SOUND) {
					writestruct(wd, DATA, "SpaceSound", 1, sl);
				}
				else if(sl->spacetype==SPACE_NLA){
					writestruct(wd, DATA, "SpaceNla", 1, sl);
				}
				else if(sl->spacetype==SPACE_TIME){
					writestruct(wd, DATA, "SpaceTime", 1, sl);
				}
				else if(sl->spacetype==SPACE_NODE){
					writestruct(wd, DATA, "SpaceNode", 1, sl);
				}
				sl= sl->next;
			}

			sa= sa->next;
		}

		sc= sc->id.next;
	}
}

static void write_libraries(WriteData *wd, Main *main)
{
	ListBase *lbarray[30];
	ID *id;
	int a, tot, foundone;

	for(; main; main= main->next) {

		a=tot= set_listbasepointers(main, lbarray);

		/* test: is lib being used */
		foundone= 0;
		while(tot--) {
			for(id= lbarray[tot]->first; id; id= id->next) {
				if(id->us>0 && (id->flag & LIB_EXTERN)) {
					foundone= 1;
					break;
				}
			}
			if(foundone) break;
		}

		if(foundone) {
			writestruct(wd, ID_LI, "Library", 1, main->curlib);

			while(a--) {
				for(id= lbarray[a]->first; id; id= id->next) {
					if(id->us>0 && (id->flag & LIB_EXTERN)) {
						writestruct(wd, ID_ID, "ID", 1, id);
					}
				}
			}
		}
	}
}

static void write_bone(WriteData *wd, Bone* bone)
{
	Bone*	cbone;

	// PATCH for upward compatibility after 2.37+ armature recode
	bone->size[0]= bone->size[1]= bone->size[2]= 1.0f;
		
	// Write this bone
	writestruct(wd, DATA, "Bone", 1, bone);
	
	// Write Children
	cbone= bone->childbase.first;
	while(cbone) {
		write_bone(wd, cbone);
		cbone= cbone->next;
	}
}

static void write_armatures(WriteData *wd, ListBase *idbase)
{
	bArmature	*arm;
	Bone		*bone;

	arm=idbase->first;
	while (arm) {
		if (arm->id.us>0 || wd->current) {
			writestruct(wd, ID_AR, "bArmature", 1, arm);
			if (arm->id.properties) IDP_WriteProperty(arm->id.properties, wd);

			/* Direct data */
			bone= arm->bonebase.first;
			while(bone) {
				write_bone(wd, bone);
				bone=bone->next;
			}
		}
		arm=arm->id.next;
	}

	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_actions(WriteData *wd, ListBase *idbase)
{
	bAction			*act;
	bActionChannel	*chan;
	TimeMarker 		*marker;
	
	for(act=idbase->first; act; act= act->id.next) {
		if (act->id.us>0 || wd->current) {
			writestruct(wd, ID_AC, "bAction", 1, act);
			if (act->id.properties) IDP_WriteProperty(act->id.properties, wd);

			for (chan=act->chanbase.first; chan; chan=chan->next) {
				writestruct(wd, DATA, "bActionChannel", 1, chan);
				write_constraint_channels(wd, &chan->constraintChannels);
			}
			
			/* writing dynamic list of TimeMarkers to the blend file */
			for(marker= act->markers.first; marker; marker= marker->next)
				writestruct(wd, DATA, "TimeMarker", 1, marker);
		}
	}
}

static void write_texts(WriteData *wd, ListBase *idbase)
{
	Text *text;
	TextLine *tmp;

	text= idbase->first;
	while(text) {
		if ( (text->flags & TXT_ISMEM) && (text->flags & TXT_ISEXT)) text->flags &= ~TXT_ISEXT;

		/* write LibData */
		writestruct(wd, ID_TXT, "Text", 1, text);
		if(text->name) writedata(wd, DATA, strlen(text->name)+1, text->name);
		if (text->id.properties) IDP_WriteProperty(text->id.properties, wd);

		if(!(text->flags & TXT_ISEXT)) {
			/* now write the text data, in two steps for optimization in the readfunction */
			tmp= text->lines.first;
			while (tmp) {
				writestruct(wd, DATA, "TextLine", 1, tmp);
				tmp= tmp->next;
			}

			tmp= text->lines.first;
			while (tmp) {
				writedata(wd, DATA, tmp->len+1, tmp->line);
				tmp= tmp->next;
			}
		}
		text= text->id.next;
	}

	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_sounds(WriteData *wd, ListBase *idbase)
{
	bSound *sound;
	bSample *sample;

	PackedFile * pf;

	// set all samples to unsaved status

	sample = samples->first;
	while (sample) {
		sample->flags |= SAMPLE_NEEDS_SAVE;
		sample = sample->id.next;
	}

	sound= idbase->first;
	while(sound) {
		if(sound->id.us>0 || wd->current) {
			// do we need to save the packedfile as well ?
			sample = sound->sample;
			if (sample) {
				if (sample->flags & SAMPLE_NEEDS_SAVE) {
					sound->newpackedfile = sample->packedfile;
					sample->flags &= ~SAMPLE_NEEDS_SAVE;
				} else {
					sound->newpackedfile = NULL;
				}
			}

			/* write LibData */
			writestruct(wd, ID_SO, "bSound", 1, sound);
			if (sound->id.properties) IDP_WriteProperty(sound->id.properties, wd);

			if (sound->newpackedfile) {
				pf = sound->newpackedfile;
				writestruct(wd, DATA, "PackedFile", 1, pf);
				writedata(wd, DATA, pf->size, pf->data);
			}

			if (sample) {
				sound->newpackedfile = sample->packedfile;
			}
		}
		sound= sound->id.next;
	}

	/* flush helps the compression for undo-save */
	mywrite(wd, MYWRITE_FLUSH, 0);
}

static void write_groups(WriteData *wd, ListBase *idbase)
{
	Group *group;
	GroupObject *go;

	for(group= idbase->first; group; group= group->id.next) {
		if(group->id.us>0 || wd->current) {
			/* write LibData */
			writestruct(wd, ID_GR, "Group", 1, group);
			if (group->id.properties) IDP_WriteProperty(group->id.properties, wd);

			go= group->gobject.first;
			while(go) {
				writestruct(wd, DATA, "GroupObject", 1, go);
				go= go->next;
			}
		}
	}
}

static void write_nodetrees(WriteData *wd, ListBase *idbase)
{
	bNodeTree *ntree;
	
	for(ntree=idbase->first; ntree; ntree= ntree->id.next) {
		if (ntree->id.us>0 || wd->current) {
			writestruct(wd, ID_NT, "bNodeTree", 1, ntree);
			write_nodetree(wd, ntree);
			if (ntree->id.properties) IDP_WriteProperty(ntree->id.properties, wd);
		}
	}
}

static void write_brushes(WriteData *wd, ListBase *idbase)
{
	Brush *brush;
	int a;
	
	for(brush=idbase->first; brush; brush= brush->id.next) {
		if(brush->id.us>0 || wd->current) {
			writestruct(wd, ID_BR, "Brush", 1, brush);
			if (brush->id.properties) IDP_WriteProperty(brush->id.properties, wd);
			for(a=0; a<MAX_MTEX; a++)
				if(brush->mtex[a])
					writestruct(wd, DATA, "MTex", 1, brush->mtex[a]);
		}
	}
}

static void write_global(WriteData *wd)
{
	FileGlobal fg;

	fg.curscreen= G.curscreen;
	fg.curscene= G.scene;
	fg.displaymode= G.displaymode;
	fg.winpos= G.winpos;
	fg.fileflags= (G.fileflags & ~G_FILE_NO_UI);	// prevent to save this, is not good convention, and feature with concerns...
	fg.globalf= G.f;

	writestruct(wd, GLOB, "FileGlobal", 1, &fg);
}

/* if MemFile * there's filesave to memory */
static int write_file_handle(int handle, MemFile *compare, MemFile *current, int write_user_block, int write_flags)
{
	BHead bhead;
	ListBase mainlist;
	char buf[13];
	WriteData *wd;

	blo_split_main(&mainlist, G.main);

	wd= bgnwrite(handle, compare, current, write_flags);
	
	sprintf(buf, "BLENDER%c%c%.3d", (sizeof(void*)==8)?'-':'_', (G.order==B_ENDIAN)?'V':'v', G.version);
	mywrite(wd, buf, 12);

	write_renderinfo(wd);
	
	if(current==NULL)
		write_screens  (wd, &G.main->screen);	/* no UI save in undo */
	write_scenes   (wd, &G.main->scene);
	write_curves   (wd, &G.main->curve);
	write_mballs   (wd, &G.main->mball);
	write_images   (wd, &G.main->image);
	write_cameras  (wd, &G.main->camera);
	write_lamps    (wd, &G.main->lamp);
	write_lattices (wd, &G.main->latt);
	write_vfonts   (wd, &G.main->vfont);
	write_ipos     (wd, &G.main->ipo);
	write_keys     (wd, &G.main->key);
	write_worlds   (wd, &G.main->world);
	write_texts    (wd, &G.main->text);
	write_sounds   (wd, &G.main->sound);
	write_groups   (wd, &G.main->group);
	write_armatures(wd, &G.main->armature);
	write_actions  (wd, &G.main->action);
	write_objects  (wd, &G.main->object);
	write_materials(wd, &G.main->mat);
	write_textures (wd, &G.main->tex);
	write_meshs    (wd, &G.main->mesh);
	write_nodetrees(wd, &G.main->nodetree);
	write_brushes  (wd, &G.main->brush);
	if(current==NULL)	
		write_libraries(wd,  G.main->next); /* no library save in undo */

	write_global(wd);
	if (write_user_block) {
		write_userdef(wd);
	}

	/* dna as last, because (to be implemented) test for which structs are written */
	writedata(wd, DNA1, wd->sdna->datalen, wd->sdna->data);

	/* end of file */
	memset(&bhead, 0, sizeof(BHead));
	bhead.code= ENDB;
	mywrite(wd, &bhead, sizeof(BHead));

	blo_join_main(&mainlist);
	G.main= mainlist.first;

	return endwrite(wd);
}

/* return: success (1) */
int BLO_write_file(char *dir, int write_flags, char **error_r)
{
	char userfilename[FILE_MAXDIR+FILE_MAXFILE];
	char tempname[FILE_MAXDIR+FILE_MAXFILE];
	int file, err, write_user_block;

	sprintf(tempname, "%s@", dir);

	file = open(tempname,O_BINARY+O_WRONLY+O_CREAT+O_TRUNC, 0666);
	if(file == -1) {
		*error_r= "Unable to open";
		return 0;
	}

	BLI_make_file_string(G.sce, userfilename, BLI_gethome(), ".B.blend");

	write_user_block= BLI_streq(dir, userfilename);

	err= write_file_handle(file, NULL,NULL, write_user_block, write_flags);
	close(file);

	if(!err) {
		if(write_flags & G_FILE_COMPRESS)
		{	
			// compressed files have the same ending as regular files... only from 2.4!!!
			
			int ret = BLI_gzip(tempname, dir);
			
			if(-1==ret) {
				*error_r= "Failed opening .gz file";
				return 0;
			}
			if(-2==ret) {
				*error_r= "Failed opening .blend file for compression";
				return 0;
			}
		}
		else
		if(BLI_rename(tempname, dir) < 0) {
			*error_r= "Can't change old file. File saved with @";
			return 0;
		}

		
	} else {
		remove(tempname);

		*error_r= "Not enough diskspace";
		return 0;
	}

	return 1;
}

/* return: success (1) */
int BLO_write_file_mem(MemFile *compare, MemFile *current, int write_flags, char **error_r)
{
	int err;

	err= write_file_handle(0, compare, current, 0, write_flags);
	
	if(err==0) return 1;
	return 0;
}


	/* Runtime writing */

#ifdef WIN32
#define PATHSEPERATOR		"\\"
#else
#define PATHSEPERATOR		"/"
#endif

static char *get_install_dir(void) {
	extern char bprogname[];
	char *tmpname = BLI_strdup(bprogname);
	char *cut;

#ifdef __APPLE__
	cut = strstr(tmpname, ".app");
	if (cut) cut[0] = 0;
#endif

	cut = BLI_last_slash(tmpname);

	if (cut) {
		cut[0] = 0;
		return tmpname;
	} else {
		MEM_freeN(tmpname);
		return NULL;
	}
}

static char *get_runtime_path(char *exename) {
	char *installpath= get_install_dir();

	if (!installpath) {
		return NULL;
	} else {
		char *path= MEM_mallocN(strlen(installpath)+strlen(PATHSEPERATOR)+strlen(exename)+1, "runtimepath");

		if (path == NULL) {
			MEM_freeN(installpath);
			return NULL;
		}

		strcpy(path, installpath);
		strcat(path, PATHSEPERATOR);
		strcat(path, exename);

		MEM_freeN(installpath);

		return path;
	}
}

#ifdef __APPLE__

static int recursive_copy_runtime(char *outname, char *exename, char **cause_r) 
{
	char *cause = NULL, *runtime = get_runtime_path(exename);
	char command[2 * (FILE_MAXDIR+FILE_MAXFILE) + 32];
	int progfd = -1;

	if (!runtime) {
		cause= "Unable to find runtime";
		goto cleanup;
	}
	//printf("runtimepath %s\n", runtime);
		
	progfd= open(runtime, O_BINARY|O_RDONLY, 0);
	if (progfd==-1) {
		cause= "Unable to find runtime";
		goto cleanup;
	}

	sprintf(command, "/bin/cp -R \"%s\" \"%s\"", runtime, outname);
	//printf("command %s\n", command);
	if (system(command) == -1) {
		cause = "Couldn't copy runtime";
	}

cleanup:
	if (progfd!=-1)
		close(progfd);
	if (runtime)
		MEM_freeN(runtime);

	if (cause) {
		*cause_r= cause;
		return 0;
	} else
		return 1;
}

void BLO_write_runtime(char *file, char *exename) {
	char gamename[FILE_MAXDIR+FILE_MAXFILE];
	int outfd = -1;
	char *cause= NULL;

	// remove existing file / bundle
	//printf("Delete file %s\n", file);
	BLI_delete(file, 0, TRUE);

	if (!recursive_copy_runtime(file, exename, &cause))
		goto cleanup;

	strcpy(gamename, file);
	strcat(gamename, "/Contents/Resources/game.blend");
	//printf("gamename %s\n", gamename);
	outfd= open(gamename, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0777);
	if (outfd != -1) {

		write_file_handle(outfd, NULL,NULL, 0, G.fileflags);

		if (write(outfd, " ", 1) != 1) {
			cause= "Unable to write to output file";
			goto cleanup;
		}
	} else {
		cause = "Unable to open blenderfile";
	}

cleanup:
	if (outfd!=-1)
		close(outfd);

	if (cause)
		error("Unable to make runtime: %s", cause);
}

#else /* !__APPLE__ */

static int handle_append_runtime(int handle, char *exename, char **cause_r) {
	char *cause= NULL, *runtime= get_runtime_path(exename);
	unsigned char buf[1024];
	int count, progfd= -1;

	if (!runtime) {
		cause= "Unable to find runtime";
		goto cleanup;
	}

	progfd= open(runtime, O_BINARY|O_RDONLY, 0);
	if (progfd==-1) {
		cause= "Unable to find runtime";
		goto cleanup;
	}

	while ((count= read(progfd, buf, sizeof(buf)))>0) {
		if (write(handle, buf, count)!=count) {
			cause= "Unable to write to output file";
			goto cleanup;
		}
	}

cleanup:
	if (progfd!=-1)
		close(progfd);
	if (runtime)
		MEM_freeN(runtime);

	if (cause) {
		*cause_r= cause;
		return 0;
	} else
		return 1;
}

static int handle_write_msb_int(int handle, int i) {
	unsigned char buf[4];
	buf[0]= (i>>24)&0xFF;
	buf[1]= (i>>16)&0xFF;
	buf[2]= (i>>8)&0xFF;
	buf[3]= (i>>0)&0xFF;

	return (write(handle, buf, 4)==4);
}

void BLO_write_runtime(char *file, char *exename) {
	int outfd= open(file, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0777);
	char *cause= NULL;
	int datastart;

	if (!outfd) {
		cause= "Unable to open output file";
		goto cleanup;
	}
	if (!handle_append_runtime(outfd, exename, &cause))
		goto cleanup;

	datastart= lseek(outfd, 0, SEEK_CUR);

	write_file_handle(outfd, NULL,NULL, 0, G.fileflags);

	if (!handle_write_msb_int(outfd, datastart) || (write(outfd, "BRUNTIME", 8)!=8)) {
		cause= "Unable to write to output file";
		goto cleanup;
	}

cleanup:
	if (outfd!=-1)
		close(outfd);

	if (cause)
		error("Unable to make runtime: %s", cause);
}

#endif /* !__APPLE__ */
