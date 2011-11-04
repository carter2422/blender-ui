/*
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2006 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file RE_pipeline.h
 *  \ingroup render
 */

#ifndef RE_PIPELINE_H
#define RE_PIPELINE_H

#include "DNA_listBase.h"
#include "DNA_vec_types.h"

struct bNodeTree;
struct Image;
struct Main;
struct NodeBlurData;
struct Object;
struct RenderData;
struct RenderResult;
struct ReportList;
struct Scene;
struct SceneRenderLayer;
struct EnvMap;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* this include is what is exposed of render to outside world */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#define RE_MAXNAME	32

/* only used as handle */
typedef struct Render Render;

/* Render Result usage:

- render engine allocates/frees and delivers raw floating point rects
- right now it's full rects, but might become tiles or file 
- the display client has to allocate display rects, sort out what to display, 
  and how it's converted
*/

typedef struct RenderPass {
	struct RenderPass *next, *prev;
	int passtype, channels;
	char name[16];		/* amount defined in openexr_multi.h */
	char chan_id[8];	/* amount defined in openexr_multi.h */
	float *rect;
	int rectx, recty;
} RenderPass;

/* a renderlayer is a full image, but with all passes and samples */
/* size of the rects is defined in RenderResult */
/* after render, the Combined pass is in rectf, for renderlayers read from files it is a real pass */
typedef struct RenderLayer {
	struct RenderLayer *next, *prev;
	
	/* copy of RenderData */
	char name[RE_MAXNAME];		
	unsigned int lay, lay_zmask;
	int layflag, passflag, pass_xor;		
	
	struct Material *mat_override;
	struct Group *light_override;
	
	float *rectf;		/* 4 float, standard rgba buffer (read not above!) */
	float *acolrect;	/* 4 float, optional transparent buffer, needs storage for display updates */
	float *scolrect;	/* 4 float, optional strand buffer, needs storage for display updates */
	int rectx, recty;
	
	ListBase passes;
	
} RenderLayer;

typedef struct RenderResult {
	struct RenderResult *next, *prev;
	
	/* target image size */
	int rectx, recty;
	short crop, sample_nr;
	
	/* optional, 32 bits version of picture, used for ogl render and image curves */
	int *rect32;
	/* if this exists, a copy of one of layers, or result of composited layers */
	float *rectf;
	/* if this exists, a copy of one of layers, or result of composited layers */
	float *rectz;
	
	/* coordinates within final image (after cropping) */
	rcti tilerect;
	/* offset to apply to get a border render in full image */
	int xof, yof;
	
	/* the main buffers */
	ListBase layers;
	
	/* allowing live updates: */
	volatile rcti renrect;
	volatile RenderLayer *renlay;
	
	/* optional saved endresult on disk */
	void *exrhandle;
	
	/* for render results in Image, verify validity for sequences */
	int framenr;

	/* for acquire image, to indicate if it there is a combined layer */
	int have_combined;

	/* render info text */
	char *text;
	
} RenderResult;


typedef struct RenderStats {
	int cfra;
	int totface, totvert, totstrand, tothalo, totlamp, totpart;
	short curfield, curblur, curpart, partsdone, convertdone, curfsa;
	double starttime, lastframetime;
	const char *infostr, *statstr;
	char scenename[32];
	
} RenderStats;

/* *********************** API ******************** */

/* the name is used as identifier, so elsewhere in blender the result can retrieved */
/* calling a new render with same name, frees automatic existing render */
struct Render *RE_NewRender (const char *name);
struct Render *RE_GetRender(const char *name);

/* returns 1 while render is working (or renders called from within render) */
int RE_RenderInProgress(struct Render *re);

/* assign default dummy callbacks */
void RE_InitRenderCB(struct Render *re);

/* use free render as signal to do everything over (previews) */
void RE_FreeRender (struct Render *re);
/* only called on exit */
void RE_FreeAllRender (void);

/* get results and statistics */
void RE_FreeRenderResult(struct RenderResult *rr);
struct RenderResult *RE_AcquireResultRead(struct Render *re);
struct RenderResult *RE_AcquireResultWrite(struct Render *re);
void RE_ReleaseResult(struct Render *re);
void RE_AcquireResultImage(struct Render *re, struct RenderResult *rr);
void RE_ReleaseResultImage(struct Render *re);
void RE_SwapResult(struct Render *re, struct RenderResult **rr);
struct RenderStats *RE_GetStats(struct Render *re);
void RE_ResultGet32(struct Render *re, unsigned int *rect);
struct RenderLayer *RE_GetRenderLayer(struct RenderResult *rr, const char *name);
float *RE_RenderLayerGetPass(struct RenderLayer *rl, int passtype);

/* obligatory initialize call, disprect is optional */
void RE_InitState (struct Render *re, struct Render *source, struct RenderData *rd, struct SceneRenderLayer *srl, int winx, int winy, rcti *disprect);

/* use this to change disprect of active render */
void RE_SetDispRect (struct Render *re, rcti *disprect);

/* set up the viewplane/perspective matrix, three choices */
struct Object *RE_GetCamera(struct Render *re); /* return camera override if set */
void RE_SetCamera(struct Render *re, struct Object *camera);
void RE_SetWindow (struct Render *re, rctf *viewplane, float clipsta, float clipend);
void RE_SetOrtho (struct Render *re, rctf *viewplane, float clipsta, float clipend);
void RE_SetPixelSize(struct Render *re, float pixsize);

/* option to set viewmatrix before making dbase */
void RE_SetView (struct Render *re, float mat[][4]);

/* make or free the dbase */
void RE_Database_FromScene(struct Render *re, struct Main *bmain, struct Scene *scene, unsigned int lay, int use_camera_view);
void RE_Database_Free (struct Render *re);

/* project dbase again, when viewplane/perspective changed */
void RE_DataBase_ApplyWindow(struct Render *re);

/* override the scene setting for amount threads, commandline */
void RE_set_max_threads(int threads);

/* set the render threads based on the commandline and autothreads setting */
void RE_init_threadcount(Render *re);

/* the main processor, assumes all was set OK! */
void RE_TileProcessor(struct Render *re);

/* only RE_NewRender() needed, main Blender render calls */
void RE_BlenderFrame(struct Render *re, struct Main *bmain, struct Scene *scene, struct SceneRenderLayer *srl, struct Object *camera_override, unsigned int lay, int frame, const short write_still);
void RE_BlenderAnim(struct Render *re, struct Main *bmain, struct Scene *scene, struct Object *camera_override, unsigned int lay, int sfra, int efra, int tfra);

/* error reporting */
void RE_SetReports(struct Render *re, struct ReportList *reports);

/* main preview render call */
void RE_PreviewRender(struct Render *re, struct Main *bmain, struct Scene *scene);

int RE_ReadRenderResult(struct Scene *scene, struct Scene *scenode);
int RE_WriteRenderResult(struct ReportList *reports, RenderResult *rr, const char *filename, int compress);
struct RenderResult *RE_MultilayerConvert(void *exrhandle, int rectx, int recty);

extern const float default_envmap_layout[];
int RE_WriteEnvmapResult(struct ReportList *reports, struct Scene *scene, struct EnvMap *env, const char *relpath, int imtype, float layout[12]);

/* do a full sample buffer compo */
void RE_MergeFullSample(struct Render *re, struct Main *bmain, struct Scene *sce, struct bNodeTree *ntree);

/* ancient stars function... go away! */
void RE_make_stars(struct Render *re, struct Scene *scenev3d, void (*initfunc)(void),
				   void (*vertexfunc)(float*),  void (*termfunc)(void));

/* display and event callbacks */
void RE_display_init_cb	(struct Render *re, void *handle, void (*f)(void *handle, RenderResult *rr));
void RE_display_clear_cb(struct Render *re, void *handle, void (*f)(void *handle, RenderResult *rr));
void RE_display_draw_cb	(struct Render *re, void *handle, void (*f)(void *handle, RenderResult *rr, volatile struct rcti *rect));
void RE_stats_draw_cb	(struct Render *re, void *handle, void (*f)(void *handle, RenderStats *rs));
void RE_progress_cb	(struct Render *re, void *handle, void (*f)(void *handle, float));
void RE_draw_lock_cb		(struct Render *re, void *handle, void (*f)(void *handle, int));
void RE_test_break_cb	(struct Render *re, void *handle, int (*f)(void *handle));

/* should move to kernel once... still unsure on how/where */
float RE_filter_value(int type, float x);
/* vector blur zbuffer method */
void RE_zbuf_accumulate_vecblur(struct NodeBlurData *nbd, int xsize, int ysize, float *newrect, float *imgrect, float *vecbufrect, float *zbufrect);

int RE_seq_render_active(struct Scene *scene, struct RenderData *rd);

/* shaded view or baking options */
#define RE_BAKE_LIGHT				0	/* not listed in rna_scene.c -> can't be enabled! */
#define RE_BAKE_ALL					1
#define RE_BAKE_AO					2
#define RE_BAKE_NORMALS				3
#define RE_BAKE_TEXTURE				4
#define RE_BAKE_DISPLACEMENT		5
#define RE_BAKE_SHADOW				6
#define RE_BAKE_SPEC_COLOR			7
#define RE_BAKE_SPEC_INTENSITY		8
#define RE_BAKE_MIRROR_COLOR		9
#define RE_BAKE_MIRROR_INTENSITY	10
#define RE_BAKE_ALPHA				11
#define RE_BAKE_EMIT				12

void RE_Database_Baking(struct Render *re, struct Main *bmain, struct Scene *scene, unsigned int lay, const int type, struct Object *actob);

void RE_DataBase_GetView(struct Render *re, float mat[][4]);
void RE_GetCameraWindow(struct Render *re, struct Object *camera, int frame, float mat[][4]);
struct Scene *RE_GetScene(struct Render *re);

int RE_is_rendering_allowed(struct Scene *scene, struct Object *camera_override, struct ReportList *reports);

#endif /* RE_PIPELINE_H */

