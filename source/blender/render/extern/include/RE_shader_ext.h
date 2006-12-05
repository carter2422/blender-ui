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
 * The Original Code is Copyright (C) 2006 by Blender Foundation
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#ifndef RE_SHADER_EXT_H
#define RE_SHADER_EXT_H

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* this include is for shading and texture exports            */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* localized texture result data */
/* note; tr tg tb ta has to remain in this order */
typedef struct TexResult {
	float tin, tr, tg, tb, ta;
	int talpha;
	float *nor;
} TexResult;

/* localized shade result data */
typedef struct ShadeResult 
{
	float combined[4];
	float col[4];
	float alpha;
	float diff[3];		/* includes ramps, shadow, etc */
	float diff_raw[3];	/* pure diffuse, no shadow no ramps */
	float spec[3];
	float shad[3];
	float ao[3];
	float refl[3];
	float refr[3];
	float nor[3];
	float winspeed[4];
} ShadeResult;

/* only here for quick copy */
struct ShadeInputCopy {
	
	struct Material *mat;
	struct VlakRen *vlr;
	int facenr;
	float facenor[3];				/* copy from face */
	struct VertRen *v1, *v2, *v3;	/* vertices can be in any order for quads... */
	short i1, i2, i3;				/* original vertex indices */
	short puno;
	float vn[3], vno[3];			/* actual render normal, and a copy to restore it */
	float n1[3], n2[3], n3[3];		/* vertex normals, corrected */
};

/* localized renderloop data */
typedef struct ShadeInput
{
	/* copy from face, also to extract tria from quad */
	/* note it mirrors a struct above for quick copy */
	
	struct Material *mat;
	struct VlakRen *vlr;
	int facenr;
	float facenor[3];				/* copy from face */
	struct VertRen *v1, *v2, *v3;	/* vertices can be in any order for quads... */
	short i1, i2, i3;				/* original vertex indices */
	short puno;
	float vn[3], vno[3];			/* actual render normal, and a copy to restore it */
	float n1[3], n2[3], n3[3];		/* vertex normals, corrected */
	
	/* internal face coordinates */
	float u, v, dx_u, dx_v, dy_u, dy_v;
	float co[3], view[3];
	
	/* copy from material, keep synced so we can do memcopy */
	/* current size: 23*4 */
	float r, g, b;
	float specr, specg, specb;
	float mirr, mirg, mirb;
	float ambr, ambb, ambg;
	
	float amb, emit, ang, spectra, ray_mirror;
	float alpha, refl, spec, zoffs, add;
	float translucency;
	/* end direct copy from material */
	
	/* individual copies: */
	int har;
	float layerfac;
	
	/* base material mode (OR-ed result of entire node tree) */
	int mode;
	
	/* texture coordinates */
	float lo[3], gl[3], uv[3], ref[3], orn[3], winco[3], sticky[3], vcol[3], rad[3];
	float refcol[4], displace[3];
	float strand, tang[3], stress, winspeed[4];
	
	/* dx/dy OSA coordinates */
	float dxco[3], dyco[3];
	float dxlo[3], dylo[3], dxgl[3], dygl[3], dxuv[3], dyuv[3];
	float dxref[3], dyref[3], dxorn[3], dyorn[3];
	float dxno[3], dyno[3], dxview, dyview;
	float dxlv[3], dylv[3];
	float dxwin[3], dywin[3];
	float dxsticky[3], dysticky[3];
	float dxrefract[3], dyrefract[3];
	float dxstrand, dystrand;
	
	/* AO is a pre-process now */
	float ao[3];
	
	int xs, ys;				/* pixel to be rendered */
	short osatex;
	int mask;
	int depth;				/* 1 or larger on raytrace shading */
	
	/* from initialize, part or renderlayer */
	short do_preview;		/* for nodes, in previewrender */
	short thread, sample;	/* sample: ShadeSample array index */
	unsigned int lay;
	int layflag, passflag;
	
} ShadeInput;


/* node shaders... */
struct Tex;
int	multitex_ext(struct Tex *tex, float *texvec, float *dxt, float *dyt, int osatex, struct TexResult *texres);

/* shaded view and bake */
struct Render;
struct Image;

void RE_shade_external(struct Render *re, struct ShadeInput *shi, struct ShadeResult *shr);
int RE_bake_shade_all_selected(struct Render *re, int type);
struct Image *RE_bake_shade_get_image(void);

#endif /* RE_SHADER_EXT_H */

