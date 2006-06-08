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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 * 
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

#ifndef BIF_RESOURCES_H
#define BIF_RESOURCES_H

/* elubie: TODO: move the typedef for icons to BIF_interface_icons.h */
/* and add/replace include of BIF_resources.h by BIF_interface_icons.h */
typedef enum {
#define BIFICONID_FIRST		(ICON_VIEW3D)
	ICON_VIEW3D,
	ICON_IPO,
	ICON_OOPS,
	ICON_BUTS,
	ICON_FILESEL,
	ICON_IMAGE_COL,
	ICON_INFO,
	ICON_SEQUENCE,
	ICON_TEXT,
	ICON_IMASEL,
	ICON_SOUND,
	ICON_ACTION,
	ICON_NLA,
	ICON_SCRIPTWIN,
	ICON_TIME,
	ICON_NODE,
	ICON_SPACE2,
	ICON_SPACE3,
	ICON_SPACE4,
	ICON_TRIA_LEFT,
	ICON_TRIA_UP,

	ICON_ORTHO,
	ICON_PERSP,
	ICON_CAMERA,
	ICON_EFFECTS,
	ICON_BBOX,
	ICON_WIRE,
	ICON_SOLID,
	ICON_SMOOTH,
	ICON_POTATO,
	ICON_MARKER_HLT,
	ICON_NORMALVIEW,
	ICON_LOCALVIEW,
	ICON_UNUSEDVIEW,
	ICON_VIEWZOOM,
	ICON_SORTALPHA,
	ICON_SORTTIME,
	ICON_SORTSIZE,
	ICON_LONGDISPLAY,
	ICON_SHORTDISPLAY,
	ICON_TRIA_DOWN,
	ICON_TRIA_RIGHT,

	ICON_VIEW_AXIS_ALL,
	ICON_VIEW_AXIS_NONE,
	ICON_VIEW_AXIS_NONE2,
	ICON_VIEW_AXIS_TOP,
	ICON_VIEW_AXIS_FRONT,
	ICON_VIEW_AXIS_SIDE,
	ICON_POSE_DEHLT,
	ICON_POSE_HLT,
	ICON_BORDERMOVE,
	ICON_MAYBE_ITS_A_LASSO,
	ICON_BLANK1,	/* ATTENTION, someone decided to use this throughout blender
	                   and didn't care to neither rename it nor update the PNG */
	ICON_VERSE,
	ICON_MOD_BOOLEAN,
	ICON_ARMATURE,
	ICON_PAUSE,
	ICON_ALIGN,
	ICON_REC,
	ICON_PLAY,
	ICON_FF,
	ICON_REW,
	ICON_PYTHON,

	
	ICON_DOTSUP,
	ICON_DOTSDOWN,
	ICON_MENU_PANEL,
	ICON_AXIS_SIDE,
	ICON_AXIS_FRONT,
	ICON_AXIS_TOP,
	ICON_DRAW_UVFACES,
	ICON_STICKY_UVS,
	ICON_STICKY2_UVS,
	ICON_PREV_KEYFRAME,
	ICON_NEXT_KEYFRAME,
	ICON_ENVMAP,
	ICON_TRANSP_HLT,
	ICON_TRANSP_DEHLT,
	ICON_CIRCLE_DEHLT,
	ICON_CIRCLE_HLT,
	ICON_TPAINT_DEHLT,
	ICON_TPAINT_HLT,
	ICON_WPAINT_DEHLT,
	ICON_WPAINT_HLT,
	ICON_MARKER,
	
	ICON_X,
	ICON_GO_LEFT,
	ICON_NO_GO_LEFT,
	ICON_UNLOCKED,
	ICON_LOCKED,
	ICON_PARLIB,
	ICON_DATALIB,
	ICON_AUTO,
	ICON_MATERIAL_DEHLT2,
	ICON_RING,
	ICON_GRID,
	ICON_PROPEDIT,
	ICON_KEEPRECT,
	ICON_DESEL_CUBE_VERTS,
	ICON_EDITMODE_DEHLT,
	ICON_EDITMODE_HLT,
	ICON_VPAINT_DEHLT,
	ICON_VPAINT_HLT,
	ICON_FACESEL_DEHLT,
	ICON_FACESEL_HLT,
	ICON_EDIT_DEHLT,
	
	ICON_HELP,
	ICON_ERROR,
	ICON_FOLDER_DEHLT,
	ICON_FOLDER_HLT,
	ICON_BLUEIMAGE_DEHLT,
	ICON_BLUEIMAGE_HLT,
	ICON_BPIBFOLDER_DEHLT,
	ICON_BPIBFOLDER_HLT,
	ICON_BPIBFOLDER_ERR,
	ICON_UGLY_GREEN_RING,
	ICON_GHOST,
	ICON_SORTBYEXT,
	ICON_BLANK33,
	ICON_VERTEXSEL,
	ICON_EDGESEL,
	ICON_FACESEL,
	ICON_BLANK26,
	ICON_BPIBFOLDER_X,
	ICON_BPIBFOLDERGREY,
	ICON_MAGNIFY,
	ICON_INFO2,
	
	ICON_RIGHTARROW,
	ICON_DOWNARROW_HLT,
	ICON_ROUNDBEVELTHING,
	ICON_FULLTEXTURE,
	ICON_HOOK,
	ICON_DOT,
	ICON_WORLD_DEHLT,
	ICON_CHECKBOX_DEHLT,
	ICON_CHECKBOX_HLT,
	ICON_LINK,
	ICON_INLINK,
	ICON_ZOOMIN,
	ICON_ZOOMOUT,
	ICON_PASTEDOWN,
	ICON_COPYDOWN,
	ICON_CONSTANT,
	ICON_LINEAR,
	ICON_CYCLIC,
	ICON_KEY_DEHLT,
	ICON_KEY_HLT,
	ICON_GRID2,
	
	ICON_EYE,
	ICON_LAMP,
	ICON_MATERIAL,
	ICON_TEXTURE,
	ICON_ANIM,
	ICON_WORLD,
	ICON_SCENE,
	ICON_EDIT,
	ICON_GAME,
	ICON_PAINT,
	ICON_RADIO,
	ICON_SCRIPT,
	ICON_SPEAKER,
	ICON_PASTEUP,
	ICON_COPYUP,
	ICON_PASTEFLIPUP,
	ICON_PASTEFLIPDOWN,
	ICON_CYCLICLINEAR,
	ICON_PIN_DEHLT,
	ICON_PIN_HLT,
	ICON_LITTLEGRID,
	
	ICON_FULLSCREEN,
	ICON_SPLITSCREEN,
	ICON_RIGHTARROW_THIN,
	ICON_DISCLOSURE_TRI_RIGHT,
	ICON_DISCLOSURE_TRI_DOWN,
	ICON_SCENE_SEPIA,
	ICON_SCENE_DEHLT,
	ICON_OBJECT,
	ICON_MESH,
	ICON_CURVE,
	ICON_MBALL,
	ICON_LATTICE,
	ICON_LAMP_DEHLT,
	ICON_MATERIAL_DEHLT,
	ICON_TEXTURE_DEHLT,
	ICON_IPO_DEHLT,
	ICON_LIBRARY_DEHLT,
	ICON_IMAGE_DEHLT,
	ICON_WINDOW_FULLSCREEN,
	ICON_WINDOW_WINDOW,
	ICON_PANEL_CLOSE,
	
	ICON_BLENDER,
	ICON_PACKAGE,
	ICON_UGLYPACKAGE,
	ICON_MATPLANE,
	ICON_MATSPHERE,
	ICON_MATCUBE,
	ICON_SCENE_HLT,
	ICON_OBJECT_HLT,
	ICON_MESH_HLT,
	ICON_CURVE_HLT,
	ICON_MBALL_HLT,
	ICON_LATTICE_HLT,
	ICON_LAMP_HLT,
	ICON_MATERIAL_HLT,
	ICON_TEXTURE_HLT,
	ICON_IPO_HLT,
	ICON_LIBRARY_HLT,
	ICON_IMAGE_HLT,
	ICON_CONSTRAINT,
	ICON_CAMERA_DEHLT,
	ICON_ARMATURE_DEHLT,
	
	ICON_SMOOTHCURVE,
	ICON_SPHERECURVE,
	ICON_ROOTCURVE,
	ICON_SHARPCURVE,
	ICON_LINCURVE,
	ICON_NOCURVE,
	ICON_PROP_OFF,
	ICON_PROP_ON,
	ICON_PROP_CON,
	ICON_SYNTAX,
	ICON_SYNTAX_OFF,
	ICON_BLANK52,
	ICON_BLANK53,
	ICON_PLUS,
	ICON_VIEWMOVE,
	ICON_HOME,
	ICON_CLIPUV_DEHLT,
	ICON_CLIPUV_HLT,
	ICON_SOME_WACKY_VERTS_AND_LINES,
	ICON_A_WACKY_VERT_AND_SOME_LINES,
	ICON_VPAINT_COL,
	
	ICON_MAN_TRANS,
	ICON_MAN_ROT,
	ICON_MAN_SCALE,
	ICON_MANIPUL,
	ICON_BLANK65,
	ICON_MODIFIER,
	ICON_MOD_WAVE,
	ICON_MOD_BUILD,
	ICON_MOD_DECIM,
	ICON_MOD_MIRROR,
	ICON_MOD_SOFT,
	ICON_MOD_SUBSURF,
	ICON_SEQ_SEQUENCER,
	ICON_SEQ_PREVIEW,
	ICON_SEQ_LUMA_WAVEFORM,
	ICON_SEQ_CHROMA_SCOPE,
	ICON_ROTATE,
	ICON_CURSOR,
	ICON_ROTATECOLLECTION,
	ICON_ROTATECENTER,
	ICON_ROTACTIVE,

	VICON_VIEW3D,
	VICON_EDIT,
	VICON_EDITMODE_DEHLT,
	VICON_EDITMODE_HLT,
	VICON_DISCLOSURE_TRI_RIGHT,
	VICON_DISCLOSURE_TRI_DOWN,
	VICON_MOVE_UP,
	VICON_MOVE_DOWN,
	VICON_X

#define BIFICONID_LAST		(VICON_X)
#define BIFNICONIDS			(BIFICONID_LAST-BIFICONID_FIRST + 1)
} BIFIconID;

typedef enum {
#define BIFCOLORSHADE_FIRST     (COLORSHADE_DARK)
        COLORSHADE_DARK,
        COLORSHADE_GREY,
        COLORSHADE_MEDIUM,
        COLORSHADE_HILITE,
        COLORSHADE_LIGHT,
        COLORSHADE_WHITE
#define BIFCOLORSHADE_LAST      (COLORSHADE_WHITE)
#define BIFNCOLORSHADES         (BIFCOLORSHADE_LAST-BIFCOLORSHADE_FIRST + 1)
} BIFColorShade;

typedef enum {
#define BIFCOLORID_FIRST	(BUTGREY)
	BUTGREY = 0,
	BUTGREEN,
	BUTBLUE,
	BUTSALMON,
	MIDGREY,
	BUTPURPLE,
	BUTYELLOW,
	REDALERT,
	BUTRUST,
	BUTWHITE,
	BUTDBLUE,
	BUTPINK,
	BUTDPINK,
	BUTMACTIVE,

	BUTIPO,
	BUTAUDIO,
	BUTCAMERA,
	BUTRANDOM,
	BUTEDITOBJECT,
	BUTPROPERTY,
	BUTSCENE,
	BUTMOTION,
	BUTMESSAGE,
	BUTACTION,
	BUTCD,
	BUTGAME,
	BUTVISIBILITY,
	BUTYUCK,
	BUTSEASICK,
	BUTCHOKE,
	BUTIMPERIAL,

	BUTTEXTCOLOR,
	BUTTEXTPRESSED,
	BUTSBACKGROUND,
	
	VIEWPORTBACKCOLOR,
	VIEWPORTGRIDCOLOR,
	VIEWPORTACTIVECOLOR,
	VIEWPORTSELECTEDCOLOR,
	VIEWPORTUNSELCOLOR,
	
	EDITVERTSEL, 
	EDITVERTUNSEL, 
	EDITEDGESEL, 
	EDITEDGEUNSEL
	
#define BIFCOLORID_LAST		(EDITEDGEUNSEL)
#define BIFNCOLORIDS		(BIFCOLORID_LAST-BIFCOLORID_FIRST + 1)

} BIFColorID;

/* XXX WARNING: this is saved in file, so do not change order! */
enum {
	TH_AUTO,	/* for buttons, to signal automatic color assignment */
	
// uibutton colors
	TH_BUT_OUTLINE,
	TH_BUT_NEUTRAL,
	TH_BUT_ACTION,
	TH_BUT_SETTING,
	TH_BUT_SETTING1,
	TH_BUT_SETTING2,
	TH_BUT_NUM,
	TH_BUT_TEXTFIELD,
	TH_BUT_POPUP,
	TH_BUT_TEXT,
	TH_BUT_TEXT_HI,
	TH_MENU_BACK,
	TH_MENU_ITEM,
	TH_MENU_HILITE,
	TH_MENU_TEXT,
	TH_MENU_TEXT_HI,
	
	TH_BUT_DRAWTYPE,
	
	TH_REDALERT,
	TH_CUSTOM,
	
	TH_BUT_TEXTFIELD_HI,
	
	TH_THEMEUI,
// common colors among spaces
	
	TH_BACK,
	TH_TEXT,
	TH_TEXT_HI,
	TH_HEADER,
	TH_HEADERDESEL,
	TH_PANEL,
	TH_SHADE1,
	TH_SHADE2,
	TH_HILITE,

	TH_GRID,
	TH_WIRE,
	TH_SELECT,
	TH_ACTIVE,
	TH_GROUP,
	TH_GROUP_ACTIVE,
	TH_TRANSFORM,
	TH_VERTEX,
	TH_VERTEX_SELECT,
	TH_VERTEX_SIZE,
	TH_EDGE,
	TH_EDGE_SELECT,
	TH_EDGE_SEAM,
	TH_EDGE_FACESEL,
	TH_FACE,
	TH_FACE_SELECT,
	TH_NORMAL,
	TH_FACE_DOT,
	TH_FACEDOT_SIZE,

	TH_SYNTAX_B,
	TH_SYNTAX_V,
	TH_SYNTAX_C,
	TH_SYNTAX_L,
	TH_SYNTAX_N,
	
	TH_BONE_SOLID,
	TH_BONE_POSE,
	
	TH_STRIP,
	TH_STRIP_SELECT,
	
	TH_LAMP,
	
	TH_NODE,
	TH_NODE_IN_OUT,
	TH_NODE_OPERATOR,
	TH_NODE_GENERATOR,
	TH_NODE_GROUP,
	
	TH_SEQ_MOVIE,
	TH_SEQ_IMAGE,
	TH_SEQ_SCENE,
	TH_SEQ_AUDIO,
	TH_SEQ_EFFECT,
	TH_SEQ_PLUGIN,
	TH_SEQ_TRANSITION,
	TH_SEQ_META,
	
};
/* XXX WARNING: previous is saved in file, so do not change order! */


/* specific defines per space should have higher define values */

struct bTheme;

// THE CODERS API FOR THEMES:

// sets the color
void 	BIF_ThemeColor(int colorid);

// sets the color plus alpha
void 	BIF_ThemeColor4(int colorid);

// sets color plus offset for shade
void 	BIF_ThemeColorShade(int colorid, int offset);

// sets color plus offset for alpha
void	BIF_ThemeColorShadeAlpha(int colorid, int coloffset, int alphaoffset);

// sets color, which is blend between two theme colors
void 	BIF_ThemeColorBlend(int colorid1, int colorid2, float fac);
// same, with shade offset
void    BIF_ThemeColorBlendShade(int colorid1, int colorid2, float fac, int offset);

// returns one value, not scaled
float 	BIF_GetThemeValuef(int colorid);
int 	BIF_GetThemeValue(int colorid);

// get three color values, scaled to 0.0-1.0 range
void 	BIF_GetThemeColor3fv(int colorid, float *col);

// get the 3 or 4 byte values
void 	BIF_GetThemeColor3ubv(int colorid, char *col);
void 	BIF_GetThemeColor4ubv(int colorid, char *col);

// get a theme color from specified space type
void	BIF_GetThemeColorType4ubv(int colorid, int spacetype, char *col);

// blends and shades between two color pointers
void	BIF_ColorPtrBlendShade3ubv(char *cp1, char *cp2, float fac, int offset);

// get a 3 byte color, blended and shaded between two other char color pointers
void	BIF_GetColorPtrBlendShade3ubv(char *cp1, char *cp2, char *col, float fac, int offset);


struct ScrArea;

// internal (blender) usage only, for init and set active
void 	BIF_InitTheme(void);
void 	BIF_SetTheme(struct ScrArea *sa);
void	BIF_resources_init		(void);
void	BIF_resources_free		(void);
void	BIF_colors_init			(void);
void	BIF_load_ui_colors		(void);

/* only for buttons in theme editor! */
char 	*BIF_ThemeGetColorPtr(struct bTheme *btheme, int spacetype, int colorid);
char 	*BIF_ThemeColorsPup(int spacetype);


void	BIF_def_color			(BIFColorID colorid, unsigned char r, unsigned char g, unsigned char b);

#endif /*  BIF_ICONS_H */
