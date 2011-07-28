/*
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */
#ifndef DNA_SEQUENCE_TYPES_H
#define DNA_SEQUENCE_TYPES_H
/** \file DNA_sequence_types.h
 *  \ingroup DNA
 *  \since mar-2001
 *  \author nzc
 */

#include "DNA_listBase.h"
#include "DNA_vec_types.h"

struct Ipo;
struct Scene;
struct bSound;

/* strlens; 80= FILE_MAXFILE, 160= FILE_MAXDIR */

typedef struct StripElem {
	char name[80];
	int orig_width, orig_height;
} StripElem;

typedef struct StripCrop {
	int top;
	int bottom;
	int left;
	int right;
} StripCrop;

typedef struct StripTransform {
	int xofs;
	int yofs;
} StripTransform;

typedef struct StripColorBalance {
	float lift[3];
	float gamma[3];
	float gain[3];
	int flag;
	int pad;
	// float exposure;
	// float saturation;
} StripColorBalance;

typedef struct StripProxy {
	char dir[160];
	char file[80];
	struct anim *anim;
	short size;
	short quality;
	int pad;
} StripProxy;

typedef struct Strip {
	struct Strip *next, *prev;
	int rt, len, us, done;
	int startstill, endstill;
	StripElem *stripdata;
	char dir[160];
	StripProxy *proxy;
	StripCrop *crop;
	StripTransform *transform;
	StripColorBalance *color_balance;
} Strip;


typedef struct PluginSeq {
	char name[256];
	void *handle;

	char *pname;

	int vars, version;

	void *varstr;
	float *cfra;

	float data[32];

	void *instance_private_data;
	void **current_private_data;

	void (*doit)(void);

	void (*callback)(void);
} PluginSeq;

/* The sequence structure is the basic struct used by any strip. each of the strips uses a different sequence structure.*/
/* WATCH IT: first part identical to ID (for use in ipo's) */

typedef struct Sequence {
	struct Sequence *next, *prev;
	void *tmp; /* tmp var for copying, and tagging for linked selection */
	void *lib; /* needed (to be like ipo), else it will raise libdata warnings, this should never be used */
	char name[24]; /* SEQ_NAME_MAXSTR - name, set by default and needs to be unique, for RNA paths */

	int flag, type;	/*flags bitmap (see below) and the type of sequence*/
	int len; /* the length of the contense of this strip - before handles are applied */
	int start, startofs, endofs;
	int startstill, endstill;
	int machine, depth; /*machine - the strip channel, depth - the depth in the sequence when dealing with metastrips */
	int startdisp, enddisp;	/*starting and ending points in the sequence*/
	float sat, pad;
	float mul, handsize;
					/* is sfra needed anymore? - it looks like its only used in one place */
	int sfra;		/* starting frame according to the timeline of the scene. */
	int anim_preseek;

	Strip *strip;

	struct Ipo *ipo;	// xxx depreceated... old animation system
	struct Scene *scene;
	struct Object *scene_camera; /* override scene camera */

	struct anim *anim;
	float effect_fader;
	float speed_fader;

	PluginSeq *plugin;

	/* pointers for effects: */
	struct Sequence *seq1, *seq2, *seq3;

	ListBase seqbase;	/* list of strips for metastrips */

	struct bSound *sound;	/* the linked "bSound" object */
	void *scene_sound;
	float volume;

	float pitch, pan;	/* pitch (-0.1..10), pan -2..2 */
	int scenenr;          /* for scene selection */
	int multicam_source;  /* for multicam source selection */
	float strobe;

	void *effectdata;	/* Struct pointer for effect settings */

	int anim_startofs;    /* only use part of animation file */
	int anim_endofs;      /* is subtle different to startofs / endofs */

	int blend_mode;
	float blend_opacity;

} Sequence;

typedef struct MetaStack {
	struct MetaStack *next, *prev;
	ListBase *oldbasep;
	Sequence *parseq;
} MetaStack;

typedef struct Editing {
	ListBase *seqbasep; /* pointer to the current list of seq's being edited (can be within a meta strip) */
	ListBase seqbase;	/* pointer to the top-most seq's */
	ListBase metastack;
	
	/* Context vars, used to be static */
	Sequence *act_seq;
	char act_imagedir[256];
	char act_sounddir[256];

	int over_ofs, over_cfra;
	int over_flag, pad;
	rctf over_border;
} Editing;

/* ************* Effect Variable Structs ********* */
typedef struct WipeVars {
	float edgeWidth,angle;
	short forward, wipetype;
} WipeVars;

typedef struct GlowVars {	
	float fMini;	/*	Minimum intensity to trigger a glow */
	float fClamp;
	float fBoost;	/*	Amount to multiply glow intensity */
	float dDist;	/*	Radius of glow blurring */
	int	dQuality;
	int	bNoComp;	/*	SHOW/HIDE glow buffer */
} GlowVars;

typedef struct TransformVars {
	float ScalexIni;
	float ScaleyIni;
	float ScalexFin; /* deprecated - old transform strip */
	float ScaleyFin; /* deprecated - old transform strip */
	float xIni;
	float xFin; /* deprecated - old transform strip */
	float yIni;
	float yFin; /* deprecated - old transform strip */
	float rotIni;
	float rotFin; /* deprecated - old transform strip */
	int percent;
	int interpolation;
	int uniform_scale; /* preserve aspect/ratio when scaling */
} TransformVars;

typedef struct SolidColorVars {
	float col[3];
	float pad;
} SolidColorVars;

typedef struct TitleCardVars {
	char titlestr[64];
	char subtitle[128];
	
	float fgcol[3];
	float bgcol[3];
} TitleCardVars;

typedef struct SpeedControlVars {
	float * frameMap;
	float globalSpeed;
	int flags;
	int length;
	int lastValidFrame;
} SpeedControlVars;

#define SELECT 1

/* Editor->over_flag */
#define SEQ_EDIT_OVERLAY_SHOW			1
#define SEQ_EDIT_OVERLAY_ABS			2

#define SEQ_STRIP_OFSBOTTOM		0.2f
#define SEQ_STRIP_OFSTOP		0.8f

/* SpeedControlVars->flags */
#define SEQ_SPEED_INTEGRATE      1
#define SEQ_SPEED_BLEND          2
#define SEQ_SPEED_COMPRESS_IPO_Y 4

/* ***************** SEQUENCE ****************** */
#define SEQ_NAME_MAXSTR			24

/* seq->flag */
#define SEQ_LEFTSEL                 (1<<1)
#define SEQ_RIGHTSEL                (1<<2)
#define SEQ_OVERLAP                 (1<<3)
#define SEQ_FILTERY                 (1<<4)
#define SEQ_MUTE                    (1<<5)
#define SEQ_MAKE_PREMUL             (1<<6)
#define SEQ_REVERSE_FRAMES          (1<<7)
#define SEQ_IPO_FRAME_LOCKED        (1<<8)
#define SEQ_EFFECT_NOT_LOADED       (1<<9)
#define SEQ_FLAG_DELETE             (1<<10)
#define SEQ_FLIPX                   (1<<11)
#define SEQ_FLIPY                   (1<<12)
#define SEQ_MAKE_FLOAT              (1<<13)
#define SEQ_LOCK                    (1<<14)
#define SEQ_USE_PROXY               (1<<15)
#define SEQ_USE_TRANSFORM           (1<<16)
#define SEQ_USE_CROP                (1<<17)
#define SEQ_USE_COLOR_BALANCE       (1<<18)
#define SEQ_USE_PROXY_CUSTOM_DIR    (1<<19)

#define SEQ_USE_PROXY_CUSTOM_FILE   (1<<21)
#define SEQ_USE_EFFECT_DEFAULT_FADE (1<<22)

// flags for whether those properties are animated or not
#define SEQ_AUDIO_VOLUME_ANIMATED   (1<<24)
#define SEQ_AUDIO_PITCH_ANIMATED    (1<<25)
#define SEQ_AUDIO_PAN_ANIMATED      (1<<26)

#define SEQ_INVALID_EFFECT          (1<<31)

/* convenience define for all selection flags */
#define SEQ_ALLSEL	(SELECT+SEQ_LEFTSEL+SEQ_RIGHTSEL)

/* deprecated, dont use a flag anymore*/
/*#define SEQ_ACTIVE                            1048576*/

#define SEQ_COLOR_BALANCE_INVERSE_GAIN 1
#define SEQ_COLOR_BALANCE_INVERSE_GAMMA 2
#define SEQ_COLOR_BALANCE_INVERSE_LIFT 4

/* seq->type WATCH IT: SEQ_EFFECT BIT is used to determine if this is an effect strip!!! */
#define SEQ_IMAGE		0
#define SEQ_META		1
#define SEQ_SCENE		2
#define SEQ_MOVIE		3
#define SEQ_RAM_SOUND		4
#define SEQ_HD_SOUND            5
#define SEQ_SOUND		4

#define SEQ_EFFECT		8
#define SEQ_CROSS		8
#define SEQ_ADD			9
#define SEQ_SUB			10
#define SEQ_ALPHAOVER	11
#define SEQ_ALPHAUNDER	12
#define SEQ_GAMCROSS	13
#define SEQ_MUL			14
#define SEQ_OVERDROP	15
#define SEQ_PLUGIN		24
#define SEQ_WIPE		25
#define SEQ_GLOW		26
#define SEQ_TRANSFORM		27
#define SEQ_COLOR               28
#define SEQ_SPEED               29
#define SEQ_MULTICAM            30
#define SEQ_ADJUSTMENT          31
#define SEQ_TITLECARD			40
#define SEQ_EFFECT_MAX          40

#define STRIPELEM_FAILED       0
#define STRIPELEM_OK           1

#define STRIPELEM_PREVIEW_DONE  1

#define SEQ_BLEND_REPLACE      0
/* all other BLEND_MODEs are simple SEQ_EFFECT ids and therefore identical
   to the table above. (Only those effects that handle _exactly_ two inputs,
   otherwise, you can't really blend, right :) !)
*/


#define SEQ_HAS_PATH(_seq) (ELEM5((_seq)->type, SEQ_MOVIE, SEQ_IMAGE, SEQ_SOUND, SEQ_RAM_SOUND, SEQ_HD_SOUND))

#endif

