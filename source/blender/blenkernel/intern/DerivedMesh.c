/**
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
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <zlib.h>

#include "PIL_time.h"

#include "MEM_guardedalloc.h"

#include "DNA_effect_types.h"
#include "DNA_mesh_types.h"
#include "DNA_key_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"
#include "DNA_object_force.h"
#include "DNA_object_fluidsim.h" // N_T
#include "DNA_scene_types.h" // N_T

#include "BLI_arithb.h"
#include "BLI_blenlib.h"
#include "BLI_edgehash.h"
#include "BLI_editVert.h"

#include "BKE_utildefines.h"
#include "BKE_cdderivedmesh.h"
#include "BKE_customdata.h"
#include "BKE_DerivedMesh.h"
#include "BKE_displist.h"
#include "BKE_effect.h"
#include "BKE_global.h"
#include "BKE_material.h"
#include "BKE_mesh.h"
#include "BKE_object.h"
#include "BKE_subsurf.h"
#include "BKE_deform.h"
#include "BKE_modifier.h"
#include "BKE_key.h"

#ifdef WITH_VERSE
#include "BKE_verse.h"
#endif

#include "BIF_gl.h"
#include "BIF_glutil.h"

#include "multires.h"

// headers for fluidsim bobj meshes
#include <stdlib.h>
#include "LBM_fluidsim.h"
#include "elbeem.h"

///////////////////////////////////
///////////////////////////////////
#define DERIVEDMESH_INITIAL_LAYERS 5

MVert *dm_dupVertArray(DerivedMesh *dm)
{
	MVert *tmp = MEM_callocN(sizeof(*tmp) * dm->getNumVerts(dm),
	                         "dm_dupVertArray tmp");

	if(tmp) dm->getVertArray(dm, tmp);

	return tmp;
}

MEdge *dm_dupEdgeArray(DerivedMesh *dm)
{
	MEdge *tmp = MEM_callocN(sizeof(*tmp) * dm->getNumEdges(dm),
	                         "dm_dupEdgeArray tmp");

	if(tmp) dm->getEdgeArray(dm, tmp);

	return tmp;
}

MFace *dm_dupFaceArray(DerivedMesh *dm)
{
	MFace *tmp = MEM_callocN(sizeof(*tmp) * dm->getNumFaces(dm),
	                         "dm_dupFaceArray tmp");

	if(tmp) dm->getFaceArray(dm, tmp);

	return tmp;
}

void DM_init_funcs(DerivedMesh *dm)
{
	/* default function implementations */
	dm->dupVertArray = dm_dupVertArray;
	dm->dupEdgeArray = dm_dupEdgeArray;
	dm->dupFaceArray = dm_dupFaceArray;

	dm->getVertData = DM_get_vert_data;
	dm->getEdgeData = DM_get_edge_data;
	dm->getFaceData = DM_get_face_data;
	dm->getVertDataArray = DM_get_vert_data_layer;
	dm->getEdgeDataArray = DM_get_edge_data_layer;
	dm->getFaceDataArray = DM_get_face_data_layer;
}

void DM_init(DerivedMesh *dm,
             int numVerts, int numEdges, int numFaces)
{
	CustomData_init(&dm->vertData, DERIVEDMESH_INITIAL_LAYERS, numVerts,
	                SUB_ELEMS_VERT);
	CustomData_init(&dm->edgeData, DERIVEDMESH_INITIAL_LAYERS, numEdges,
	                SUB_ELEMS_EDGE);
	CustomData_init(&dm->faceData, DERIVEDMESH_INITIAL_LAYERS, numFaces,
	                SUB_ELEMS_FACE);

	CustomData_add_layer(&dm->vertData, LAYERTYPE_ORIGINDEX, 0, NULL);
	CustomData_add_layer(&dm->edgeData, LAYERTYPE_ORIGINDEX, 0, NULL);
	CustomData_add_layer(&dm->faceData, LAYERTYPE_ORIGINDEX, 0, NULL);

	DM_init_funcs(dm);
}

void DM_from_template(DerivedMesh *dm, DerivedMesh *source,
                      int numVerts, int numEdges, int numFaces)
{
	CustomData_from_template(&source->vertData, &dm->vertData, 0, numVerts);
	CustomData_from_template(&source->edgeData, &dm->edgeData, 0, numEdges);
	CustomData_from_template(&source->faceData, &dm->faceData, 0, numFaces);

	DM_init_funcs(dm);
}

void DM_release(DerivedMesh *dm)
{
	CustomData_free(&dm->vertData);
	CustomData_free(&dm->edgeData);
	CustomData_free(&dm->faceData);
}

void DM_to_mesh(DerivedMesh *dm, Mesh *me)
{
	/* dm might depend on me, so we need to do everything with a local copy */
	Mesh tmp_me = *me;
	int numVerts = dm->getNumVerts(dm);

	tmp_me.dvert = NULL;
	tmp_me.tface = NULL;
	tmp_me.mcol = NULL;

	tmp_me.totvert = numVerts;
	tmp_me.totedge = dm->getNumEdges(dm);
	tmp_me.totface = dm->getNumFaces(dm);

	tmp_me.mvert = dm->dupVertArray(dm);
	tmp_me.medge = dm->dupEdgeArray(dm);
	tmp_me.mface = dm->dupFaceArray(dm);

	if(dm->getFaceDataArray(dm, LAYERTYPE_TFACE))
		tmp_me.tface = MEM_dupallocN(dm->getFaceDataArray(dm,
		                                                  LAYERTYPE_TFACE));
	if(dm->getFaceDataArray(dm, LAYERTYPE_MCOL))
		tmp_me.mcol = MEM_dupallocN(dm->getFaceDataArray(dm,
		                                                 LAYERTYPE_MCOL));
	if(dm->getVertDataArray(dm, LAYERTYPE_MDEFORMVERT)) {
		int i;
		MDeformVert *dv;

		tmp_me.dvert = MEM_dupallocN(
		                    dm->getVertDataArray(dm, LAYERTYPE_MDEFORMVERT));

		for(i = 0, dv = tmp_me.dvert; i < numVerts; ++i, ++dv)
			dv->dw = MEM_dupallocN(dv->dw);
	}

	if(me->mvert) MEM_freeN(me->mvert);
	if(me->dvert) free_dverts(me->dvert, me->totvert);
	if(me->mface) MEM_freeN(me->mface);
	if(me->tface) MEM_freeN(me->tface);
	if(me->mcol) MEM_freeN(me->mcol);
	if(me->medge) MEM_freeN(me->medge);

	/* if the number of verts has changed, remove invalid data */
	if(numVerts != me->totvert) {
		if(me->msticky) MEM_freeN(me->msticky);
		me->msticky = NULL;

		if(me->key) me->key->id.us--;
		me->key = NULL;
	}

	*me = tmp_me;
}

void DM_add_vert_layer(DerivedMesh *dm, int type, int flag, void *layer)
{
	CustomData_add_layer(&dm->vertData, type, flag, layer);
}

void DM_add_edge_layer(DerivedMesh *dm, int type, int flag, void *layer)
{
	CustomData_add_layer(&dm->edgeData, type, flag, layer);
}

void DM_add_face_layer(DerivedMesh *dm, int type, int flag, void *layer)
{
	CustomData_add_layer(&dm->faceData, type, flag, layer);
}

void *DM_get_vert_data(DerivedMesh *dm, int index, int type)
{
	return CustomData_get(&dm->vertData, index, type);
}

void *DM_get_edge_data(DerivedMesh *dm, int index, int type)
{
	return CustomData_get(&dm->edgeData, index, type);
}

void *DM_get_face_data(DerivedMesh *dm, int index, int type)
{
	return CustomData_get(&dm->faceData, index, type);
}

void *DM_get_vert_data_layer(DerivedMesh *dm, int type)
{
	return CustomData_get_layer(&dm->vertData, type);
}

void *DM_get_edge_data_layer(DerivedMesh *dm, int type)
{
	return CustomData_get_layer(&dm->edgeData, type);
}

void *DM_get_face_data_layer(DerivedMesh *dm, int type)
{
	return CustomData_get_layer(&dm->faceData, type);
}

void DM_set_vert_data(DerivedMesh *dm, int index, int type, void *data)
{
	CustomData_set(&dm->vertData, index, type, data);
}

void DM_set_edge_data(DerivedMesh *dm, int index, int type, void *data)
{
	CustomData_set(&dm->edgeData, index, type, data);
}

void DM_set_face_data(DerivedMesh *dm, int index, int type, void *data)
{
	CustomData_set(&dm->faceData, index, type, data);
}

void DM_copy_vert_data(DerivedMesh *source, DerivedMesh *dest,
                       int source_index, int dest_index, int count)
{
	CustomData_copy_data(&source->vertData, &dest->vertData,
	                     source_index, dest_index,
	                     count);
}

void DM_copy_edge_data(DerivedMesh *source, DerivedMesh *dest,
                       int source_index, int dest_index, int count)
{
	CustomData_copy_data(&source->edgeData, &dest->edgeData,
	                     source_index, dest_index,
	                     count);
}

void DM_copy_face_data(DerivedMesh *source, DerivedMesh *dest,
                       int source_index, int dest_index, int count)
{
	CustomData_copy_data(&source->faceData, &dest->faceData,
	                     source_index, dest_index,
	                     count);
}

void DM_free_vert_data(struct DerivedMesh *dm, int index, int count)
{
	CustomData_free_elem(&dm->vertData, index, count);
}

void DM_free_edge_data(struct DerivedMesh *dm, int index, int count)
{
	CustomData_free_elem(&dm->edgeData, index, count);
}

void DM_free_face_data(struct DerivedMesh *dm, int index, int count)
{
	CustomData_free_elem(&dm->faceData, index, count);
}

void DM_interp_vert_data(DerivedMesh *source, DerivedMesh *dest,
                         int *src_indices, float *weights,
                         int count, int dest_index)
{
	CustomData_interp(&source->vertData, &dest->vertData, src_indices,
	                  weights, NULL, count, dest_index);
}

void DM_interp_edge_data(DerivedMesh *source, DerivedMesh *dest,
                         int *src_indices,
                         float *weights, EdgeVertWeight *vert_weights,
                         int count, int dest_index)
{
	CustomData_interp(&source->edgeData, &dest->edgeData, src_indices,
	                  weights, (float*)vert_weights, count, dest_index);
}

void DM_interp_face_data(DerivedMesh *source, DerivedMesh *dest,
                         int *src_indices,
                         float *weights, FaceVertWeight *vert_weights,
                         int count, int dest_index)
{
	CustomData_interp(&source->faceData, &dest->faceData, src_indices,
	                  weights, (float*)vert_weights, count, dest_index);
}

typedef struct {
	DerivedMesh dm;

	Object *ob;
	Mesh *me;
	MVert *verts;
	float *nors;
	MCol *wpaintMCol;

	int freeNors, freeVerts;
} MeshDerivedMesh;

static DispListMesh *meshDM_convertToDispListMesh(DerivedMesh *dm, int allowShared)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	DispListMesh *dlm = MEM_callocN(sizeof(*dlm), "dlm");

	dlm->totvert = me->totvert;
	dlm->totedge = me->totedge;
	dlm->totface = me->totface;
	dlm->mvert = mdm->verts;
	dlm->medge = me->medge;
	dlm->mface = me->mface;
	dlm->tface = me->tface;
	dlm->mcol = me->mcol;
	dlm->nors = mdm->nors;
	dlm->dontFreeVerts = dlm->dontFreeOther = dlm->dontFreeNors = 1;

	if (!allowShared) {
		dlm->mvert = MEM_dupallocN(dlm->mvert);
		if (dlm->nors) dlm->nors = MEM_dupallocN(dlm->nors);

		dlm->dontFreeVerts = dlm->dontFreeNors = 0;
	}

	return dlm;
}

static void meshDM_getMinMax(DerivedMesh *dm, float min_r[3], float max_r[3])
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	int i;

	if (me->totvert) {
		for (i=0; i<me->totvert; i++) {
			DO_MINMAX(mdm->verts[i].co, min_r, max_r);
		}
	} else {
		min_r[0] = min_r[1] = min_r[2] = max_r[0] = max_r[1] = max_r[2] = 0.0;
	}
}

static void meshDM_getVertCos(DerivedMesh *dm, float (*cos_r)[3])
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	int i;

	for (i=0; i<me->totvert; i++) {
		cos_r[i][0] = mdm->verts[i].co[0];
		cos_r[i][1] = mdm->verts[i].co[1];
		cos_r[i][2] = mdm->verts[i].co[2];
	}
}

static void meshDM_getVertCo(DerivedMesh *dm, int index, float co_r[3])
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;

	VECCOPY(co_r, mdm->verts[index].co);
}

static void meshDM_getVertNo(DerivedMesh *dm, int index, float no_r[3])
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	short *no = mdm->verts[index].no;

	no_r[0] = no[0]/32767.f;
	no_r[1] = no[1]/32767.f;
	no_r[2] = no[2]/32767.f;
}

static void meshDM_drawVerts(DerivedMesh *dm)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	int i;

	glBegin(GL_POINTS);
	for(i=0; i<me->totvert; i++) {
		glVertex3fv(mdm->verts[i].co);
	}
	glEnd();
}
static void meshDM_drawUVEdges(DerivedMesh *dm)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	int i;

	if (me->tface) {
		glBegin(GL_LINES);
		for (i=0; i<me->totface; i++) {
			TFace *tf = &me->tface[i];

			if (!(tf->flag&TF_HIDE)) {
				glVertex2fv(tf->uv[0]);
				glVertex2fv(tf->uv[1]);

				glVertex2fv(tf->uv[1]);
				glVertex2fv(tf->uv[2]);

				if (!me->mface[i].v4) {
					glVertex2fv(tf->uv[2]);
					glVertex2fv(tf->uv[0]);
				} else {
					glVertex2fv(tf->uv[2]);
					glVertex2fv(tf->uv[3]);

					glVertex2fv(tf->uv[3]);
					glVertex2fv(tf->uv[0]);
				}
			}
		}
		glEnd();
	}
}
static void meshDM_drawEdges(DerivedMesh *dm, int drawLooseEdges)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me= mdm->me;
	MEdge *medge= me->medge;
	int i;
		
	glBegin(GL_LINES);
	for(i=0; i<me->totedge; i++, medge++) {
		if ((medge->flag&ME_EDGEDRAW) && (drawLooseEdges || !(medge->flag&ME_LOOSEEDGE))) {
			glVertex3fv(mdm->verts[medge->v1].co);
			glVertex3fv(mdm->verts[medge->v2].co);
		}
	}
	glEnd();
}
static void meshDM_drawMappedEdges(DerivedMesh *dm, int (*setDrawOptions)(void *userData, int index), void *userData)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me= mdm->me;
	int i;
		
	glBegin(GL_LINES);
	for (i=0; i<me->totedge; i++) {
		if (!setDrawOptions || setDrawOptions(userData, i)) {
			glVertex3fv(mdm->verts[me->medge[i].v1].co);
			glVertex3fv(mdm->verts[me->medge[i].v2].co);
		}
	}
	glEnd();
}
static void meshDM_drawLooseEdges(DerivedMesh *dm)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me= mdm->me;
	MEdge *medge= me->medge;
	int i;

	glBegin(GL_LINES);
	for (i=0; i<me->totedge; i++, medge++) {
		if (medge->flag&ME_LOOSEEDGE) {
			glVertex3fv(mdm->verts[medge->v1].co);
			glVertex3fv(mdm->verts[medge->v2].co);
		}
	}
	glEnd();
}
static void meshDM_drawFacesSolid(DerivedMesh *dm, int (*setMaterial)(int))
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	MVert *mvert= mdm->verts;
	MFace *mface= me->mface;
	float *nors = mdm->nors;
	int a;
	int glmode=-1, shademodel=-1, matnr=-1, drawCurrentMat=1;

#define PASSVERT(index) {						\
	if (shademodel==GL_SMOOTH) {				\
		short *no = mvert[index].no;			\
		glNormal3sv(no);						\
	}											\
	glVertex3fv(mvert[index].co);	\
}

	glBegin(glmode=GL_QUADS);
	for(a=0; a<me->totface; a++, mface++, nors+=3) {
		int new_glmode, new_matnr, new_shademodel;
			
		new_glmode = mface->v4?GL_QUADS:GL_TRIANGLES;
		new_matnr = mface->mat_nr+1;
		new_shademodel = (mface->flag & ME_SMOOTH)?GL_SMOOTH:GL_FLAT;
		
		if (new_glmode!=glmode || new_matnr!=matnr || new_shademodel!=shademodel) {
			glEnd();

			drawCurrentMat = setMaterial(matnr=new_matnr);

			glShadeModel(shademodel=new_shademodel);
			glBegin(glmode=new_glmode);
		} 
		
		if (drawCurrentMat) {
			if(shademodel==GL_FLAT) 
				glNormal3fv(nors);

			PASSVERT(mface->v1);
			PASSVERT(mface->v2);
			PASSVERT(mface->v3);
			if (mface->v4) {
				PASSVERT(mface->v4);
			}
		}
	}
	glEnd();

	glShadeModel(GL_FLAT);
#undef PASSVERT
}

static void meshDM_drawFacesColored(DerivedMesh *dm, int useTwoSide, unsigned char *col1, unsigned char *col2)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me= mdm->me;
	MFace *mface= me->mface;
	int a, glmode;
	unsigned char *cp1, *cp2;

	cp1= col1;
	if(col2) {
		cp2= col2;
	} else {
		cp2= NULL;
		useTwoSide= 0;
	}

	/* there's a conflict here... twosided colors versus culling...? */
	/* defined by history, only texture faces have culling option */
	/* we need that as mesh option builtin, next to double sided lighting */
	if(col1 && col2)
		glEnable(GL_CULL_FACE);
	
	glShadeModel(GL_SMOOTH);
	glBegin(glmode=GL_QUADS);
	for(a=0; a<me->totface; a++, mface++, cp1+= 16) {
		int new_glmode= mface->v4?GL_QUADS:GL_TRIANGLES;

		if (new_glmode!=glmode) {
			glEnd();
			glBegin(glmode= new_glmode);
		}
			
		glColor3ub(cp1[3], cp1[2], cp1[1]);
		glVertex3fv( mdm->verts[mface->v1].co );
		glColor3ub(cp1[7], cp1[6], cp1[5]);
		glVertex3fv( mdm->verts[mface->v2].co );
		glColor3ub(cp1[11], cp1[10], cp1[9]);
		glVertex3fv( mdm->verts[mface->v3].co );
		if(mface->v4) {
			glColor3ub(cp1[15], cp1[14], cp1[13]);
			glVertex3fv( mdm->verts[mface->v4].co );
		}
			
		if(useTwoSide) {
			glColor3ub(cp2[11], cp2[10], cp2[9]);
			glVertex3fv( mdm->verts[mface->v3].co );
			glColor3ub(cp2[7], cp2[6], cp2[5]);
			glVertex3fv( mdm->verts[mface->v2].co );
			glColor3ub(cp2[3], cp2[2], cp2[1]);
			glVertex3fv( mdm->verts[mface->v1].co );
			if(mface->v4) {
				glColor3ub(cp2[15], cp2[14], cp2[13]);
				glVertex3fv( mdm->verts[mface->v4].co );
			}
		}
		if(col2) cp2+= 16;
	}
	glEnd();

	glShadeModel(GL_FLAT);
	glDisable(GL_CULL_FACE);
}

static void meshDM_drawFacesTex_common(DerivedMesh *dm, int (*drawParams)(TFace *tface, int matnr), int (*drawParamsMapped)(void *userData, int index), void *userData) 
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	MVert *mvert= mdm->verts;
	MFace *mface= me->mface;
	TFace *tface = me->tface;
	float *nors = mdm->nors;
	int i;

	for (i=0; i<me->totface; i++) {
		MFace *mf= &mface[i];
		TFace *tf = tface?&tface[i]:NULL;
		int flag;
		unsigned char *cp= NULL;
		
		if (drawParams)
			flag = drawParams(tf, mf->mat_nr);
		else
			flag = drawParamsMapped(userData, i);

		if (flag==0) {
			continue;
		} else if (flag==1) {
			if (mdm->wpaintMCol) {
				cp= (unsigned char*) &mdm->wpaintMCol[i*4];
			} else if (tf) {
				cp= (unsigned char*) tf->col;
			} else if (me->mcol) {
				cp= (unsigned char*) &me->mcol[i*4];
			}
		}

		if (!(mf->flag&ME_SMOOTH)) {
			glNormal3fv(&nors[i*3]);
		}

		glBegin(mf->v4?GL_QUADS:GL_TRIANGLES);
		if (tf) glTexCoord2fv(tf->uv[0]);
		if (cp) glColor3ub(cp[3], cp[2], cp[1]);
		if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v1].no);
		glVertex3fv(mvert[mf->v1].co);
			
		if (tf) glTexCoord2fv(tf->uv[1]);
		if (cp) glColor3ub(cp[7], cp[6], cp[5]);
		if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v2].no);
		glVertex3fv(mvert[mf->v2].co);

		if (tf) glTexCoord2fv(tf->uv[2]);
		if (cp) glColor3ub(cp[11], cp[10], cp[9]);
		if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v3].no);
		glVertex3fv(mvert[mf->v3].co);

		if(mf->v4) {
			if (tf) glTexCoord2fv(tf->uv[3]);
			if (cp) glColor3ub(cp[15], cp[14], cp[13]);
			if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v4].no);
			glVertex3fv(mvert[mf->v4].co);
		}
		glEnd();
	}
}
static void meshDM_drawFacesTex(DerivedMesh *dm, int (*setDrawParams)(TFace *tface, int matnr)) 
{
	meshDM_drawFacesTex_common(dm, setDrawParams, NULL, NULL);
}
static void meshDM_drawMappedFacesTex(DerivedMesh *dm, int (*setDrawParams)(void *userData, int index), void *userData) 
{
	meshDM_drawFacesTex_common(dm, NULL, setDrawParams, userData);
}

static void meshDM_drawMappedFaces(DerivedMesh *dm, int (*setDrawOptions)(void *userData, int index, int *drawSmooth_r), void *userData, int useColors) 
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;
	MVert *mvert= mdm->verts;
	MFace *mface= me->mface;
	float *nors= mdm->nors;
	int i;

	for (i=0; i<me->totface; i++) {
		MFace *mf= &mface[i];
		int drawSmooth = (mf->flag & ME_SMOOTH);

		if (!setDrawOptions || setDrawOptions(userData, i, &drawSmooth)) {
			unsigned char *cp = NULL;

			if (useColors) {
				if (mdm->wpaintMCol) {
					cp= (unsigned char*) &mdm->wpaintMCol[i*4];
				} else if (me->tface) {
					cp= (unsigned char*) me->tface[i].col;
				} else if (me->mcol) {
					cp= (unsigned char*) &me->mcol[i*4];
				}
			}

			glShadeModel(drawSmooth?GL_SMOOTH:GL_FLAT);
			glBegin(mf->v4?GL_QUADS:GL_TRIANGLES);

			if (!drawSmooth) {
				glNormal3fv(&nors[i*3]);

				if (cp) glColor3ub(cp[3], cp[2], cp[1]);
				glVertex3fv(mvert[mf->v1].co);
				if (cp) glColor3ub(cp[7], cp[6], cp[5]);
				glVertex3fv(mvert[mf->v2].co);
				if (cp) glColor3ub(cp[11], cp[10], cp[9]);
				glVertex3fv(mvert[mf->v3].co);
				if(mf->v4) {
					if (cp) glColor3ub(cp[15], cp[14], cp[13]);
					glVertex3fv(mvert[mf->v4].co);
				}
			} else {
				if (cp) glColor3ub(cp[3], cp[2], cp[1]);
				glNormal3sv(mvert[mf->v1].no);
				glVertex3fv(mvert[mf->v1].co);
				if (cp) glColor3ub(cp[7], cp[6], cp[5]);
				glNormal3sv(mvert[mf->v2].no);
				glVertex3fv(mvert[mf->v2].co);
				if (cp) glColor3ub(cp[11], cp[10], cp[9]);
				glNormal3sv(mvert[mf->v3].no);
				glVertex3fv(mvert[mf->v3].co);
				if(mf->v4) {
					if (cp) glColor3ub(cp[15], cp[14], cp[13]);
					glNormal3sv(mvert[mf->v4].no);
					glVertex3fv(mvert[mf->v4].co);
				}
			}

			glEnd();
		}
	}
}
static int meshDM_getNumVerts(DerivedMesh *dm)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;

	return me->totvert;
}

static int meshDM_getNumEdges(DerivedMesh *dm)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;

	return me->totedge;
}

static int meshDM_getNumFaces(DerivedMesh *dm)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;
	Mesh *me = mdm->me;

	return me->totface;
}

void meshDM_getVert(DerivedMesh *dm, int index, MVert *vert_r)
{
	MVert *verts = ((MeshDerivedMesh *)dm)->verts;

	*vert_r = verts[index];
}

void meshDM_getEdge(DerivedMesh *dm, int index, MEdge *edge_r)
{
	Mesh *me = ((MeshDerivedMesh *)dm)->me;

	*edge_r = me->medge[index];
}

void meshDM_getFace(DerivedMesh *dm, int index, MFace *face_r)
{
	Mesh *me = ((MeshDerivedMesh *)dm)->me;

	*face_r = me->mface[index];
}

void meshDM_getVertArray(DerivedMesh *dm, MVert *vert_r)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh *)dm;
	memcpy(vert_r, mdm->verts, sizeof(*vert_r) * mdm->me->totvert);
}

void meshDM_getEdgeArray(DerivedMesh *dm, MEdge *edge_r)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh *)dm;
	memcpy(edge_r, mdm->me->medge, sizeof(*edge_r) * mdm->me->totedge);
}

void meshDM_getFaceArray(DerivedMesh *dm, MFace *face_r)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh *)dm;
	memcpy(face_r, mdm->me->mface, sizeof(*face_r) * mdm->me->totface);
}

static void meshDM_release(DerivedMesh *dm)
{
	MeshDerivedMesh *mdm = (MeshDerivedMesh*) dm;

	DM_release(dm);

	if (mdm->wpaintMCol) MEM_freeN(mdm->wpaintMCol);
	if (mdm->freeNors) MEM_freeN(mdm->nors);
	if (mdm->freeVerts) MEM_freeN(mdm->verts);
	MEM_freeN(mdm);
}

static DerivedMesh *getMeshDerivedMesh(Mesh *me, Object *ob, float (*vertCos)[3])
{
	MeshDerivedMesh *mdm = MEM_callocN(sizeof(*mdm), "mdm");

	DM_init(&mdm->dm, me->totvert, me->totedge, me->totface);

	mdm->dm.getMinMax = meshDM_getMinMax;

	mdm->dm.convertToDispListMesh = meshDM_convertToDispListMesh;
	mdm->dm.getNumVerts = meshDM_getNumVerts;
	mdm->dm.getNumEdges = meshDM_getNumEdges;
	mdm->dm.getNumFaces = meshDM_getNumFaces;

	mdm->dm.getVert = meshDM_getVert;
	mdm->dm.getEdge = meshDM_getEdge;
	mdm->dm.getFace = meshDM_getFace;
	mdm->dm.getVertArray = meshDM_getVertArray;
	mdm->dm.getEdgeArray = meshDM_getEdgeArray;
	mdm->dm.getFaceArray = meshDM_getFaceArray;

	mdm->dm.getVertCos = meshDM_getVertCos;
	mdm->dm.getVertCo = meshDM_getVertCo;
	mdm->dm.getVertNo = meshDM_getVertNo;

	mdm->dm.drawVerts = meshDM_drawVerts;

	mdm->dm.drawUVEdges = meshDM_drawUVEdges;
	mdm->dm.drawEdges = meshDM_drawEdges;
	mdm->dm.drawLooseEdges = meshDM_drawLooseEdges;
	
	mdm->dm.drawFacesSolid = meshDM_drawFacesSolid;
	mdm->dm.drawFacesColored = meshDM_drawFacesColored;
	mdm->dm.drawFacesTex = meshDM_drawFacesTex;
	mdm->dm.drawMappedFaces = meshDM_drawMappedFaces;
	mdm->dm.drawMappedFacesTex = meshDM_drawMappedFacesTex;

	mdm->dm.drawMappedEdges = meshDM_drawMappedEdges;
	mdm->dm.drawMappedFaces = meshDM_drawMappedFaces;

	mdm->dm.release = meshDM_release;

	/* add appropriate data layers (don't copy, just reference) */
	if(me->msticky)
		DM_add_vert_layer(&mdm->dm, LAYERTYPE_MSTICKY,
		                  LAYERFLAG_NOFREE, me->msticky);
	if(me->dvert)
		DM_add_vert_layer(&mdm->dm, LAYERTYPE_MDEFORMVERT,
		                  LAYERFLAG_NOFREE, me->dvert);

	if(me->tface)
		DM_add_face_layer(&mdm->dm, LAYERTYPE_TFACE,
		                  LAYERFLAG_NOFREE, me->tface);
	if(me->mcol)
		DM_add_face_layer(&mdm->dm, LAYERTYPE_MCOL,
		                  LAYERFLAG_NOFREE, me->mcol);

		/* Works in conjunction with hack during modifier calc */
	if ((G.f & G_WEIGHTPAINT) && ob==(G.scene->basact?G.scene->basact->object:NULL)) {
		mdm->wpaintMCol = MEM_dupallocN(me->mcol);
	}

	mdm->ob = ob;
	mdm->me = me;
	mdm->verts = me->mvert;
	mdm->nors = NULL;
	mdm->freeNors = 0;
	mdm->freeVerts = 0;

	if((ob->fluidsimFlag & OB_FLUIDSIM_ENABLE) &&
		 (ob->fluidsimSettings->type & OB_FLUIDSIM_DOMAIN)&&
	   (ob->fluidsimSettings->meshSurface) &&
		 (1) && (!give_parteff(ob)) && // doesnt work together with particle systems!
		 (me->totvert == ((Mesh *)(ob->fluidsimSettings->meshSurface))->totvert) ) {
		// dont recompute for fluidsim mesh, use from readBobjgz
		// TODO? check for modifiers!?
		int i;
		mesh_calc_normals(mdm->verts, me->totvert, me->mface, me->totface, &mdm->nors);
		mdm->freeNors = 1;
		for (i=0; i<me->totvert; i++) {
			MVert *mv= &mdm->verts[i];
			MVert *fsv; 
			fsv = &ob->fluidsimSettings->meshSurfNormals[i];
			VECCOPY(mv->no, fsv->no);
			//mv->no[0]= 30000; mv->no[1]= mv->no[2]= 0; // DEBUG fixed test normals
		}
	} else {
		// recompute normally

		if (vertCos) {
			int i;

			/* copy the original verts to preserve flag settings; if this is too
			 * costly, must at least use MEM_callocN to clear flags */
			mdm->verts = MEM_dupallocN( me->mvert );
			for (i=0; i<me->totvert; i++) {
				VECCOPY(mdm->verts[i].co, vertCos[i]);
			}
			mesh_calc_normals(mdm->verts, me->totvert, me->mface, me->totface, &mdm->nors);
			mdm->freeNors = 1;
			mdm->freeVerts = 1;
		} else {
			// XXX this is kinda hacky because we shouldn't really be editing
			// the mesh here, however, we can't just call mesh_build_faceNormals(ob)
			// because in the case when a key is applied to a mesh the vertex normals
			// would never be correctly computed.
			mesh_calc_normals(mdm->verts, me->totvert, me->mface, me->totface, &mdm->nors);
			mdm->freeNors = 1;
		}
	} // fs TEST

	return (DerivedMesh*) mdm;
}

///

typedef struct {
	DerivedMesh dm;

	EditMesh *em;
	float (*vertexCos)[3];
	float (*vertexNos)[3];
	float (*faceNos)[3];
} EditMeshDerivedMesh;

static void emDM_foreachMappedVert(DerivedMesh *dm, void (*func)(void *userData, int index, float *co, float *no_f, short *no_s), void *userData)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditVert *eve;
	int i;

	for (i=0,eve= emdm->em->verts.first; eve; i++,eve=eve->next) {
		if (emdm->vertexCos) {
			func(userData, i, emdm->vertexCos[i], emdm->vertexNos[i], NULL);
		} else {
			func(userData, i, eve->co, eve->no, NULL);
		}
	}
}
static void emDM_foreachMappedEdge(DerivedMesh *dm, void (*func)(void *userData, int index, float *v0co, float *v1co), void *userData)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditEdge *eed;
	int i;

	if (emdm->vertexCos) {
		EditVert *eve, *preveve;

		for (i=0,eve=emdm->em->verts.first; eve; eve= eve->next)
			eve->prev = (EditVert*) i++;
		for(i=0,eed= emdm->em->edges.first; eed; i++,eed= eed->next)
			func(userData, i, emdm->vertexCos[(int) eed->v1->prev], emdm->vertexCos[(int) eed->v2->prev]);
		for (preveve=NULL, eve=emdm->em->verts.first; eve; preveve=eve, eve= eve->next)
			eve->prev = preveve;
	} else {
		for(i=0,eed= emdm->em->edges.first; eed; i++,eed= eed->next)
			func(userData, i, eed->v1->co, eed->v2->co);
	}
}
static void emDM_drawMappedEdges(DerivedMesh *dm, int (*setDrawOptions)(void *userData, int index), void *userData) 
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditEdge *eed;
	int i;

	if (emdm->vertexCos) {
		EditVert *eve, *preveve;

		for (i=0,eve=emdm->em->verts.first; eve; eve= eve->next)
			eve->prev = (EditVert*) i++;

		glBegin(GL_LINES);
		for(i=0,eed= emdm->em->edges.first; eed; i++,eed= eed->next) {
			if(!setDrawOptions || setDrawOptions(userData, i)) {
				glVertex3fv(emdm->vertexCos[(int) eed->v1->prev]);
				glVertex3fv(emdm->vertexCos[(int) eed->v2->prev]);
			}
		}
		glEnd();

		for (preveve=NULL, eve=emdm->em->verts.first; eve; preveve=eve, eve= eve->next)
			eve->prev = preveve;
	} else {
		glBegin(GL_LINES);
		for(i=0,eed= emdm->em->edges.first; eed; i++,eed= eed->next) {
			if(!setDrawOptions || setDrawOptions(userData, i)) {
				glVertex3fv(eed->v1->co);
				glVertex3fv(eed->v2->co);
			}
		}
		glEnd();
	}
}
static void emDM_drawEdges(DerivedMesh *dm, int drawLooseEdges)
{
	emDM_drawMappedEdges(dm, NULL, NULL);
}
static void emDM_drawMappedEdgesInterp(DerivedMesh *dm, int (*setDrawOptions)(void *userData, int index), void (*setDrawInterpOptions)(void *userData, int index, float t), void *userData) 
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditEdge *eed;
	int i;

	if (emdm->vertexCos) {
		EditVert *eve, *preveve;

		for (i=0,eve=emdm->em->verts.first; eve; eve= eve->next)
			eve->prev = (EditVert*) i++;

		glBegin(GL_LINES);
		for (i=0,eed= emdm->em->edges.first; eed; i++,eed= eed->next) {
			if(!setDrawOptions || setDrawOptions(userData, i)) {
				setDrawInterpOptions(userData, i, 0.0);
				glVertex3fv(emdm->vertexCos[(int) eed->v1->prev]);
				setDrawInterpOptions(userData, i, 1.0);
				glVertex3fv(emdm->vertexCos[(int) eed->v2->prev]);
			}
		}
		glEnd();

		for (preveve=NULL, eve=emdm->em->verts.first; eve; preveve=eve, eve= eve->next)
			eve->prev = preveve;
	} else {
		glBegin(GL_LINES);
		for (i=0,eed= emdm->em->edges.first; eed; i++,eed= eed->next) {
			if(!setDrawOptions || setDrawOptions(userData, i)) {
				setDrawInterpOptions(userData, i, 0.0);
				glVertex3fv(eed->v1->co);
				setDrawInterpOptions(userData, i, 1.0);
				glVertex3fv(eed->v2->co);
			}
		}
		glEnd();
	}
}

static void emDM_drawUVEdges(DerivedMesh *dm)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditFace *efa;
	TFace *tf;

	glBegin(GL_LINES);
	for(efa= emdm->em->faces.first; efa; efa= efa->next) {
		tf = CustomData_em_get(&emdm->em->fdata, efa->data, LAYERTYPE_TFACE);

		if(tf && !(tf->flag&TF_HIDE)) {
			glVertex2fv(tf->uv[0]);
			glVertex2fv(tf->uv[1]);

			glVertex2fv(tf->uv[1]);
			glVertex2fv(tf->uv[2]);

			if (!efa->v4) {
				glVertex2fv(tf->uv[2]);
				glVertex2fv(tf->uv[0]);
			} else {
				glVertex2fv(tf->uv[2]);
				glVertex2fv(tf->uv[3]);
				glVertex2fv(tf->uv[3]);
				glVertex2fv(tf->uv[0]);
			}
		}
	}
	glEnd();
}

static void emDM__calcFaceCent(EditFace *efa, float cent[3], float (*vertexCos)[3])
{
	if (vertexCos) {
		VECCOPY(cent, vertexCos[(int) efa->v1->prev]);
		VecAddf(cent, cent, vertexCos[(int) efa->v2->prev]);
		VecAddf(cent, cent, vertexCos[(int) efa->v3->prev]);
		if (efa->v4) VecAddf(cent, cent, vertexCos[(int) efa->v4->prev]);
	} else {
		VECCOPY(cent, efa->v1->co);
		VecAddf(cent, cent, efa->v2->co);
		VecAddf(cent, cent, efa->v3->co);
		if (efa->v4) VecAddf(cent, cent, efa->v4->co);
	}

	if (efa->v4) {
		VecMulf(cent, 0.25f);
	} else {
		VecMulf(cent, 0.33333333333f);
	}
}
static void emDM_foreachMappedFaceCenter(DerivedMesh *dm, void (*func)(void *userData, int index, float *co, float *no), void *userData)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditVert *eve, *preveve;
	EditFace *efa;
	float cent[3];
	int i;

	if (emdm->vertexCos) {
		for (i=0,eve=emdm->em->verts.first; eve; eve= eve->next)
			eve->prev = (EditVert*) i++;
	}

	for(i=0,efa= emdm->em->faces.first; efa; i++,efa= efa->next) {
		emDM__calcFaceCent(efa, cent, emdm->vertexCos);
		func(userData, i, cent, emdm->vertexCos?emdm->faceNos[i]:efa->n);
	}

	if (emdm->vertexCos) {
		for (preveve=NULL, eve=emdm->em->verts.first; eve; preveve=eve, eve= eve->next)
			eve->prev = preveve;
	}
}
static void emDM_drawMappedFaces(DerivedMesh *dm, int (*setDrawOptions)(void *userData, int index, int *drawSmooth_r), void *userData, int useColors)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditFace *efa;
	int i;

	if (emdm->vertexCos) {
		EditVert *eve, *preveve;

		for (i=0,eve=emdm->em->verts.first; eve; eve= eve->next)
			eve->prev = (EditVert*) i++;

		for (i=0,efa= emdm->em->faces.first; efa; i++,efa= efa->next) {
			int drawSmooth = (efa->flag & ME_SMOOTH);
			if(!setDrawOptions || setDrawOptions(userData, i, &drawSmooth)) {
				glShadeModel(drawSmooth?GL_SMOOTH:GL_FLAT);

				glBegin(efa->v4?GL_QUADS:GL_TRIANGLES);
				if (!drawSmooth) {
					glNormal3fv(emdm->faceNos[i]);
					glVertex3fv(emdm->vertexCos[(int) efa->v1->prev]);
					glVertex3fv(emdm->vertexCos[(int) efa->v2->prev]);
					glVertex3fv(emdm->vertexCos[(int) efa->v3->prev]);
					if(efa->v4) glVertex3fv(emdm->vertexCos[(int) efa->v4->prev]);
				} else {
					glNormal3fv(emdm->vertexNos[(int) efa->v1->prev]);
					glVertex3fv(emdm->vertexCos[(int) efa->v1->prev]);
					glNormal3fv(emdm->vertexNos[(int) efa->v2->prev]);
					glVertex3fv(emdm->vertexCos[(int) efa->v2->prev]);
					glNormal3fv(emdm->vertexNos[(int) efa->v3->prev]);
					glVertex3fv(emdm->vertexCos[(int) efa->v3->prev]);
					if(efa->v4) {
						glNormal3fv(emdm->vertexNos[(int) efa->v4->prev]);
						glVertex3fv(emdm->vertexCos[(int) efa->v4->prev]);
					}
				}
				glEnd();
			}
		}

		for (preveve=NULL, eve=emdm->em->verts.first; eve; preveve=eve, eve= eve->next)
			eve->prev = preveve;
	} else {
		for (i=0,efa= emdm->em->faces.first; efa; i++,efa= efa->next) {
			int drawSmooth = (efa->flag & ME_SMOOTH);
			if(!setDrawOptions || setDrawOptions(userData, i, &drawSmooth)) {
				glShadeModel(drawSmooth?GL_SMOOTH:GL_FLAT);

				glBegin(efa->v4?GL_QUADS:GL_TRIANGLES);
				if (!drawSmooth) {
					glNormal3fv(efa->n);
					glVertex3fv(efa->v1->co);
					glVertex3fv(efa->v2->co);
					glVertex3fv(efa->v3->co);
					if(efa->v4) glVertex3fv(efa->v4->co);
				} else {
					glNormal3fv(efa->v1->no);
					glVertex3fv(efa->v1->co);
					glNormal3fv(efa->v2->no);
					glVertex3fv(efa->v2->co);
					glNormal3fv(efa->v3->no);
					glVertex3fv(efa->v3->co);
					if(efa->v4) {
						glNormal3fv(efa->v4->no);
						glVertex3fv(efa->v4->co);
					}
				}
				glEnd();
			}
		}
	}
}

static void emDM_getMinMax(DerivedMesh *dm, float min_r[3], float max_r[3])
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;
	EditVert *eve;
	int i;

	if (emdm->em->verts.first) {
		for (i=0,eve= emdm->em->verts.first; eve; i++,eve= eve->next) {
			if (emdm->vertexCos) {
				DO_MINMAX(emdm->vertexCos[i], min_r, max_r);
			} else {
				DO_MINMAX(eve->co, min_r, max_r);
			}
		}
	} else {
		min_r[0] = min_r[1] = min_r[2] = max_r[0] = max_r[1] = max_r[2] = 0.0;
	}
}
static int emDM_getNumVerts(DerivedMesh *dm)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;

	return BLI_countlist(&emdm->em->verts);
}

static int emDM_getNumEdges(DerivedMesh *dm)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;

	return BLI_countlist(&emdm->em->edges);
}

static int emDM_getNumFaces(DerivedMesh *dm)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;

	return BLI_countlist(&emdm->em->faces);
}

void emDM_getVert(DerivedMesh *dm, int index, MVert *vert_r)
{
	EditVert *ev = ((EditMeshDerivedMesh *)dm)->em->verts.first;
	int i;

	for(i = 0; i < index; ++i) ev = ev->next;

	VECCOPY(vert_r->co, ev->co);

	vert_r->no[0] = ev->no[0] * 32767.0;
	vert_r->no[1] = ev->no[1] * 32767.0;
	vert_r->no[2] = ev->no[2] * 32767.0;

	/* TODO what to do with vert_r->flag and vert_r->mat_nr? */
	vert_r->mat_nr = 0;
}

void emDM_getEdge(DerivedMesh *dm, int index, MEdge *edge_r)
{
	EditMesh *em = ((EditMeshDerivedMesh *)dm)->em;
	EditEdge *ee = em->edges.first;
	EditVert *ev, *v1, *v2;
	int i;

	for(i = 0; i < index; ++i) ee = ee->next;

	edge_r->crease = (unsigned char) (ee->crease*255.0f);
	/* TODO what to do with edge_r->flag? */
	edge_r->flag = ME_EDGEDRAW|ME_EDGERENDER;
	if (ee->seam) edge_r->flag |= ME_SEAM;
	if (ee->sharp) edge_r->flag |= ME_SHARP;
#if 0
	/* this needs setup of f2 field */
	if (!ee->f2) edge_r->flag |= ME_LOOSEEDGE;
#endif

	/* goddamn, we have to search all verts to find indices */
	v1 = ee->v1;
	v2 = ee->v2;
	for(i = 0, ev = em->verts.first; v1 || v2; i++, ev = ev->next) {
		if(ev == v1) {
			edge_r->v1 = i;
			v1 = NULL;
		}
		if(ev == v2) {
			edge_r->v2 = i;
			v2 = NULL;
		}
	}
}

void emDM_getFace(DerivedMesh *dm, int index, MFace *face_r)
{
	EditMesh *em = ((EditMeshDerivedMesh *)dm)->em;
	EditFace *ef = em->faces.first;
	EditVert *ev, *v1, *v2, *v3, *v4;
	int i;

	for(i = 0; i < index; ++i) ef = ef->next;

	face_r->mat_nr = ef->mat_nr;
	face_r->flag = ef->flag;

	/* goddamn, we have to search all verts to find indices */
	v1 = ef->v1;
	v2 = ef->v2;
	v3 = ef->v3;
	v4 = ef->v4;
	if(!v4) face_r->v4 = 0;

	for(i = 0, ev = em->verts.first; v1 || v2 || v3 || v4;
	    i++, ev = ev->next) {
		if(ev == v1) {
			face_r->v1 = i;
			v1 = NULL;
		}
		if(ev == v2) {
			face_r->v2 = i;
			v2 = NULL;
		}
		if(ev == v3) {
			face_r->v3 = i;
			v3 = NULL;
		}
		if(ev == v4) {
			face_r->v4 = i;
			v4 = NULL;
		}
	}

	test_index_face(face_r, NULL, NULL, ef->v4?4:3);
}

void emDM_getVertArray(DerivedMesh *dm, MVert *vert_r)
{
	EditVert *ev = ((EditMeshDerivedMesh *)dm)->em->verts.first;

	for( ; ev; ev = ev->next, ++vert_r) {
		VECCOPY(vert_r->co, ev->co);

		vert_r->no[0] = ev->no[0] * 32767.0;
		vert_r->no[1] = ev->no[1] * 32767.0;
		vert_r->no[2] = ev->no[2] * 32767.0;

		/* TODO what to do with vert_r->flag and vert_r->mat_nr? */
		vert_r->mat_nr = 0;
		vert_r->flag = 0;
	}
}

void emDM_getEdgeArray(DerivedMesh *dm, MEdge *edge_r)
{
	EditMesh *em = ((EditMeshDerivedMesh *)dm)->em;
	EditEdge *ee = em->edges.first;
	EditVert *ev, *prevev;
	int i;

	/* store vert indices in the prev pointer (kind of hacky) */
	for(ev = em->verts.first, i = 0; ev; ev = ev->next, ++i)
		ev->prev = (EditVert*) i++;

	for( ; ee; ee = ee->next, ++edge_r) {
		edge_r->crease = (unsigned char) (ee->crease*255.0f);
		/* TODO what to do with edge_r->flag? */
		edge_r->flag = ME_EDGEDRAW|ME_EDGERENDER;
		if (ee->seam) edge_r->flag |= ME_SEAM;
		if (ee->sharp) edge_r->flag |= ME_SHARP;
#if 0
		/* this needs setup of f2 field */
		if (!ee->f2) edge_r->flag |= ME_LOOSEEDGE;
#endif

		edge_r->v1 = (int)ee->v1->prev;
		edge_r->v2 = (int)ee->v2->prev;
	}

	/* restore prev pointers */
	for(prevev = NULL, ev = em->verts.first; ev; prevev = ev, ev = ev->next)
		ev->prev = prevev;
}

void emDM_getFaceArray(DerivedMesh *dm, MFace *face_r)
{
	EditMesh *em = ((EditMeshDerivedMesh *)dm)->em;
	EditFace *ef = em->faces.first;
	EditVert *ev, *prevev;
	int i;

	/* store vert indices in the prev pointer (kind of hacky) */
	for(ev = em->verts.first, i = 0; ev; ev = ev->next, ++i)
		ev->prev = (EditVert*) i++;

	for( ; ef; ef = ef->next, ++face_r) {
		face_r->mat_nr = ef->mat_nr;
		face_r->flag = ef->flag;

		face_r->v1 = (int)ef->v1->prev;
		face_r->v2 = (int)ef->v2->prev;
		face_r->v3 = (int)ef->v3->prev;
		if(ef->v4) face_r->v4 = (int)ef->v4->prev;
		else face_r->v4 = 0;

		test_index_face(face_r, NULL, NULL, ef->v4?4:3);
	}

	/* restore prev pointers */
	for(prevev = NULL, ev = em->verts.first; ev; prevev = ev, ev = ev->next)
		ev->prev = prevev;
}

static void emDM_release(DerivedMesh *dm)
{
	EditMeshDerivedMesh *emdm= (EditMeshDerivedMesh*) dm;

	DM_release(dm);

	if (emdm->vertexCos) {
		MEM_freeN(emdm->vertexCos);
		MEM_freeN(emdm->vertexNos);
		MEM_freeN(emdm->faceNos);
	}

	MEM_freeN(emdm);
}

static DerivedMesh *getEditMeshDerivedMesh(EditMesh *em, Object *ob,
                                           float (*vertexCos)[3])
{
	EditMeshDerivedMesh *emdm = MEM_callocN(sizeof(*emdm), "emdm");

	DM_init(&emdm->dm, BLI_countlist(&em->verts),
	                 BLI_countlist(&em->edges), BLI_countlist(&em->faces));

	emdm->dm.getMinMax = emDM_getMinMax;

	emdm->dm.getNumVerts = emDM_getNumVerts;
	emdm->dm.getNumEdges = emDM_getNumEdges;
	emdm->dm.getNumFaces = emDM_getNumFaces;

	emdm->dm.getVert = emDM_getVert;
	emdm->dm.getEdge = emDM_getEdge;
	emdm->dm.getFace = emDM_getFace;
	emdm->dm.getVertArray = emDM_getVertArray;
	emdm->dm.getEdgeArray = emDM_getEdgeArray;
	emdm->dm.getFaceArray = emDM_getFaceArray;

	emdm->dm.foreachMappedVert = emDM_foreachMappedVert;
	emdm->dm.foreachMappedEdge = emDM_foreachMappedEdge;
	emdm->dm.foreachMappedFaceCenter = emDM_foreachMappedFaceCenter;

	emdm->dm.drawEdges = emDM_drawEdges;
	emdm->dm.drawMappedEdges = emDM_drawMappedEdges;
	emdm->dm.drawMappedEdgesInterp = emDM_drawMappedEdgesInterp;
	emdm->dm.drawMappedFaces = emDM_drawMappedFaces;
	emdm->dm.drawUVEdges = emDM_drawUVEdges;

	emdm->dm.release = emDM_release;
	
	emdm->em = em;
	emdm->vertexCos = vertexCos;

	if(CustomData_has_layer(&em->vdata, LAYERTYPE_MDEFORMVERT)) {
		EditVert *eve;
		int i;

		DM_add_vert_layer(&emdm->dm, LAYERTYPE_MDEFORMVERT, 0, NULL);

		for(eve = em->verts.first, i = 0; eve; eve = eve->next, ++i)
			DM_set_vert_data(&emdm->dm, i, LAYERTYPE_MDEFORMVERT,
			                 CustomData_em_get(&em->vdata, eve->data, LAYERTYPE_MDEFORMVERT));
	}

	if(vertexCos) {
		EditVert *eve, *preveve;
		EditFace *efa;
		int totface = BLI_countlist(&em->faces);
		int i;

		for (i=0,eve=em->verts.first; eve; eve= eve->next)
			eve->prev = (EditVert*) i++;

		emdm->vertexNos = MEM_callocN(sizeof(*emdm->vertexNos)*i, "emdm_vno");
		emdm->faceNos = MEM_mallocN(sizeof(*emdm->faceNos)*totface, "emdm_vno");

		for(i=0, efa= em->faces.first; efa; i++, efa=efa->next) {
			float *v1 = vertexCos[(int) efa->v1->prev];
			float *v2 = vertexCos[(int) efa->v2->prev];
			float *v3 = vertexCos[(int) efa->v3->prev];
			float *no = emdm->faceNos[i];
			
			if(efa->v4) {
				float *v4 = vertexCos[(int) efa->v3->prev];

				CalcNormFloat4(v1, v2, v3, v4, no);
				VecAddf(emdm->vertexNos[(int) efa->v4->prev], emdm->vertexNos[(int) efa->v4->prev], no);
			}
			else {
				CalcNormFloat(v1, v2, v3, no);
			}

			VecAddf(emdm->vertexNos[(int) efa->v1->prev], emdm->vertexNos[(int) efa->v1->prev], no);
			VecAddf(emdm->vertexNos[(int) efa->v2->prev], emdm->vertexNos[(int) efa->v2->prev], no);
			VecAddf(emdm->vertexNos[(int) efa->v3->prev], emdm->vertexNos[(int) efa->v3->prev], no);
		}

		for(i=0, eve= em->verts.first; eve; i++, eve=eve->next) {
			float *no = emdm->vertexNos[i];
			/* following Mesh convention; we use vertex coordinate itself
			 * for normal in this case */
			if (Normalise(no)==0.0) {
				VECCOPY(no, vertexCos[i]);
				Normalise(no);
			}
		}

		for (preveve=NULL, eve=emdm->em->verts.first; eve; preveve=eve, eve= eve->next)
			eve->prev = preveve;
	}

	return (DerivedMesh*) emdm;
}

///

typedef struct {
	DerivedMesh dm;

	DispListMesh *dlm;
} SSDerivedMesh;

static void ssDM_foreachMappedVert(DerivedMesh *dm, void (*func)(void *userData, int index, float *co, float *no_f, short *no_s), void *userData)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	int i;
	int *index = dm->getVertDataArray(dm, LAYERTYPE_ORIGINDEX);

	for (i=0; i<dlm->totvert; i++, index++) {
		MVert *mv = &dlm->mvert[i];

		if(*index != ORIGINDEX_NONE)
			func(userData, *index, mv->co, NULL, mv->no);
	}
}
static void ssDM_foreachMappedEdge(DerivedMesh *dm, void (*func)(void *userData, int index, float *v0co, float *v1co), void *userData)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	int i;
	int *index = dm->getEdgeDataArray(dm, LAYERTYPE_ORIGINDEX);

	for (i=0; i<dlm->totedge; i++, index++) {
		MEdge *med = &dlm->medge[i];

		if(*index != ORIGINDEX_NONE)
			func(userData, *index, dlm->mvert[med->v1].co,
			     dlm->mvert[med->v2].co);
	}
}
static void ssDM_drawMappedEdges(DerivedMesh *dm, int (*setDrawOptions)(void *userData, int index), void *userData) 
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	int i;
	int *index = dm->getEdgeDataArray(dm, LAYERTYPE_ORIGINDEX);

	glBegin(GL_LINES);
	for(i=0; i<dlm->totedge; i++, index++) {
		MEdge *med = &dlm->medge[i];

		if(*index != ORIGINDEX_NONE
		   && (!setDrawOptions || setDrawOptions(userData, *index))) {
			glVertex3fv(dlm->mvert[med->v1].co);
			glVertex3fv(dlm->mvert[med->v2].co);
		}
	}
	glEnd();
}

static void ssDM_foreachMappedFaceCenter(DerivedMesh *dm, void (*func)(void *userData, int index, float *co, float *no), void *userData)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	int i;
	int *index = dm->getFaceDataArray(dm, LAYERTYPE_ORIGINDEX);

	for (i=0; i<dlm->totface; i++, index++) {
		MFace *mf = &dlm->mface[i];

		if(*index != ORIGINDEX_NONE) {
			float cent[3];
			float no[3];

			VECCOPY(cent, dlm->mvert[mf->v1].co);
			VecAddf(cent, cent, dlm->mvert[mf->v2].co);
			VecAddf(cent, cent, dlm->mvert[mf->v3].co);

			if (mf->v4) {
				CalcNormFloat4(dlm->mvert[mf->v1].co, dlm->mvert[mf->v2].co, dlm->mvert[mf->v3].co, dlm->mvert[mf->v4].co, no);
				VecAddf(cent, cent, dlm->mvert[mf->v4].co);
				VecMulf(cent, 0.25f);
			} else {
				CalcNormFloat(dlm->mvert[mf->v1].co, dlm->mvert[mf->v2].co, dlm->mvert[mf->v3].co, no);
				VecMulf(cent, 0.33333333333f);
			}

			func(userData, *index, cent, no);
		}
	}
}
static void ssDM_drawVerts(DerivedMesh *dm)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	MVert *mvert= dlm->mvert;
	int i;

	bglBegin(GL_POINTS);
	for (i=0; i<dlm->totvert; i++) {
		bglVertex3fv(mvert[i].co);
	}
	bglEnd();
}
static void ssDM_drawUVEdges(DerivedMesh *dm)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	int i;

	if (dlm->tface) {
		glBegin(GL_LINES);
		for (i=0; i<dlm->totface; i++) {
			TFace *tf = &dlm->tface[i];

			if (!(tf->flag&TF_HIDE)) {
				glVertex2fv(tf->uv[0]);
				glVertex2fv(tf->uv[1]);

				glVertex2fv(tf->uv[1]);
				glVertex2fv(tf->uv[2]);

				if (!dlm->mface[i].v4) {
					glVertex2fv(tf->uv[2]);
					glVertex2fv(tf->uv[0]);
				} else {
					glVertex2fv(tf->uv[2]);
					glVertex2fv(tf->uv[3]);

					glVertex2fv(tf->uv[3]);
					glVertex2fv(tf->uv[0]);
				}
			}
		}
		glEnd();
	}
}
static void ssDM_drawLooseEdges(DerivedMesh *dm) 
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	MVert *mvert = dlm->mvert;
	MEdge *medge= dlm->medge;
	int i;

	glBegin(GL_LINES);
	for (i=0; i<dlm->totedge; i++, medge++) {
		if (medge->flag&ME_LOOSEEDGE) {
			glVertex3fv(mvert[medge->v1].co); 
			glVertex3fv(mvert[medge->v2].co);
		}
	}
	glEnd();
}
static void ssDM_drawEdges(DerivedMesh *dm, int drawLooseEdges) 
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	MVert *mvert= dlm->mvert;
	MEdge *medge= dlm->medge;
	int i;
	
	glBegin(GL_LINES);
	for (i=0; i<dlm->totedge; i++, medge++) {
		if ((medge->flag&ME_EDGEDRAW) && (drawLooseEdges || !(medge->flag&ME_LOOSEEDGE))) {
			glVertex3fv(mvert[medge->v1].co); 
			glVertex3fv(mvert[medge->v2].co);
		}
	}
	glEnd();
}
static void ssDM_drawFacesSolid(DerivedMesh *dm, int (*setMaterial)(int))
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	float *nors = dlm->nors;
	int glmode=-1, shademodel=-1, matnr=-1, drawCurrentMat=1;
	int i;

#define PASSVERT(ind) {						\
	if (shademodel==GL_SMOOTH)				\
		glNormal3sv(dlm->mvert[(ind)].no);	\
	glVertex3fv(dlm->mvert[(ind)].co);		\
}

	glBegin(glmode=GL_QUADS);
	for (i=0; i<dlm->totface; i++) {
		MFace *mf= &dlm->mface[i];
		int new_glmode = mf->v4?GL_QUADS:GL_TRIANGLES;
		int new_shademodel = (mf->flag&ME_SMOOTH)?GL_SMOOTH:GL_FLAT;
		int new_matnr = mf->mat_nr+1;
		
		if(new_glmode!=glmode || new_shademodel!=shademodel || new_matnr!=matnr) {
			glEnd();

			drawCurrentMat = setMaterial(matnr=new_matnr);

			glShadeModel(shademodel=new_shademodel);
			glBegin(glmode=new_glmode);
		}
		
		if (drawCurrentMat) {
			if (shademodel==GL_FLAT)
				glNormal3fv(&nors[i*3]);
				
			PASSVERT(mf->v1);
			PASSVERT(mf->v2);
			PASSVERT(mf->v3);
			if (mf->v4)
				PASSVERT(mf->v4);
		}
	}
	glEnd();
	
#undef PASSVERT
}
static void ssDM_drawFacesColored(DerivedMesh *dm, int useTwoSided, unsigned char *vcols1, unsigned char *vcols2)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	int i, lmode;
	
	glShadeModel(GL_SMOOTH);
	if (vcols2) {
		glEnable(GL_CULL_FACE);
	} else {
		useTwoSided = 0;
	}
		
#define PASSVERT(vidx, fidx) {					\
	unsigned char *col= &colbase[fidx*4];		\
	glColor3ub(col[3], col[2], col[1]);			\
	glVertex3fv(dlm->mvert[(vidx)].co);			\
}

	glBegin(lmode= GL_QUADS);
	for (i=0; i<dlm->totface; i++) {
		MFace *mf= &dlm->mface[i];
		int nmode= mf->v4?GL_QUADS:GL_TRIANGLES;
		unsigned char *colbase= &vcols1[i*16];
		
		if (nmode!=lmode) {
			glEnd();
			glBegin(lmode= nmode);
		}
		
		PASSVERT(mf->v1, 0);
		PASSVERT(mf->v2, 1);
		PASSVERT(mf->v3, 2);
		if (mf->v4)
			PASSVERT(mf->v4, 3);
		
		if (useTwoSided) {
			unsigned char *colbase= &vcols2[i*16];

			if (mf->v4)
				PASSVERT(mf->v4, 3);
			PASSVERT(mf->v3, 2);
			PASSVERT(mf->v2, 1);
			PASSVERT(mf->v1, 0);
		}
	}
	glEnd();

	if (vcols2)
		glDisable(GL_CULL_FACE);
	
#undef PASSVERT
}

static void ssDM_drawFacesTex_common(DerivedMesh *dm, int (*drawParams)(TFace *tface, int matnr), int (*drawParamsMapped)(void *userData, int index), void *userData) 
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	MVert *mvert= dlm->mvert;
	MFace *mface= dlm->mface;
	TFace *tface = dlm->tface;
	float *nors = dlm->nors;
	int a;
	int *index = dm->getFaceDataArray(dm, LAYERTYPE_ORIGINDEX);
	
	for (a=0; a<dlm->totface; a++, index++) {
		MFace *mf= &mface[a];
		TFace *tf = tface?&tface[a]:NULL;
		int flag = 0;
		unsigned char *cp= NULL;
		
		if (drawParams) {
			flag = drawParams(tf, mf->mat_nr);
		}
		else {
			if(*index != ORIGINDEX_NONE)
				flag = drawParamsMapped(userData, *index);
		}

		if (flag==0) {
			continue;
		} else if (flag==1) {
			if (tf) {
				cp= (unsigned char*) tf->col;
			} else if (dlm->mcol) {
				cp= (unsigned char*) &dlm->mcol[a*4];
			}
		}

		if (!(mf->flag&ME_SMOOTH)) {
			glNormal3fv(&nors[a*3]);
		}

		glBegin(mf->v4?GL_QUADS:GL_TRIANGLES);
		if (tf) glTexCoord2fv(tf->uv[0]);
		if (cp) glColor3ub(cp[3], cp[2], cp[1]);
		if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v1].no);
		glVertex3fv((mvert+mf->v1)->co);
			
		if (tf) glTexCoord2fv(tf->uv[1]);
		if (cp) glColor3ub(cp[7], cp[6], cp[5]);
		if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v2].no);
		glVertex3fv((mvert+mf->v2)->co);

		if (tf) glTexCoord2fv(tf->uv[2]);
		if (cp) glColor3ub(cp[11], cp[10], cp[9]);
		if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v3].no);
		glVertex3fv((mvert+mf->v3)->co);

		if(mf->v4) {
			if (tf) glTexCoord2fv(tf->uv[3]);
			if (cp) glColor3ub(cp[15], cp[14], cp[13]);
			if (mf->flag&ME_SMOOTH) glNormal3sv(mvert[mf->v4].no);
			glVertex3fv((mvert+mf->v4)->co);
		}
		glEnd();
	}
}
static void ssDM_drawFacesTex(DerivedMesh *dm, int (*setDrawParams)(TFace *tface, int matnr))
{
	ssDM_drawFacesTex_common(dm, setDrawParams, NULL, NULL);
}
static void ssDM_drawMappedFacesTex(DerivedMesh *dm, int (*setDrawParams)(void *userData, int index), void *userData) 
{
	ssDM_drawFacesTex_common(dm, NULL, setDrawParams, userData);
}

static void ssDM_drawMappedFaces(DerivedMesh *dm, int (*setDrawOptions)(void *userData, int index, int *drawSmooth_r), void *userData, int useColors) 
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	DispListMesh *dlm = ssdm->dlm;
	MVert *mvert= dlm->mvert;
	MFace *mface= dlm->mface;
	float *nors = dlm->nors;
	int i;
	int *index = dm->getFaceDataArray(dm, LAYERTYPE_ORIGINDEX);

	for (i=0; i<dlm->totface; i++, index++) {
		MFace *mf = &mface[i];
		int drawSmooth = (mf->flag & ME_SMOOTH);

		if(*index != ORIGINDEX_NONE
		   && (!setDrawOptions
		       || setDrawOptions(userData, *index, &drawSmooth))) {
			unsigned char *cp = NULL;

			if (useColors) {
				if (dlm->tface) {
					cp= (unsigned char*) dlm->tface[i].col;
				} else if (dlm->mcol) {
					cp= (unsigned char*) &dlm->mcol[i*4];
				}
			}

			glShadeModel(drawSmooth?GL_SMOOTH:GL_FLAT);
			glBegin(mf->v4?GL_QUADS:GL_TRIANGLES);

			if (!drawSmooth) {
				glNormal3fv(&nors[i*3]);

				if (cp) glColor3ub(cp[3], cp[2], cp[1]);
				glVertex3fv(mvert[mf->v1].co);
				if (cp) glColor3ub(cp[7], cp[6], cp[5]);
				glVertex3fv(mvert[mf->v2].co);
				if (cp) glColor3ub(cp[11], cp[10], cp[9]);
				glVertex3fv(mvert[mf->v3].co);
				if(mf->v4) {
					if (cp) glColor3ub(cp[15], cp[14], cp[13]);
					glVertex3fv(mvert[mf->v4].co);
				}
			} else {
				if (cp) glColor3ub(cp[3], cp[2], cp[1]);
				glNormal3sv(mvert[mf->v1].no);
				glVertex3fv(mvert[mf->v1].co);
				if (cp) glColor3ub(cp[7], cp[6], cp[5]);
				glNormal3sv(mvert[mf->v2].no);
				glVertex3fv(mvert[mf->v2].co);
				if (cp) glColor3ub(cp[11], cp[10], cp[9]);
				glNormal3sv(mvert[mf->v3].no);
				glVertex3fv(mvert[mf->v3].co);
				if(mf->v4) {
					if (cp) glColor3ub(cp[15], cp[14], cp[13]);
					glNormal3sv(mvert[mf->v4].no);
					glVertex3fv(mvert[mf->v4].co);
				}
			}

			glEnd();
		}
	}
}
static void ssDM_getMinMax(DerivedMesh *dm, float min_r[3], float max_r[3])
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	int i;

	if (ssdm->dlm->totvert) {
		for (i=0; i<ssdm->dlm->totvert; i++) {
			DO_MINMAX(ssdm->dlm->mvert[i].co, min_r, max_r);
		}
	} else {
		min_r[0] = min_r[1] = min_r[2] = max_r[0] = max_r[1] = max_r[2] = 0.0;
	}
}

static void ssDM_getVertCos(DerivedMesh *dm, float (*cos_r)[3])
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;
	int i;

	for (i=0; i<ssdm->dlm->totvert; i++) {
		cos_r[i][0] = ssdm->dlm->mvert[i].co[0];
		cos_r[i][1] = ssdm->dlm->mvert[i].co[1];
		cos_r[i][2] = ssdm->dlm->mvert[i].co[2];
	}
}

static int ssDM_getNumVerts(DerivedMesh *dm)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;

	return ssdm->dlm->totvert;
}

static int ssDM_getNumEdges(DerivedMesh *dm)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;

	return ssdm->dlm->totedge;
}

static int ssDM_getNumFaces(DerivedMesh *dm)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;

	return ssdm->dlm->totface;
}

void ssDM_getVert(DerivedMesh *dm, int index, MVert *vert_r)
{
	*vert_r = ((SSDerivedMesh *)dm)->dlm->mvert[index];
}

void ssDM_getEdge(DerivedMesh *dm, int index, MEdge *edge_r)
{
	*edge_r = ((SSDerivedMesh *)dm)->dlm->medge[index];
}

void ssDM_getFace(DerivedMesh *dm, int index, MFace *face_r)
{
	*face_r = ((SSDerivedMesh *)dm)->dlm->mface[index];
}

void ssDM_getVertArray(DerivedMesh *dm, MVert *vert_r)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh *)dm;
	memcpy(vert_r, ssdm->dlm->mvert, sizeof(*vert_r) * ssdm->dlm->totvert);
}

void ssDM_getEdgeArray(DerivedMesh *dm, MEdge *edge_r)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh *)dm;
	memcpy(edge_r, ssdm->dlm->medge, sizeof(*edge_r) * ssdm->dlm->totedge);
}

void ssDM_getFaceArray(DerivedMesh *dm, MFace *face_r)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh *)dm;
	memcpy(face_r, ssdm->dlm->mface, sizeof(*face_r) * ssdm->dlm->totface);
}

static DispListMesh *ssDM_convertToDispListMesh(DerivedMesh *dm, int allowShared)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;

	if (allowShared) {
		return displistmesh_copyShared(ssdm->dlm);
	} else {
		return displistmesh_copy(ssdm->dlm);
	}
}

static void ssDM_release(DerivedMesh *dm)
{
	SSDerivedMesh *ssdm = (SSDerivedMesh*) dm;

	DM_release(dm);

	displistmesh_free(ssdm->dlm);

	MEM_freeN(dm);
}

DerivedMesh *derivedmesh_from_displistmesh(DispListMesh *dlm, float (*vertexCos)[3])
{
	SSDerivedMesh *ssdm = MEM_callocN(sizeof(*ssdm), "ssdm");

	DM_init(&ssdm->dm, dlm->totvert, dlm->totedge, dlm->totface);

	ssdm->dm.getMinMax = ssDM_getMinMax;

	ssdm->dm.getNumVerts = ssDM_getNumVerts;
	ssdm->dm.getNumEdges = ssDM_getNumEdges;
	ssdm->dm.getNumFaces = ssDM_getNumFaces;

	ssdm->dm.getVert = ssDM_getVert;
	ssdm->dm.getEdge = ssDM_getEdge;
	ssdm->dm.getFace = ssDM_getFace;
	ssdm->dm.getVertArray = ssDM_getVertArray;
	ssdm->dm.getEdgeArray = ssDM_getEdgeArray;
	ssdm->dm.getFaceArray = ssDM_getFaceArray;

	ssdm->dm.convertToDispListMesh = ssDM_convertToDispListMesh;

	ssdm->dm.getVertCos = ssDM_getVertCos;

	ssdm->dm.drawVerts = ssDM_drawVerts;

	ssdm->dm.drawUVEdges = ssDM_drawUVEdges;
	ssdm->dm.drawEdges = ssDM_drawEdges;
	ssdm->dm.drawLooseEdges = ssDM_drawLooseEdges;
	
	ssdm->dm.drawFacesSolid = ssDM_drawFacesSolid;
	ssdm->dm.drawFacesColored = ssDM_drawFacesColored;
	ssdm->dm.drawFacesTex = ssDM_drawFacesTex;
	ssdm->dm.drawMappedFaces = ssDM_drawMappedFaces;
	ssdm->dm.drawMappedFacesTex = ssDM_drawMappedFacesTex;

		/* EM functions */
	
	ssdm->dm.foreachMappedVert = ssDM_foreachMappedVert;
	ssdm->dm.foreachMappedEdge = ssDM_foreachMappedEdge;
	ssdm->dm.foreachMappedFaceCenter = ssDM_foreachMappedFaceCenter;
	
	ssdm->dm.drawMappedEdges = ssDM_drawMappedEdges;
	ssdm->dm.drawMappedEdgesInterp = NULL; // no way to implement this one
	
	ssdm->dm.release = ssDM_release;
	
	ssdm->dlm = dlm;

	if (vertexCos) {
		int i;

		for (i=0; i<dlm->totvert; i++) {
			VECCOPY(dlm->mvert[i].co, vertexCos[i]);
		}

		if (dlm->nors && !dlm->dontFreeNors) {
			MEM_freeN(dlm->nors);
			dlm->nors = 0;
		}

		mesh_calc_normals(dlm->mvert, dlm->totvert, dlm->mface, dlm->totface, &dlm->nors);
	}

	return (DerivedMesh*) ssdm;
}

#ifdef WITH_VERSE

/* verse derived mesh */
typedef struct {
	struct DerivedMesh dm;
	struct VNode *vnode;
	struct VLayer *vertex_layer;
	struct VLayer *polygon_layer;
	float (*verts)[3];
} VDerivedMesh;

/* this function set up border points of verse mesh bounding box */
static void vDM_getMinMax(DerivedMesh *dm, float min_r[3], float max_r[3])
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseVert *vvert;

	if(!vdm->vertex_layer) return;

	vvert = (VerseVert*)vdm->vertex_layer->dl.lb.first;

	if(vdm->vertex_layer->dl.da.count > 0) {
		while(vvert) {
			DO_MINMAX(vdm->verts ? vvert->cos : vvert->co, min_r, max_r);
			vvert = vvert->next;
		}
	}
	else {
		min_r[0] = min_r[1] = min_r[2] = max_r[0] = max_r[1] = max_r[2] = 0.0;
	}
}

/* this function return number of vertexes in vertex layer */
static int vDM_getNumVerts(DerivedMesh *dm)
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;

	if(!vdm->vertex_layer) return 0;
	else return vdm->vertex_layer->dl.da.count;
}

/* this function return number of 'fake' edges */
static int vDM_getNumEdges(DerivedMesh *dm)
{
	return 0;
}

/* this function returns number of polygons in polygon layer */
static int vDM_getNumFaces(DerivedMesh *dm)
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;

	if(!vdm->polygon_layer) return 0;
	else return vdm->polygon_layer->dl.da.count;
}

/* this function doesn't return vertex with index of access array,
 * but it return 'indexth' vertex of dynamic list */
void vDM_getVert(DerivedMesh *dm, int index, MVert *vert_r)
{
	VerseVert *vvert = ((VDerivedMesh*)dm)->vertex_layer->dl.lb.first;
	int i;

	for(i=0 ; i<index; i++) vvert = vvert->next;

	if(vvert) {
		VECCOPY(vert_r->co, vvert->co);

		vert_r->no[0] = vvert->no[0] * 32767.0;
		vert_r->no[1] = vvert->no[1] * 32767.0;
		vert_r->no[2] = vvert->no[2] * 32767.0;

		/* TODO what to do with vert_r->flag and vert_r->mat_nr? */
		vert_r->mat_nr = 0;
		vert_r->flag = 0;
	}
}

/* dummy function, because verse mesh doesn't store edges */
void vDM_getEdge(DerivedMesh *dm, int index, MEdge *edge_r)
{
	edge_r->flag = 0;
	edge_r->crease = 0;
	edge_r->v1 = 0;
	edge_r->v2 = 0;
}

/* this function doesn't return face with index of access array,
 * but it returns 'indexth' vertex of dynamic list */
void vDM_getFace(DerivedMesh *dm, int index, MFace *face_r)
{
	struct VerseFace *vface = ((VDerivedMesh*)dm)->polygon_layer->dl.lb.first;
	struct VerseVert *vvert = ((VDerivedMesh*)dm)->vertex_layer->dl.lb.first;
	struct VerseVert *vvert0, *vvert1, *vvert2, *vvert3;
	int i;

	for(i = 0; i < index; ++i) vface = vface->next;

	face_r->mat_nr = 0;
	face_r->flag = 0;

	/* goddamn, we have to search all verts to find indices */
	vvert0 = vface->vvert0;
	vvert1 = vface->vvert1;
	vvert2 = vface->vvert2;
	vvert3 = vface->vvert3;
	if(!vvert3) face_r->v4 = 0;

	for(i = 0; vvert0 || vvert1 || vvert2 || vvert3; i++, vvert = vvert->next) {
		if(vvert == vvert0) {
			face_r->v1 = i;
			vvert0 = NULL;
		}
		if(vvert == vvert1) {
			face_r->v2 = i;
			vvert1 = NULL;
		}
		if(vvert == vvert2) {
			face_r->v3 = i;
			vvert2 = NULL;
		}
		if(vvert == vvert3) {
			face_r->v4 = i;
			vvert3 = NULL;
		}
	}

	test_index_face(face_r, NULL, NULL, vface->vvert3?4:3);
}

/* fill array of mvert */
void vDM_getVertArray(DerivedMesh *dm, MVert *vert_r)
{
	VerseVert *vvert = ((VDerivedMesh *)dm)->vertex_layer->dl.lb.first;

	for( ; vvert; vvert = vvert->next, ++vert_r) {
		VECCOPY(vert_r->co, vvert->co);

		vert_r->no[0] = vvert->no[0] * 32767.0;
		vert_r->no[1] = vvert->no[1] * 32767.0;
		vert_r->no[2] = vvert->no[2] * 32767.0;

		vert_r->mat_nr = 0;
		vert_r->flag = 0;
	}
}

/* dummy function, edges arent supported in verse mesh */
void vDM_getEdgeArray(DerivedMesh *dm, MEdge *edge_r)
{
}

/* fill array of mfaces */
void vDM_getFaceArray(DerivedMesh *dm, MFace *face_r)
{
	VerseFace *vface = ((VDerivedMesh*)dm)->polygon_layer->dl.lb.first;
	VerseVert *vvert = ((VDerivedMesh*)dm)->vertex_layer->dl.lb.first;
	int i;

	/* store vert indices in the prev pointer (kind of hacky) */
	for(i = 0; vvert; vvert = vvert->next, ++i)
		vvert->tmp.index = i;

	for( ; vface; vface = vface->next, ++face_r) {
		face_r->mat_nr = 0;
		face_r->flag = 0;

		face_r->v1 = vface->vvert0->tmp.index;
		face_r->v2 = vface->vvert1->tmp.index;
		face_r->v3 = vface->vvert2->tmp.index;
		if(vface->vvert3) face_r->v4 = vface->vvert3->tmp.index;
		else face_r->v4 = 0;

		test_index_face(face_r, NULL, NULL, vface->vvert3?4:3);
	}
}

/* create diplist mesh from verse mesh */
static DispListMesh* vDM_convertToDispListMesh(DerivedMesh *dm, int allowShared)
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	DispListMesh *dlm = MEM_callocN(sizeof(*dlm), "dlm");
	struct VerseVert *vvert;
	struct VerseFace *vface;
	struct MVert *mvert=NULL;
	struct MFace *mface=NULL;
	float *norms;
	unsigned int i;
	EdgeHash *edges;

	if(!vdm->vertex_layer || !vdm->polygon_layer) {
		dlm->totvert = 0;
		dlm->totedge = 0;
		dlm->totface = 0;
		dlm->dontFreeVerts = dlm->dontFreeOther = dlm->dontFreeNors = 0;

		return dlm;
	};
	
	/* number of vertexes, edges and faces */
	dlm->totvert = vdm->vertex_layer->dl.da.count;
	dlm->totface = vdm->polygon_layer->dl.da.count;

	/* create dynamic array of mverts */
	mvert = (MVert*)MEM_mallocN(sizeof(MVert)*dlm->totvert, "dlm verts");
	dlm->mvert = mvert;
	vvert = (VerseVert*)vdm->vertex_layer->dl.lb.first;
	i = 0;
	while(vvert) {
		VECCOPY(mvert->co, vdm->verts ? vvert->cos : vvert->co);
		VECCOPY(mvert->no, vvert->no);
		mvert->mat_nr = 0;
		mvert->flag = 0;

		vvert->tmp.index = i++;
		mvert++;
		vvert = vvert->next;
	}

	edges = BLI_edgehash_new();

	/* create dynamic array of faces */
	mface = (MFace*)MEM_mallocN(sizeof(MFace)*dlm->totface, "dlm faces");
	dlm->mface = mface;
	vface = (VerseFace*)vdm->polygon_layer->dl.lb.first;
	i = 0;
	while(vface) {
		mface->v1 = vface->vvert0->tmp.index;
		mface->v2 = vface->vvert1->tmp.index;
		mface->v3 = vface->vvert2->tmp.index;
		if(!BLI_edgehash_haskey(edges, mface->v1, mface->v2))
			BLI_edgehash_insert(edges, mface->v1, mface->v2, NULL);
		if(!BLI_edgehash_haskey(edges, mface->v2, mface->v3))
			BLI_edgehash_insert(edges, mface->v2, mface->v3, NULL);
		if(vface->vvert3) {
			mface->v4 = vface->vvert3->tmp.index;
			if(!BLI_edgehash_haskey(edges, mface->v3, mface->v4))
				BLI_edgehash_insert(edges, mface->v3, mface->v4, NULL);
			if(!BLI_edgehash_haskey(edges, mface->v4, mface->v1))
				BLI_edgehash_insert(edges, mface->v4, mface->v1, NULL);
		} else {
			mface->v4 = 0;
			if(!BLI_edgehash_haskey(edges, mface->v3, mface->v1))
				BLI_edgehash_insert(edges, mface->v3, mface->v1, NULL);
		}

		mface->pad = 0;
		mface->mat_nr = 0;
		mface->flag = 0;
		mface->edcode = 0;

		test_index_face(mface, NULL, NULL, vface->vvert3?4:3);

		mface++;
		vface = vface->next;
	}

	dlm->totedge = BLI_edgehash_size(edges);

	if(dlm->totedge) {
		EdgeHashIterator *i;
		MEdge *medge = dlm->medge = (MEdge *)MEM_mallocN(sizeof(MEdge)*dlm->totedge, "mesh_from_verse edge");

		for(i = BLI_edgehashIterator_new(edges); !BLI_edgehashIterator_isDone(i); BLI_edgehashIterator_step(i), ++medge) {
			BLI_edgehashIterator_getKey(i, (int*)&medge->v1, (int*)&medge->v2);
			medge->crease = medge->pad = medge->flag = 0;
		}
		BLI_edgehashIterator_free(i);
	}

	BLI_edgehash_free(edges, NULL);

	/* textures and verex colors aren't supported yet */
	dlm->tface = NULL;
	dlm->mcol = NULL;

	/* faces normals */
	norms = (float*)MEM_mallocN(sizeof(float)*3*dlm->totface, "dlm norms");
	dlm->nors = norms;

	vface = (VerseFace*)vdm->polygon_layer->dl.lb.first;
	while(vface){
		VECCOPY(norms, vface->no);
		norms += 3;
		vface = vface->next;
	}

	/* free everything, nothing is shared */
	dlm->dontFreeVerts = dlm->dontFreeOther = dlm->dontFreeNors = 0;

	return dlm;
}

/* return coordination of vertex with index ... I suppose, that it will
 * be very hard to do, becuase there can be holes in access array */
static void vDM_getVertCo(DerivedMesh *dm, int index, float co_r[3])
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseVert *vvert = NULL;

	if(!vdm->vertex_layer) return;

	vvert = BLI_dlist_find_link(&(vdm->vertex_layer->dl), index);
	if(vvert) {
		VECCOPY(co_r, vdm->verts ? vvert->cos : vvert->co);
	}
	else {
		co_r[0] = co_r[1] = co_r[2] = 0.0;
	}
}

/* return array of vertex coordiantions */
static void vDM_getVertCos(DerivedMesh *dm, float (*cos_r)[3])
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseVert *vvert;
	int i = 0;

	if(!vdm->vertex_layer) return;

	vvert = vdm->vertex_layer->dl.lb.first;
	while(vvert) {
		VECCOPY(cos_r[i], vdm->verts ? vvert->cos : vvert->co);
		i++;
		vvert = vvert->next;
	}
}

/* return normal of vertex with index ... again, it will be hard to
 * implemente, because access array */
static void vDM_getVertNo(DerivedMesh *dm, int index, float no_r[3])
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseVert *vvert = NULL;

	if(!vdm->vertex_layer) return;

	vvert = BLI_dlist_find_link(&(vdm->vertex_layer->dl), index);
	if(vvert) {
		VECCOPY(no_r, vvert->no);
	}
	else {
		no_r[0] = no_r[1] = no_r[2] = 0.0;
	}
}

/* draw all VerseVertexes */
static void vDM_drawVerts(DerivedMesh *dm)
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseVert *vvert;

	if(!vdm->vertex_layer) return;

	vvert = vdm->vertex_layer->dl.lb.first;

	bglBegin(GL_POINTS);
	while(vvert) {
		bglVertex3fv(vdm->verts ? vvert->cos : vvert->co);
		vvert = vvert->next;
	}
	bglEnd();
}

/* draw all edges of VerseFaces ... it isn't optimal, because verse
 * specification doesn't support edges :-( ... bother eskil ;-)
 * ... some edges (most of edges) are drawn twice */
static void vDM_drawEdges(DerivedMesh *dm, int drawLooseEdges)
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseFace *vface;

	if(!vdm->polygon_layer) return;

	vface = vdm->polygon_layer->dl.lb.first;

	while(vface) {
		glBegin(GL_LINE_LOOP);
		glVertex3fv(vdm->verts ? vface->vvert0->cos : vface->vvert0->co);
		glVertex3fv(vdm->verts ? vface->vvert1->cos : vface->vvert1->co);
		glVertex3fv(vdm->verts ? vface->vvert2->cos : vface->vvert2->co);
		if(vface->vvert3) glVertex3fv(vdm->verts ? vface->vvert3->cos : vface->vvert3->co);
		glEnd();

		vface = vface->next;
	}
}

/* verse spec doesn't support edges ... loose edges can't exist */
void vDM_drawLooseEdges(DerivedMesh *dm)
{
}

/* draw uv edges, not supported yet */
static void vDM_drawUVEdges(DerivedMesh *dm)
{
}

/* draw all VerseFaces */
static void vDM_drawFacesSolid(DerivedMesh *dm, int (*setMaterial)(int))
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseFace *vface;

	if(!vdm->polygon_layer) return;

	vface = vdm->polygon_layer->dl.lb.first;

	while(vface) {
/*		if((vface->smooth) && (vface->smooth->value)){
			glShadeModel(GL_SMOOTH);
			glBegin(vface->vvert3?GL_QUADS:GL_TRIANGLES);
			glNormal3fv(vface->vvert0->no);
			glVertex3fv(vdm->verts ? vface->vvert0->cos : vface->vvert0->co);
			glNormal3fv(vface->vvert1->no);
			glVertex3fv(vdm->verts ? vface->vvert1->cos : vface->vvert1->co);
			glNormal3fv(vface->vvert2->no);
			glVertex3fv(vdm->verts ? vface->vvert2->cos : vface->vvert2->co);
			if(vface->vvert3){
				glNormal3fv(vface->vvert3->no);
				glVertex3fv(vdm->verts ? vface->vvert3->cos : vface->vvert3->co);
			}
			glEnd();
		}
		else { */
			glShadeModel(GL_FLAT);
			glBegin(vface->vvert3?GL_QUADS:GL_TRIANGLES);
			glNormal3fv(vface->no);
			glVertex3fv(vdm->verts ? vface->vvert0->cos : vface->vvert0->co);
			glVertex3fv(vdm->verts ? vface->vvert1->cos : vface->vvert1->co);
			glVertex3fv(vdm->verts ? vface->vvert2->cos : vface->vvert2->co);
			if(vface->vvert3)
				glVertex3fv(vdm->verts ? vface->vvert3->cos : vface->vvert3->co);
			glEnd();
/*		} */

		vface = vface->next;
	}
	glShadeModel(GL_FLAT);
}

/* thsi function should draw mesh with mapped texture, but it isn't supported yet */
static void vDM_drawFacesTex(DerivedMesh *dm, int (*setDrawOptions)(struct TFace *tface, int matnr))
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseFace *vface;

	if(!vdm->polygon_layer) return;

	vface = vdm->polygon_layer->dl.lb.first;

	while(vface) {
		glBegin(vface->vvert3?GL_QUADS:GL_TRIANGLES);
		glVertex3fv(vdm->verts ? vface->vvert0->cos : vface->vvert0->co);
		glVertex3fv(vdm->verts ? vface->vvert1->cos : vface->vvert1->co);
		glVertex3fv(vdm->verts ? vface->vvert2->cos : vface->vvert2->co);
		if(vface->vvert3)
			glVertex3fv(vdm->verts ? vface->vvert3->cos : vface->vvert3->co);
		glEnd();

		vface = vface->next;
	}
}

/* this function should draw mesh with colored faces (weight paint, vertex
 * colors, etc.), but it isn't supported yet */
static void vDM_drawFacesColored(DerivedMesh *dm, int useTwoSided, unsigned char *col1, unsigned char *col2)
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;
	struct VerseFace *vface;

	if(!vdm->polygon_layer) return;

	vface = vdm->polygon_layer->dl.lb.first;

	while(vface) {
		glBegin(vface->vvert3?GL_QUADS:GL_TRIANGLES);
		glVertex3fv(vdm->verts ? vface->vvert0->cos : vface->vvert0->co);
		glVertex3fv(vdm->verts ? vface->vvert1->cos : vface->vvert1->co);
		glVertex3fv(vdm->verts ? vface->vvert2->cos : vface->vvert2->co);
		if(vface->vvert3)
			glVertex3fv(vdm->verts ? vface->vvert3->cos : vface->vvert3->co);
		glEnd();

		vface = vface->next;
	}
}

/**/
static void vDM_foreachMappedVert(
		DerivedMesh *dm,
		void (*func)(void *userData, int index, float *co, float *no_f, short *no_s),
		void *userData)
{
}

/**/
static void vDM_foreachMappedEdge(
		DerivedMesh *dm,
		void (*func)(void *userData, int index, float *v0co, float *v1co),
		void *userData)
{
}

/**/
static void vDM_foreachMappedFaceCenter(
		DerivedMesh *dm,
		void (*func)(void *userData, int index, float *cent, float *no),
		void *userData)
{
}

/**/
static void vDM_drawMappedFacesTex(
		DerivedMesh *dm,
		int (*setDrawParams)(void *userData, int index),
		void *userData)
{
}

/**/
static void vDM_drawMappedFaces(
		DerivedMesh *dm,
		int (*setDrawOptions)(void *userData, int index, int *drawSmooth_r),
		void *userData,
		int useColors)
{
}

/**/
static void vDM_drawMappedEdges(
		DerivedMesh *dm,
		int (*setDrawOptions)(void *userData, int index),
		void *userData)
{
}

/**/
static void vDM_drawMappedEdgesInterp(
		DerivedMesh *dm, 
		int (*setDrawOptions)(void *userData, int index), 
		void (*setDrawInterpOptions)(void *userData, int index, float t),
		void *userData)
{
}

/* free all DerivedMesh data */
static void vDM_release(DerivedMesh *dm)
{
	VDerivedMesh *vdm = (VDerivedMesh*)dm;

	DM_release(dm);

	if(vdm->verts) MEM_freeN(vdm->verts);
	MEM_freeN(vdm);
}

/* create derived mesh from verse mesh ... it is used in object mode, when some other client can
 * change shared data and want to see this changes in real time too */
DerivedMesh *derivedmesh_from_versemesh(VNode *vnode, float (*vertexCos)[3])
{
	VDerivedMesh *vdm = MEM_callocN(sizeof(*vdm), "vdm");
	struct VerseVert *vvert;

	vdm->vnode = vnode;
	vdm->vertex_layer = find_verse_layer_type((VGeomData*)vnode->data, VERTEX_LAYER);
	vdm->polygon_layer = find_verse_layer_type((VGeomData*)vnode->data, POLYGON_LAYER);

	if(vdm->vertex_layer && vdm->polygon_layer)
		DM_init(&vdm->dm, vdm->vertex_layer->dl.da.count, 0, vdm->polygon_layer->dl.da.count);
	else
		DM_init(&vdm->dm, 0, 0, 0);
	
	vdm->dm.getMinMax = vDM_getMinMax;

	vdm->dm.getNumVerts = vDM_getNumVerts;
	vdm->dm.getNumEdges = vDM_getNumEdges;
	vdm->dm.getNumFaces = vDM_getNumFaces;

	vdm->dm.getVert = vDM_getVert;
	vdm->dm.getEdge = vDM_getEdge;
	vdm->dm.getFace = vDM_getFace;
	vdm->dm.getVertArray = vDM_getVertArray;
	vdm->dm.getEdgeArray = vDM_getEdgeArray;
	vdm->dm.getFaceArray = vDM_getFaceArray;
	
	vdm->dm.foreachMappedVert = vDM_foreachMappedVert;
	vdm->dm.foreachMappedEdge = vDM_foreachMappedEdge;
	vdm->dm.foreachMappedFaceCenter = vDM_foreachMappedFaceCenter;

	vdm->dm.convertToDispListMesh = vDM_convertToDispListMesh;

	vdm->dm.getVertCos = vDM_getVertCos;
	vdm->dm.getVertCo = vDM_getVertCo;
	vdm->dm.getVertNo = vDM_getVertNo;

	vdm->dm.drawVerts = vDM_drawVerts;

	vdm->dm.drawEdges = vDM_drawEdges;
	vdm->dm.drawLooseEdges = vDM_drawLooseEdges;
	vdm->dm.drawUVEdges = vDM_drawUVEdges;

	vdm->dm.drawFacesSolid = vDM_drawFacesSolid;
	vdm->dm.drawFacesTex = vDM_drawFacesTex;
	vdm->dm.drawFacesColored = vDM_drawFacesColored;

	vdm->dm.drawMappedFacesTex = vDM_drawMappedFacesTex;
	vdm->dm.drawMappedFaces = vDM_drawMappedFaces;
	vdm->dm.drawMappedEdges = vDM_drawMappedEdges;
	vdm->dm.drawMappedEdgesInterp = vDM_drawMappedEdgesInterp;

	vdm->dm.release = vDM_release;

	if(vdm->vertex_layer) {
		if(vertexCos) {
			int i;

			vdm->verts = MEM_mallocN(sizeof(float)*3*vdm->vertex_layer->dl.da.count, "verse mod vertexes");
			vvert = vdm->vertex_layer->dl.lb.first;

			for(i=0; i<vdm->vertex_layer->dl.da.count && vvert; i++, vvert = vvert->next) {
				VECCOPY(vdm->verts[i], vertexCos[i]);
				vvert->cos = vdm->verts[i];
			}
		}
		else {
			vdm->verts = NULL;
			vvert = vdm->vertex_layer->dl.lb.first;

			while(vvert) {
				vvert->cos = NULL;
				vvert = vvert->next;
			}
		}
	}

	return (DerivedMesh*) vdm;
}

#endif

/***/

DerivedMesh *mesh_create_derived_for_modifier(Object *ob, ModifierData *md)
{
	Mesh *me = ob->data;
	ModifierTypeInfo *mti = modifierType_getInfo(md->type);
	DerivedMesh *dm;

	if (!(md->mode&eModifierMode_Realtime)) return NULL;
	if (mti->isDisabled && mti->isDisabled(md)) return NULL;

	if (mti->type==eModifierTypeType_OnlyDeform) {
		int numVerts;
		float (*deformedVerts)[3] = mesh_getVertexCos(me, &numVerts);

		mti->deformVerts(md, ob, NULL, deformedVerts, numVerts);
#ifdef WITH_VERSE
		if(me->vnode) dm = derivedmesh_from_versemesh(me->vnode, deformedVerts);
		else dm = getMeshDerivedMesh(me, ob, deformedVerts);
#else
		dm = getMeshDerivedMesh(me, ob, deformedVerts);
#endif

		MEM_freeN(deformedVerts);
	} else {
		DerivedMesh *tdm = getMeshDerivedMesh(me, ob, NULL);
		dm = mti->applyModifier(md, ob, tdm, 0, 0);

		if(tdm != dm) tdm->release(tdm);
	}

	return dm;
}

static void mesh_calc_modifiers(Object *ob, float (*inputVertexCos)[3],
                                DerivedMesh **deform_r, DerivedMesh **final_r,
                                int useRenderParams, int useDeform,
                                int needMapping)
{
	Mesh *me = ob->data;
	ModifierData *md = modifiers_getVirtualModifierList(ob);
	float (*deformedVerts)[3] = NULL;
	DerivedMesh *dm;
	int numVerts = me->totvert;
	int fluidsimMeshUsed = 0;
	int required_mode;

	modifiers_clearErrors(ob);

	if(deform_r) *deform_r = NULL;
	*final_r = NULL;

	/* replace original mesh by fluidsim surface mesh for fluidsim
	 * domain objects
	 */
	if((G.obedit!=ob) && !needMapping) {
		if((ob->fluidsimFlag & OB_FLUIDSIM_ENABLE) &&
		   (1) && (!give_parteff(ob)) ) { // doesnt work together with particle systems!
			if(ob->fluidsimSettings->type & OB_FLUIDSIM_DOMAIN) {
				loadFluidsimMesh(ob,useRenderParams);
				fluidsimMeshUsed = 1;
				/* might have changed... */
				me = ob->data;
				numVerts = me->totvert;
			}
		}
	}

	if(useRenderParams) required_mode = eModifierMode_Render;
	else required_mode = eModifierMode_Realtime;

	if(useDeform) {
		if(do_ob_key(ob)) /* shape key makes deform verts */
			deformedVerts = mesh_getVertexCos(me, &numVerts);
		
		/* Apply all leading deforming modifiers */
		for(; md; md = md->next) {
			ModifierTypeInfo *mti = modifierType_getInfo(md->type);

			if((md->mode & required_mode) != required_mode) continue;
			if(mti->isDisabled && mti->isDisabled(md)) continue;

			if(mti->type == eModifierTypeType_OnlyDeform) {
				if(!deformedVerts)
					deformedVerts = mesh_getVertexCos(me, &numVerts);

				mti->deformVerts(md, ob, NULL, deformedVerts, numVerts);
			} else {
				break;
			}
		}

		/* Result of all leading deforming modifiers is cached for
		 * places that wish to use the original mesh but with deformed
		 * coordinates (vpaint, etc.)
		 */
		if (deform_r) {
#ifdef WITH_VERSE
			if(me->vnode) *deform_r = derivedmesh_from_versemesh(me->vnode, deformedVerts);
			else {
				*deform_r = CDDM_from_mesh(me);
				if(deformedVerts) {
					CDDM_apply_vert_coords(*deform_r, deformedVerts);
					CDDM_calc_normals(*deform_r);
				}
			}
#else
			*deform_r = CDDM_from_mesh(me);
			if(deformedVerts) {
				CDDM_apply_vert_coords(*deform_r, deformedVerts);
				CDDM_calc_normals(*deform_r);
			}
#endif
		}
	} else {
		if(!fluidsimMeshUsed) {
			/* default behaviour for meshes */
			deformedVerts = inputVertexCos;
		} else {
			/* the fluid sim mesh might have more vertices than the original 
			 * one, so inputVertexCos shouldnt be used
			 */
			deformedVerts = mesh_getVertexCos(me, &numVerts);
		}
	}


	/* Now apply all remaining modifiers. If useDeform is off then skip
	 * OnlyDeform ones. 
	 */
	dm = NULL;

#ifdef WITH_VERSE
	/* hack to make sure modifiers don't try to use mesh data from a verse
	 * node
	 */
	if(me->vnode) dm = derivedmesh_from_versemesh(me->vnode, deformedVerts);
#endif

	for(; md; md = md->next) {
		ModifierTypeInfo *mti = modifierType_getInfo(md->type);

		if((md->mode & required_mode) != required_mode) continue;
		if(mti->type == eModifierTypeType_OnlyDeform && !useDeform) continue;
		if((mti->flags & eModifierTypeFlag_RequiresOriginalData) && dm) {
			modifier_setError(md, "Internal error, modifier requires "
			                  "original data (bad stack position).");
			continue;
		}
		if(mti->isDisabled && mti->isDisabled(md)) continue;
		if(needMapping && !modifier_supportsMapping(md)) continue;

		/* How to apply modifier depends on (a) what we already have as
		 * a result of previous modifiers (could be a DerivedMesh or just
		 * deformed vertices) and (b) what type the modifier is.
		 */

		if(mti->type == eModifierTypeType_OnlyDeform) {
			/* No existing verts to deform, need to build them. */
			if(!deformedVerts) {
				if(dm) {
					/* Deforming a derived mesh, read the vertex locations
					 * out of the mesh and deform them. Once done with this
					 * run of deformers verts will be written back.
					 */
					numVerts = dm->getNumVerts(dm);
					deformedVerts =
					    MEM_mallocN(sizeof(*deformedVerts) * numVerts, "dfmv");
					dm->getVertCos(dm, deformedVerts);
				} else {
					deformedVerts = mesh_getVertexCos(me, &numVerts);
				}
			}

			mti->deformVerts(md, ob, dm, deformedVerts, numVerts);
		} else {
			DerivedMesh *ndm;

			/* apply vertex coordinates or build a DerivedMesh as necessary */
			if(dm) {
				if(deformedVerts) {
					DerivedMesh *tdm = CDDM_copy(dm);
					dm->release(dm);
					dm = tdm;

					CDDM_apply_vert_coords(dm, deformedVerts);
					CDDM_calc_normals(dm);
				}
			} else {
				dm = CDDM_from_mesh(me);

				if(deformedVerts) {
					CDDM_apply_vert_coords(dm, deformedVerts);
					CDDM_calc_normals(dm);
				}
			}

			ndm = mti->applyModifier(md, ob, dm, useRenderParams,
			                         !inputVertexCos);

			if(ndm) {
				/* if the modifier returned a new dm, release the old one */
				if(dm && dm != ndm) dm->release(dm);

				dm = ndm;

				if(deformedVerts) {
					if(deformedVerts != inputVertexCos)
						MEM_freeN(deformedVerts);

					deformedVerts = NULL;
				}
			} 
		}
	}

	/* Yay, we are done. If we have a DerivedMesh and deformed vertices
	 * need to apply these back onto the DerivedMesh. If we have no
	 * DerivedMesh then we need to build one.
	 */
	if(dm && deformedVerts) {
		*final_r = CDDM_copy(dm);

		dm->release(dm);

		CDDM_apply_vert_coords(*final_r, deformedVerts);
		CDDM_calc_normals(*final_r);
	} else if(dm) {
		*final_r = dm;
	} else {
#ifdef WITH_VERSE
		if(me->vnode)
			*final_r = derivedmesh_from_versemesh(me->vnode, deformedVerts);
		else {
			*final_r = CDDM_from_mesh(me);
			if(deformedVerts) {
				CDDM_apply_vert_coords(*final_r, deformedVerts);
				CDDM_calc_normals(*final_r);
			}
		}
#else
		*final_r = CDDM_from_mesh(me);
		if(deformedVerts) {
			CDDM_apply_vert_coords(*final_r, deformedVerts);
			CDDM_calc_normals(*final_r);
		}
#endif
	}

	if(deformedVerts && deformedVerts != inputVertexCos)
		MEM_freeN(deformedVerts);

	/* restore mesh in any case */
	if(fluidsimMeshUsed) ob->data = ob->fluidsimSettings->orgMesh;
}

static float (*editmesh_getVertexCos(EditMesh *em, int *numVerts_r))[3]
{
	int i, numVerts = *numVerts_r = BLI_countlist(&em->verts);
	float (*cos)[3];
	EditVert *eve;

	cos = MEM_mallocN(sizeof(*cos)*numVerts, "vertexcos");
	for (i=0,eve=em->verts.first; i<numVerts; i++,eve=eve->next) {
		VECCOPY(cos[i], eve->co);
	}

	return cos;
}

static void editmesh_calc_modifiers(DerivedMesh **cage_r, DerivedMesh **final_r)
{
	Object *ob = G.obedit;
	EditMesh *em = G.editMesh;
	ModifierData *md;
	float (*deformedVerts)[3] = NULL;
	DerivedMesh *dm;
	int i, numVerts = 0, cageIndex = modifiers_getCageIndex(ob, NULL);
	int required_mode = eModifierMode_Realtime | eModifierMode_Editmode;

	modifiers_clearErrors(ob);

	if(cage_r && cageIndex == -1) {
		*cage_r = getEditMeshDerivedMesh(em, ob, NULL);
	}

	dm = NULL;
	for(i = 0, md = ob->modifiers.first; md; i++, md = md->next) {
		ModifierTypeInfo *mti = modifierType_getInfo(md->type);

		if((md->mode & required_mode) != required_mode) continue;
		if((mti->flags & eModifierTypeFlag_RequiresOriginalData) && dm) {
			modifier_setError(md, "Internal error, modifier requires"
			                  "original data (bad stack position).");
			continue;
		}
		if(mti->isDisabled && mti->isDisabled(md)) continue;
		if(!(mti->flags & eModifierTypeFlag_SupportsEditmode)) continue;

		/* How to apply modifier depends on (a) what we already have as
		 * a result of previous modifiers (could be a DerivedMesh or just
		 * deformed vertices) and (b) what type the modifier is.
		 */

		if(mti->type == eModifierTypeType_OnlyDeform) {
			/* No existing verts to deform, need to build them. */
			if(!deformedVerts) {
				if(dm) {
					/* Deforming a derived mesh, read the vertex locations
					 * out of the mesh and deform them. Once done with this
					 * run of deformers verts will be written back.
					 */
					numVerts = dm->getNumVerts(dm);
					deformedVerts =
					    MEM_mallocN(sizeof(*deformedVerts) * numVerts, "dfmv");
					dm->getVertCos(dm, deformedVerts);
				} else {
					deformedVerts = editmesh_getVertexCos(em, &numVerts);
				}
			}

			mti->deformVertsEM(md, ob, em, dm, deformedVerts, numVerts);
		} else {
			DerivedMesh *ndm;

			/* apply vertex coordinates or build a DerivedMesh as necessary */
			if(dm) {
				if(deformedVerts) {
					DerivedMesh *tdm = CDDM_copy(dm);
					if(!(cage_r && dm == *cage_r)) dm->release(dm);
					dm = tdm;

					CDDM_apply_vert_coords(dm, deformedVerts);
					CDDM_calc_normals(dm);
				} else if(cage_r && dm == *cage_r) {
					/* dm may be changed by this modifier, so we need to copy it
					 */
					dm = CDDM_copy(dm);
				}

			} else {
				dm = CDDM_from_editmesh(em, ob->data);

				if(deformedVerts) {
					CDDM_apply_vert_coords(dm, deformedVerts);
					CDDM_calc_normals(dm);
				}
			}

			ndm = mti->applyModifierEM(md, ob, em, dm);

			if (ndm) {
				if(dm && dm != ndm)
					dm->release(dm);

				dm = ndm;

				if (deformedVerts) {
					MEM_freeN(deformedVerts);
					deformedVerts = NULL;
				}
			}
		}

		if(cage_r && i == cageIndex) {
			if(dm && deformedVerts) {
				*cage_r = CDDM_copy(dm);
				CDDM_apply_vert_coords(*cage_r, deformedVerts);
			} else if(dm) {
				*cage_r = dm;
			} else {
				*cage_r =
				    getEditMeshDerivedMesh(em, ob,
				        deformedVerts ? MEM_dupallocN(deformedVerts) : NULL);
			}
		}
	}

	/* Yay, we are done. If we have a DerivedMesh and deformed vertices need
	 * to apply these back onto the DerivedMesh. If we have no DerivedMesh
	 * then we need to build one.
	 */
	if(dm && deformedVerts) {
		*final_r = CDDM_copy(dm);

		if(!(cage_r && dm == *cage_r)) dm->release(dm);

		CDDM_apply_vert_coords(*final_r, deformedVerts);
		CDDM_calc_normals(*final_r);

		MEM_freeN(deformedVerts);
	} else if (dm) {
		*final_r = dm;
	} else {
		*final_r = getEditMeshDerivedMesh(em, ob, deformedVerts);
	}
}

/***/


	/* Something of a hack, at the moment deal with weightpaint
	 * by tucking into colors during modifier eval, only in
	 * wpaint mode. Works ok but need to make sure recalc
	 * happens on enter/exit wpaint.
	 */

void weight_to_rgb(float input, float *fr, float *fg, float *fb)
{
	float blend;
	
	blend= ((input/2.0f)+0.5f);
	
	if (input<=0.25f){	// blue->cyan
		*fr= 0.0f;
		*fg= blend*input*4.0f;
		*fb= blend;
	}
	else if (input<=0.50f){	// cyan->green
		*fr= 0.0f;
		*fg= blend;
		*fb= blend*(1.0f-((input-0.25f)*4.0f)); 
	}
	else if (input<=0.75){	// green->yellow
		*fr= blend * ((input-0.50f)*4.0f);
		*fg= blend;
		*fb= 0.0f;
	}
	else if (input<=1.0){ // yellow->red
		*fr= blend;
		*fg= blend * (1.0f-((input-0.75f)*4.0f)); 
		*fb= 0.0f;
	}
}
static void calc_weightpaint_vert_color(Object *ob, int vert, unsigned char *col)
{
	Mesh *me = ob->data;
	float fr, fg, fb, input = 0.0f;
	int i;

	if (me->dvert) {
		for (i=0; i<me->dvert[vert].totweight; i++)
			if (me->dvert[vert].dw[i].def_nr==ob->actdef-1)
				input+=me->dvert[vert].dw[i].weight;		
	}

	CLAMP(input, 0.0f, 1.0f);
	
	weight_to_rgb(input, &fr, &fg, &fb);
	
	col[3] = (unsigned char)(fr * 255.0f);
	col[2] = (unsigned char)(fg * 255.0f);
	col[1] = (unsigned char)(fb * 255.0f);
	col[0] = 255;
}
static unsigned char *calc_weightpaint_colors(Object *ob) 
{
	Mesh *me = ob->data;
	MFace *mf = me->mface;
	unsigned char *wtcol;
	int i;
	
	wtcol = MEM_callocN (sizeof (unsigned char) * me->totface*4*4, "weightmap");
	
	memset(wtcol, 0x55, sizeof (unsigned char) * me->totface*4*4);
	for (i=0; i<me->totface; i++, mf++){
		calc_weightpaint_vert_color(ob, mf->v1, &wtcol[(i*4 + 0)*4]); 
		calc_weightpaint_vert_color(ob, mf->v2, &wtcol[(i*4 + 1)*4]); 
		calc_weightpaint_vert_color(ob, mf->v3, &wtcol[(i*4 + 2)*4]); 
		if (mf->v4)
			calc_weightpaint_vert_color(ob, mf->v4, &wtcol[(i*4 + 3)*4]); 
	}
	
	return wtcol;
}

static void clear_mesh_caches(Object *ob)
{
	Mesh *me= ob->data;

		/* also serves as signal to remake texspace */
	if (me->bb) {
		MEM_freeN(me->bb);
		me->bb = NULL;
	}

	freedisplist(&ob->disp);

	if (ob->derivedFinal) {
		ob->derivedFinal->release(ob->derivedFinal);
		ob->derivedFinal= NULL;
	}
	if (ob->derivedDeform) {
		ob->derivedDeform->release(ob->derivedDeform);
		ob->derivedDeform= NULL;
	}
}

static void mesh_build_data(Object *ob)
{
	Mesh *me = ob->data;
	float min[3], max[3];

	clear_mesh_caches(ob);

	if(ob!=G.obedit) {
		Object *obact = G.scene->basact?G.scene->basact->object:NULL;
		int editing = (G.f & (G_FACESELECT|G_WEIGHTPAINT|G_VERTEXPAINT|G_TEXTUREPAINT));
		int needMapping = editing && (ob==obact);

		if( (G.f & G_WEIGHTPAINT) && ob==obact ) {
			MCol *mcol = me->mcol;
			TFace *tface =  me->tface;

			me->mcol = (MCol*) calc_weightpaint_colors(ob);
			if(me->tface) {
				me->tface = MEM_dupallocN(me->tface);
				mcol_to_tface(me, 1);
			}

			mesh_calc_modifiers(ob, NULL, &ob->derivedDeform, &ob->derivedFinal, 0, 1,
			                    needMapping);

			if(me->mcol) MEM_freeN(me->mcol);
			if(me->tface) MEM_freeN(me->tface);
			me->mcol = mcol;
			me->tface = tface;
		} else {
			mesh_calc_modifiers(ob, NULL, &ob->derivedDeform,
			                    &ob->derivedFinal, 0, 1, needMapping);
		}

		INIT_MINMAX(min, max);

		ob->derivedFinal->getMinMax(ob->derivedFinal, min, max);

		boundbox_set_from_min_max(mesh_get_bb(ob->data), min, max);
	}
}

static void editmesh_build_data(void)
{
	float min[3], max[3];

	EditMesh *em = G.editMesh;

	clear_mesh_caches(G.obedit);

	if (em->derivedFinal) {
		if (em->derivedFinal!=em->derivedCage) {
			em->derivedFinal->release(em->derivedFinal);
		}
		em->derivedFinal = NULL;
	}
	if (em->derivedCage) {
		em->derivedCage->release(em->derivedCage);
		em->derivedCage = NULL;
	}

	editmesh_calc_modifiers(&em->derivedCage, &em->derivedFinal);

	INIT_MINMAX(min, max);

	em->derivedFinal->getMinMax(em->derivedFinal, min, max);

	boundbox_set_from_min_max(mesh_get_bb(G.obedit->data), min, max);
}

void makeDispListMesh(Object *ob)
{
	if (ob==G.obedit) {
		editmesh_build_data();
	} else {
		PartEff *paf= give_parteff(ob);
		
		mesh_build_data(ob);
		
		if(paf) {
			if((paf->flag & PAF_STATIC) || (ob->recalc & OB_RECALC_TIME)==0)
				build_particle_system(ob);
		}
	}
}

/***/

DerivedMesh *mesh_get_derived_final(Object *ob, int *needsFree_r)
{
	if (!ob->derivedFinal) {
		mesh_build_data(ob);
	}

	*needsFree_r = 0;
	return ob->derivedFinal;
}

DerivedMesh *mesh_get_derived_deform(Object *ob, int *needsFree_r)
{
	if (!ob->derivedDeform) {
		mesh_build_data(ob);
	} 

	*needsFree_r = 0;
	return ob->derivedDeform;
}

DerivedMesh *mesh_create_derived_render(Object *ob)
{
	DerivedMesh *final;
	Mesh *m= get_mesh(ob);
	unsigned i;

	/* Goto the pin level for multires */
	if(m->mr) {
		m->mr->newlvl= m->mr->pinlvl;
		multires_set_level(ob,m);
	}

	mesh_calc_modifiers(ob, NULL, NULL, &final, 1, 1, 0);

	/* Propagate the changes to render level - fails if mesh topology changed */
	if(m->mr) {
		if(final->getNumVerts(final) == m->totvert &&
		   final->getNumFaces(final) == m->totface) {
			for(i=0; i<m->totvert; ++i)
				memcpy(&m->mvert[i], CustomData_get(&final->vertData, i, LAYERTYPE_MVERT), sizeof(MVert));

			final->release(final);
			
			m->mr->newlvl= m->mr->renderlvl;
			multires_set_level(ob,m);
			final= getMeshDerivedMesh(m,ob,NULL);
		}
	}	

	return final;
}

DerivedMesh *mesh_create_derived_view(Object *ob)
{
	DerivedMesh *final;

	mesh_calc_modifiers(ob, NULL, NULL, &final, 0, 1, 0);

	return final;
}

DerivedMesh *mesh_create_derived_no_deform(Object *ob, float (*vertCos)[3])
{
	DerivedMesh *final;

	mesh_calc_modifiers(ob, vertCos, NULL, &final, 0, 0, 0);

	return final;
}

DerivedMesh *mesh_create_derived_no_deform_render(Object *ob, float (*vertCos)[3])
{
	DerivedMesh *final;

	mesh_calc_modifiers(ob, vertCos, NULL, &final, 1, 0, 0);

	return final;
}

/***/

DerivedMesh *editmesh_get_derived_cage_and_final(DerivedMesh **final_r, int *cageNeedsFree_r, int *finalNeedsFree_r)
{
	*cageNeedsFree_r = *finalNeedsFree_r = 0;

	if (!G.editMesh->derivedCage)
		editmesh_build_data();

	*final_r = G.editMesh->derivedFinal;
	return G.editMesh->derivedCage;
}

DerivedMesh *editmesh_get_derived_cage(int *needsFree_r)
{
	*needsFree_r = 0;

	if (!G.editMesh->derivedCage)
		editmesh_build_data();

	return G.editMesh->derivedCage;
}

DerivedMesh *editmesh_get_derived_base(void)
{
	return getEditMeshDerivedMesh(G.editMesh, G.obedit, NULL);
}


/* ********* For those who don't grasp derived stuff! (ton) :) *************** */

static void make_vertexcosnos__mapFunc(void *userData, int index, float *co, float *no_f, short *no_s)
{
	float *vec = userData;
	
	vec+= 6*index;

	/* check if we've been here before (normal should not be 0) */
	if(vec[3] || vec[4] || vec[5]) return;

	VECCOPY(vec, co);
	vec+= 3;
	if(no_f) {
		VECCOPY(vec, no_f);
	}
	else {
		VECCOPY(vec, no_s);
	}
}

/* always returns original amount me->totvert of vertices and normals, but fully deformed and subsurfered */
/* this is needed for all code using vertexgroups (no subsurf support) */
/* it stores the normals as floats, but they can still be scaled as shorts (32767 = unit) */
/* in use now by vertex/weight paint and particle generating */

float *mesh_get_mapped_verts_nors(Object *ob)
{
	Mesh *me= ob->data;
	DerivedMesh *dm;
	float *vertexcosnos;
	int needsFree;
	
	/* lets prevent crashing... */
	if(ob->type!=OB_MESH || me->totvert==0)
		return NULL;
	
	dm= mesh_get_derived_final(ob, &needsFree);
	vertexcosnos= MEM_callocN(6*sizeof(float)*me->totvert, "vertexcosnos map");
	
	if(dm->foreachMappedVert) {
		dm->foreachMappedVert(dm, make_vertexcosnos__mapFunc, vertexcosnos);
	}
	else {
		float *fp= vertexcosnos;
		int a;
		
		for(a=0; a< me->totvert; a++, fp+=6) {
			dm->getVertCo(dm, a, fp);
			dm->getVertNo(dm, a, fp+3);
		}
	}
	
	if (needsFree) dm->release(dm);
	return vertexcosnos;
}


/* ************************* fluidsim bobj file handling **************************** */

#ifndef DISABLE_ELBEEM

#ifdef WIN32
#ifndef snprintf
#define snprintf _snprintf
#endif
#endif

/* write .bobj.gz file for a mesh object */
void writeBobjgz(char *filename, struct Object *ob, int useGlobalCoords, int append, float time) 
{
	char debugStrBuffer[256];
	int wri,i,j;
	float wrf;
	gzFile gzf;
	DispListMesh *dlm = NULL;
	DerivedMesh *dm;
	float vec[3];
	float rotmat[3][3];
	MFace *mface = NULL;
	//if(append)return; // DEBUG

	if(!ob->data || (ob->type!=OB_MESH)) {
		snprintf(debugStrBuffer,256,"Writing GZ_BOBJ Invalid object %s ...\n", ob->id.name); 
		elbeemDebugOut(debugStrBuffer);
		return;
	}
	if((ob->size[0]<0.0) || (ob->size[0]<0.0) || (ob->size[0]<0.0) ) {
		snprintf(debugStrBuffer,256,"\nfluidSim::writeBobjgz:: Warning object %s has negative scaling - check triangle ordering...?\n\n", ob->id.name); 
		elbeemDebugOut(debugStrBuffer);
	}

	snprintf(debugStrBuffer,256,"Writing GZ_BOBJ '%s' ... ",filename); elbeemDebugOut(debugStrBuffer); 
	if(append) gzf = gzopen(filename, "a+b9");
	else       gzf = gzopen(filename, "wb9");
	if (!gzf) {
		snprintf(debugStrBuffer,256,"writeBobjgz::error - Unable to open file for writing '%s'\n", filename);
		elbeemDebugOut(debugStrBuffer);
		return;
	}

	dm = mesh_create_derived_render(ob);
	//dm = mesh_create_derived_no_deform(ob,NULL);
	dlm = dm->convertToDispListMesh(dm, 1);
	mface = dlm->mface;

	// write time value for appended anim mesh
	if(append) {
		gzwrite(gzf, &time, sizeof(time));
	}

	// continue with verts/norms
	if(sizeof(wri)!=4) { snprintf(debugStrBuffer,256,"Writing GZ_BOBJ, Invalid int size %d...\n", wri); elbeemDebugOut(debugStrBuffer); return; } // paranoia check
	wri = dlm->totvert;
	gzwrite(gzf, &wri, sizeof(wri));
	for(i=0; i<wri;i++) {
		VECCOPY(vec, dlm->mvert[i].co);
		if(useGlobalCoords) { Mat4MulVecfl(ob->obmat, vec); }
		for(j=0; j<3; j++) {
			wrf = vec[j]; 
			gzwrite(gzf, &wrf, sizeof( wrf )); 
		}
	}

	// should be the same as Vertices.size
	wri = dlm->totvert;
	gzwrite(gzf, &wri, sizeof(wri));
	EulToMat3(ob->rot, rotmat);
	for(i=0; i<wri;i++) {
		VECCOPY(vec, dlm->mvert[i].no);
		Normalise(vec);
		if(useGlobalCoords) { Mat3MulVecfl(rotmat, vec); }
		for(j=0; j<3; j++) {
			wrf = vec[j];
			gzwrite(gzf, &wrf, sizeof( wrf )); 
		}
	}

	// append only writes verts&norms 
	if(!append) {
		//float side1[3],side2[3],norm1[3],norm2[3];
		//float inpf;
	
		// compute no. of triangles 
		wri = 0;
		for(i=0; i<dlm->totface; i++) {
			wri++;
			if(mface[i].v4) { wri++; }
		}
		gzwrite(gzf, &wri, sizeof(wri));
		for(i=0; i<dlm->totface; i++) {

			int face[4];
			face[0] = mface[i].v1;
			face[1] = mface[i].v2;
			face[2] = mface[i].v3;
			face[3] = mface[i].v4;
			//snprintf(debugStrBuffer,256,"F %s %d = %d,%d,%d,%d \n",ob->id.name, i, face[0],face[1],face[2],face[3] ); elbeemDebugOut(debugStrBuffer);
			//VecSubf(side1, dlm->mvert[face[1]].co,dlm->mvert[face[0]].co);
			//VecSubf(side2, dlm->mvert[face[2]].co,dlm->mvert[face[0]].co);
			//Crossf(norm1,side1,side2);
			gzwrite(gzf, &(face[0]), sizeof( face[0] )); 
			gzwrite(gzf, &(face[1]), sizeof( face[1] )); 
			gzwrite(gzf, &(face[2]), sizeof( face[2] )); 
			if(face[3]) { 
				//VecSubf(side1, dlm->mvert[face[2]].co,dlm->mvert[face[0]].co);
				//VecSubf(side2, dlm->mvert[face[3]].co,dlm->mvert[face[0]].co);
				//Crossf(norm2,side1,side2);
				//inpf = Inpf(norm1,norm2);
				//if(inpf>0.) {
				gzwrite(gzf, &(face[0]), sizeof( face[0] )); 
				gzwrite(gzf, &(face[2]), sizeof( face[2] )); 
				gzwrite(gzf, &(face[3]), sizeof( face[3] )); 
				//} else {
					//gzwrite(gzf, &(face[0]), sizeof( face[0] )); 
					//gzwrite(gzf, &(face[3]), sizeof( face[3] )); 
					//gzwrite(gzf, &(face[2]), sizeof( face[2] )); 
				//}
			} // quad
		}
	}

	snprintf(debugStrBuffer,256,"Done. #Vertices: %d, #Triangles: %d\n", dlm->totvert, dlm->totface ); 
	elbeemDebugOut(debugStrBuffer);
	
	gzclose( gzf );
	if(dlm) displistmesh_free(dlm);
	dm->release(dm);
}

void initElbeemMesh(struct Object *ob, 
		int *numVertices, float **vertices, 
		int *numTriangles, int **triangles,
		int useGlobalCoords) 
{
	DispListMesh *dlm = NULL;
	DerivedMesh *dm = NULL;
	MFace *mface = NULL;
	int countTris=0, i;
	float *verts;
	int *tris;

	dm = mesh_create_derived_render(ob);
	//dm = mesh_create_derived_no_deform(ob,NULL);
	if(!dm) { *numVertices = *numTriangles = 0; *triangles=NULL; *vertices=NULL; }
	dlm = dm->convertToDispListMesh(dm, 1);
	if(!dlm) { dm->release(dm); *numVertices = *numTriangles = 0; *triangles=NULL; *vertices=NULL; }
	mface = dlm->mface;

	*numVertices = dlm->totvert;
	verts = MEM_callocN( dlm->totvert*3*sizeof(float), "elbeemmesh_vertices");
	for(i=0; i<dlm->totvert; i++) {
		VECCOPY( &verts[i*3], dlm->mvert[i].co);
		if(useGlobalCoords) { Mat4MulVecfl(ob->obmat, &verts[i*3]); }
	}
	*vertices = verts;

	for(i=0; i<dlm->totface; i++) {
		countTris++;
		if(mface[i].v4) { countTris++; }
	}
	*numTriangles = countTris;
	tris = MEM_callocN( countTris*3*sizeof(int), "elbeemmesh_triangles");
	countTris = 0;
	for(i=0; i<dlm->totface; i++) {
		int face[4];
		face[0] = mface[i].v1;
		face[1] = mface[i].v2;
		face[2] = mface[i].v3;
		face[3] = mface[i].v4;

		tris[countTris*3+0] = face[0]; 
		tris[countTris*3+1] = face[1]; 
		tris[countTris*3+2] = face[2]; 
		countTris++;
		if(face[3]) { 
			tris[countTris*3+0] = face[0]; 
			tris[countTris*3+1] = face[2]; 
			tris[countTris*3+2] = face[3]; 
			countTris++;
		}
	}
	*triangles = tris;

	if(dlm) displistmesh_free(dlm);
	dm->release(dm);
}

/* read .bobj.gz file into a fluidsimDerivedMesh struct */
Mesh* readBobjgz(char *filename, Mesh *orgmesh, float* bbstart, float *bbsize) //, fluidsimDerivedMesh *fsdm)
{
	int wri,i,j;
	char debugStrBuffer[256];
	float wrf;
	Mesh *newmesh; 
	const int debugBobjRead = 1;
	// init data from old mesh (materials,flags)
	MFace *origMFace = &((MFace*) orgmesh->mface)[0];
	int mat_nr = -1;
	int flag = -1;
	MFace *fsface = NULL;
	int gotBytes;
	gzFile gzf;

	if(!orgmesh) return NULL;
	if(!origMFace) return NULL;
	mat_nr = origMFace->mat_nr;
	flag = origMFace->flag;

	// similar to copy_mesh
	newmesh = MEM_dupallocN(orgmesh);
	newmesh->mat= orgmesh->mat;

	newmesh->mvert= NULL;
	newmesh->medge= NULL;
	newmesh->mface= NULL;
	newmesh->tface= NULL;
	newmesh->dface= NULL;

	newmesh->dvert = NULL;

	newmesh->mcol= NULL;
	newmesh->msticky= NULL;
	newmesh->texcomesh= NULL;

	newmesh->key= NULL;
	newmesh->totface = 0;
	newmesh->totvert = 0;
	newmesh->totedge = 0;
	newmesh->medge = NULL;


	snprintf(debugStrBuffer,256,"Reading '%s' GZ_BOBJ... ",filename); elbeemDebugOut(debugStrBuffer); 
	gzf = gzopen(filename, "rb");
	// gzf = fopen(filename, "rb");
	// debug: fread(b,c,1,a) = gzread(a,b,c)
	if (!gzf) {
		//snprintf(debugStrBuffer,256,"readBobjgz::error - Unable to open file for reading '%s'\n", filename); // DEBUG
		MEM_freeN(newmesh);
		return NULL;
	}

	//if(sizeof(wri)!=4) { snprintf(debugStrBuffer,256,"Reading GZ_BOBJ, Invalid int size %d...\n", wri); return NULL; } // paranoia check
	gotBytes = gzread(gzf, &wri, sizeof(wri));
	newmesh->totvert = wri;
	newmesh->mvert = MEM_callocN(sizeof(MVert)*newmesh->totvert, "fluidsimDerivedMesh_bobjvertices");
	if(debugBobjRead){ snprintf(debugStrBuffer,256,"#vertices %d ", newmesh->totvert); elbeemDebugOut(debugStrBuffer); } //DEBUG
	for(i=0; i<newmesh->totvert;i++) {
		//if(debugBobjRead) snprintf(debugStrBuffer,256,"V %d = ",i);
		for(j=0; j<3; j++) {
			gotBytes = gzread(gzf, &wrf, sizeof( wrf )); 
			newmesh->mvert[i].co[j] = wrf;
			//if(debugBobjRead) snprintf(debugStrBuffer,256,"%25.20f ", wrf);
		}
		//if(debugBobjRead) snprintf(debugStrBuffer,256,"\n");
	}

	// should be the same as Vertices.size
	gotBytes = gzread(gzf, &wri, sizeof(wri));
	if(wri != newmesh->totvert) {
		// complain #vertices has to be equal to #normals, reset&abort
		MEM_freeN(newmesh->mvert);
		MEM_freeN(newmesh);
		snprintf(debugStrBuffer,256,"Reading GZ_BOBJ, #normals=%d, #vertices=%d, aborting...\n", wri,newmesh->totvert );
		return NULL;
	}
	for(i=0; i<newmesh->totvert;i++) {
		for(j=0; j<3; j++) {
			gotBytes = gzread(gzf, &wrf, sizeof( wrf )); 
			newmesh->mvert[i].no[j] = (short)(wrf*32767.0f);
			//newmesh->mvert[i].no[j] = 0.5; // DEBUG tst
		}
	//fprintf(stderr,"  DEBDPCN nm%d, %d = %d,%d,%d \n",
			//(int)(newmesh->mvert), i, newmesh->mvert[i].no[0], newmesh->mvert[i].no[1], newmesh->mvert[i].no[2]);
	}
	//fprintf(stderr,"  DPCN 0 = %d,%d,%d \n", newmesh->mvert[0].no[0], newmesh->mvert[0].no[1], newmesh->mvert[0].no[2]);

	
	/* compute no. of triangles */
	gotBytes = gzread(gzf, &wri, sizeof(wri));
	newmesh->totface = wri;
	newmesh->mface = MEM_callocN(sizeof(MFace)*newmesh->totface, "fluidsimDerivedMesh_bobjfaces");
	if(debugBobjRead){ snprintf(debugStrBuffer,256,"#faces %d ", newmesh->totface); elbeemDebugOut(debugStrBuffer); } //DEBUG
	fsface = newmesh->mface;
	for(i=0; i<newmesh->totface; i++) {
		int face[4];

		gotBytes = gzread(gzf, &(face[0]), sizeof( face[0] )); 
		gotBytes = gzread(gzf, &(face[1]), sizeof( face[1] )); 
		gotBytes = gzread(gzf, &(face[2]), sizeof( face[2] )); 
		face[3] = 0;

		fsface[i].v1 = face[0];
		fsface[i].v2 = face[1];
		fsface[i].v3 = face[2];
		fsface[i].v4 = face[3];
	}

	// correct triangles with v3==0 for blender, cycle verts
	for(i=0; i<newmesh->totface; i++) {
		if(!fsface[i].v3) {
			int temp = fsface[i].v1;
			fsface[i].v1 = fsface[i].v2;
			fsface[i].v2 = fsface[i].v3;
			fsface[i].v3 = temp;
		}
	}
	
	gzclose( gzf );
	for(i=0;i<newmesh->totface;i++) { 
		fsface[i].mat_nr = mat_nr;
		fsface[i].flag = flag;
		fsface[i].edcode = ME_V1V2 | ME_V2V3 | ME_V3V1;
		//snprintf(debugStrBuffer,256,"%d : %d,%d,%d\n", i,fsface[i].mat_nr, fsface[i].flag, fsface[i].edcode );
	}

	snprintf(debugStrBuffer,256," (%d,%d) done\n", newmesh->totvert,newmesh->totface); elbeemDebugOut(debugStrBuffer); //DEBUG
	return newmesh;
}

/* read zipped fluidsim velocities into the co's of the fluidsimsettings normals struct */
void readVelgz(char *filename, Object *srcob)
{
	char debugStrBuffer[256];
	int wri, i, j;
	float wrf;
	gzFile gzf;
	MVert *vverts = srcob->fluidsimSettings->meshSurfNormals;
	int len = strlen(filename);
	Mesh *mesh = srcob->data;
	// mesh and vverts have to be valid from loading...

	// clean up in any case
	for(i=0; i<mesh->totvert;i++) { 
		for(j=0; j<3; j++) {
		 	vverts[i].co[j] = 0.; 
		} 
	} 
	if(srcob->fluidsimSettings->domainNovecgen>0) return;

	if(len<7) { 
		//printf("readVelgz Eror: invalid filename '%s'\n",filename); // DEBUG
		return; 
	}

	// .bobj.gz , correct filename
	// 87654321
	filename[len-6] = 'v';
	filename[len-5] = 'e';
	filename[len-4] = 'l';

	snprintf(debugStrBuffer,256,"Reading '%s' GZ_VEL... ",filename); elbeemDebugOut(debugStrBuffer); 
	gzf = gzopen(filename, "rb");
	if (!gzf) { 
		//printf("readVelgz Eror: unable to open file '%s'\n",filename); // DEBUG
		return; 
	}

	gzread(gzf, &wri, sizeof( wri ));
	if(wri != mesh->totvert) {
		//printf("readVelgz Eror: invalid no. of velocities %d vs. %d aborting.\n" ,wri ,mesh->totvert ); // DEBUG
		return; 
	}

	for(i=0; i<mesh->totvert;i++) {
		for(j=0; j<3; j++) {
			gzread(gzf, &wrf, sizeof( wrf )); 
			vverts[i].co[j] = wrf;
		}
		//if(i<20) fprintf(stderr, "GZ_VELload %d = %f,%f,%f  \n",i,vverts[i].co[0],vverts[i].co[1],vverts[i].co[2]); // DEBUG
	}

	gzclose(gzf);
}


/* ***************************** fluidsim derived mesh ***************************** */

/* check which file to load, and replace old mesh of the object with it */
/* this replacement is undone at the end of mesh_calc_modifiers */
void loadFluidsimMesh(Object *srcob, int useRenderParams)
{
	Mesh *mesh = NULL;
	float *bbStart = NULL, *bbSize = NULL;
	float lastBB[3];
	int displaymode = 0;
	int curFrame = G.scene->r.cfra - 1 /*G.scene->r.sfra*/; /* start with 0 at start frame */
	char targetDir[FILE_MAXFILE+FILE_MAXDIR], targetFile[FILE_MAXFILE+FILE_MAXDIR];
	char debugStrBuffer[256];
	//snprintf(debugStrBuffer,256,"loadFluidsimMesh call (obid '%s', rp %d)\n", srcob->id.name, useRenderParams); // debug

	if((!srcob)||(!srcob->fluidsimSettings)) {
		snprintf(debugStrBuffer,256,"DEBUG - Invalid loadFluidsimMesh call, rp %d, dm %d)\n", useRenderParams, displaymode); // debug
		elbeemDebugOut(debugStrBuffer); // debug
		return;
	}
	// make sure the original mesh data pointer is stored
	if(!srcob->fluidsimSettings->orgMesh) {
		srcob->fluidsimSettings->orgMesh = srcob->data;
	}

	// free old mesh, if there is one (todo, check if it's still valid?)
	if(srcob->fluidsimSettings->meshSurface) {
		Mesh *freeFsMesh = srcob->fluidsimSettings->meshSurface;

		// similar to free_mesh(...) , but no things like unlink...
		if(freeFsMesh->mvert){ MEM_freeN(freeFsMesh->mvert); freeFsMesh->mvert=NULL; }
		if(freeFsMesh->medge){ MEM_freeN(freeFsMesh->medge); freeFsMesh->medge=NULL; }
		if(freeFsMesh->mface){ MEM_freeN(freeFsMesh->mface); freeFsMesh->mface=NULL; }
		MEM_freeN(freeFsMesh);
		
		if(srcob->data == srcob->fluidsimSettings->meshSurface)
		 srcob->data = srcob->fluidsimSettings->orgMesh;
		srcob->fluidsimSettings->meshSurface = NULL;

		if(srcob->fluidsimSettings->meshSurfNormals) MEM_freeN(srcob->fluidsimSettings->meshSurfNormals);
		srcob->fluidsimSettings->meshSurfNormals = NULL;
	} 

	// init bounding box
	bbStart = srcob->fluidsimSettings->bbStart; 
	bbSize = srcob->fluidsimSettings->bbSize;
	lastBB[0] = bbSize[0];  // TEST
	lastBB[1] = bbSize[1]; 
	lastBB[2] = bbSize[2];
	fluidsimGetAxisAlignedBB(srcob->fluidsimSettings->orgMesh, srcob->obmat, bbStart, bbSize, &srcob->fluidsimSettings->meshBB);
	// check free fsmesh... TODO
	
	if(!useRenderParams) {
		displaymode = srcob->fluidsimSettings->guiDisplayMode;
	} else {
		displaymode = srcob->fluidsimSettings->renderDisplayMode;
	}
	
	snprintf(debugStrBuffer,256,"loadFluidsimMesh call (obid '%s', rp %d, dm %d), curFra=%d, sFra=%d #=%d \n", 
			srcob->id.name, useRenderParams, displaymode, G.scene->r.cfra, G.scene->r.sfra, curFrame ); // debug
	elbeemDebugOut(debugStrBuffer); // debug

 	strncpy(targetDir, srcob->fluidsimSettings->surfdataPath, FILE_MAXDIR);
	// use preview or final mesh?
	if(displaymode==1) {
		// just display original object
		srcob->data = srcob->fluidsimSettings->orgMesh;
		return;
	} else if(displaymode==2) {
		strcat(targetDir,"fluidsurface_preview_#");
	} else { // 3
		strcat(targetDir,"fluidsurface_final_#");
	}
	BLI_convertstringcode(targetDir, G.sce, curFrame); // fixed #frame-no 
	strcpy(targetFile,targetDir);
	strcat(targetFile, ".bobj.gz");

	snprintf(debugStrBuffer,256,"loadFluidsimMesh call (obid '%s', rp %d, dm %d) '%s' \n", srcob->id.name, useRenderParams, displaymode, targetFile);  // debug
	elbeemDebugOut(debugStrBuffer); // debug

	if(displaymode!=2) { // dont add bounding box for final
		mesh = readBobjgz(targetFile, srcob->fluidsimSettings->orgMesh ,NULL,NULL);
	} else {
		mesh = readBobjgz(targetFile, srcob->fluidsimSettings->orgMesh, bbSize,bbSize );
	}
	if(!mesh) {
		// switch, abort background rendering when fluidsim mesh is missing
		const char *strEnvName2 = "BLENDER_ELBEEMBOBJABORT"; // from blendercall.cpp
		if(G.background==1) {
			if(getenv(strEnvName2)) {
				int elevel = atoi(getenv(strEnvName2));
				if(elevel>0) {
					printf("Env. var %s set, fluid sim mesh '%s' not found, aborting render...\n",strEnvName2, targetFile);
					exit(1);
				}
			}
		}
		
		// display org. object upon failure
		srcob->data = srcob->fluidsimSettings->orgMesh;
		return;
	}

	if((mesh)&&(mesh->totvert>0)) {
		make_edges(mesh, 0);	// 0 = make all edges draw
	}
	srcob->fluidsimSettings->meshSurface = mesh;
	srcob->data = mesh;
	srcob->fluidsimSettings->meshSurfNormals = MEM_dupallocN(mesh->mvert);

	// load vertex velocities, if they exist...
	// TODO? use generate flag as loading flag as well?
	// warning, needs original .bobj.gz mesh loading filename
	if(displaymode==3) {
		readVelgz(targetFile, srcob);
	} else {
		// no data for preview, only clear...
		int i,j;
		for(i=0; i<mesh->totvert;i++) { for(j=0; j<3; j++) { srcob->fluidsimSettings->meshSurfNormals[i].co[j] = 0.; }} 
	}

	//fprintf(stderr,"LOADFLM DEBXHCH fs=%d 3:%d,%d,%d \n", (int)mesh, ((Mesh *)(srcob->fluidsimSettings->meshSurface))->mvert[3].no[0], ((Mesh *)(srcob->fluidsimSettings->meshSurface))->mvert[3].no[1], ((Mesh *)(srcob->fluidsimSettings->meshSurface))->mvert[3].no[2]);
	return;
}

/* helper function */
/* init axis aligned BB for mesh object */
void fluidsimGetAxisAlignedBB(struct Mesh *mesh, float obmat[][4],
		 /*RET*/ float start[3], /*RET*/ float size[3], /*RET*/ struct Mesh **bbmesh )
{
	float bbsx=0.0, bbsy=0.0, bbsz=0.0;
	float bbex=1.0, bbey=1.0, bbez=1.0;
	int i;
	float vec[3];

	VECCOPY(vec, mesh->mvert[0].co); 
	Mat4MulVecfl(obmat, vec);
	bbsx = vec[0]; bbsy = vec[1]; bbsz = vec[2];
	bbex = vec[0]; bbey = vec[1]; bbez = vec[2];

	for(i=1; i<mesh->totvert;i++) {
		VECCOPY(vec, mesh->mvert[i].co);
		Mat4MulVecfl(obmat, vec);

		if(vec[0] < bbsx){ bbsx= vec[0]; }
		if(vec[1] < bbsy){ bbsy= vec[1]; }
		if(vec[2] < bbsz){ bbsz= vec[2]; }
		if(vec[0] > bbex){ bbex= vec[0]; }
		if(vec[1] > bbey){ bbey= vec[1]; }
		if(vec[2] > bbez){ bbez= vec[2]; }
	}

	// return values...
	if(start) {
		start[0] = bbsx;
		start[1] = bbsy;
		start[2] = bbsz;
	} 
	if(size) {
		size[0] = bbex-bbsx;
		size[1] = bbey-bbsy;
		size[2] = bbez-bbsz;
	}

	// init bounding box mesh?
	if(bbmesh) {
		int i,j;
		Mesh *newmesh = NULL;
		if(!(*bbmesh)) { newmesh = MEM_callocN(sizeof(Mesh), "fluidsimGetAxisAlignedBB_meshbb"); }
		else {           newmesh = *bbmesh; }

		newmesh->totvert = 8;
		if(!newmesh->mvert) newmesh->mvert = MEM_callocN(sizeof(MVert)*newmesh->totvert, "fluidsimBBMesh_bobjvertices");
		for(i=0; i<8; i++) {
			for(j=0; j<3; j++) newmesh->mvert[i].co[j] = start[j]; 
		}

		newmesh->totface = 6;
		if(!newmesh->mface) newmesh->mface = MEM_callocN(sizeof(MFace)*newmesh->totface, "fluidsimBBMesh_bobjfaces");

		*bbmesh = newmesh;
	}
}

#else // DISABLE_ELBEEM

/* dummy for mesh_calc_modifiers */
void loadFluidsimMesh(Object *srcob, int useRenderParams) {
}

#endif // DISABLE_ELBEEM

