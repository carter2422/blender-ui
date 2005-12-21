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
 * Drawing routines for the Action window type
 */

/* System includes ----------------------------------------------------- */

#include <math.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MEM_guardedalloc.h"

#include "BMF_Api.h"

#include "BLI_blenlib.h"
#include "BLI_arithb.h"

/* Types --------------------------------------------------------------- */
#include "DNA_action_types.h"
#include "DNA_armature_types.h"
#include "DNA_curve_types.h"
#include "DNA_ipo_types.h"
#include "DNA_object_types.h"
#include "DNA_screen_types.h"
#include "DNA_scene_types.h"
#include "DNA_space_types.h"
#include "DNA_constraint_types.h"
#include "DNA_key_types.h"

#include "BKE_action.h"
#include "BKE_ipo.h"
#include "BKE_global.h"

/* Everything from source (BIF, BDR, BSE) ------------------------------ */ 

#include "BIF_editaction.h"
#include "BIF_editkey.h"
#include "BIF_interface.h"
#include "BIF_interface_icons.h"
#include "BIF_gl.h"
#include "BIF_glutil.h"
#include "BIF_resources.h"
#include "BIF_screen.h"
#include "BIF_mywindow.h"
#include "BIF_space.h"

#include "BDR_drawaction.h"
#include "BDR_editcurve.h"

#include "BSE_view.h"
#include "BSE_drawnla.h"
#include "BSE_drawipo.h"

/* 'old' stuff": defines and types, and own include -------------------- */

#include "blendef.h"
#include "mydevice.h"


/* local functions ----------------------------------------------------- */
int count_action_levels(bAction *act);
static BezTriple **ipo_to_keylist(Ipo *ipo, int flags, int *totvert);
static BezTriple **action_to_keylist(bAction *act, int flags, int *totvert);
static BezTriple **ob_to_keylist(Object *ob, int flags, int *totvert);
static BezTriple **icu_to_keylist(IpoCurve *icu, int flags, int *totvert);
void draw_icu_channel(gla2DDrawInfo *di, IpoCurve *icu, int flags, float ypos);

/* implementation ------------------------------------------------------ */
extern short showsliders; /* editaction .c */
extern short ACTWIDTH;

static void meshactionbuts(SpaceAction *saction, Object *ob, Key *key)
{
	int           i;
	char          str[64];
	float	        x, y;
	uiBlock       *block;
	uiBut 		  *but;

#define XIC 20
#define YIC 20

	/* lets make the rvk sliders */

	/* reset the damn myortho2 or the sliders won't draw/redraw
	 * correctly *grumble*
	 */
	mywinset(curarea->win);
	myortho2(-0.375, curarea->winx-0.375, -0.375, curarea->winy-0.375);

    sprintf(str, "actionbuttonswin %d", curarea->win);
    block= uiNewBlock (&curarea->uiblocks, str, 
                       UI_EMBOSS, UI_HELV, curarea->win);

	x = NAMEWIDTH + 1;
    y = key->totkey*(CHANNELHEIGHT+CHANNELSKIP) 
	  + CHANNELHEIGHT/2  - G.v2d->cur.ymin;

	/* make the little 'open the sliders' widget */
    BIF_ThemeColor(TH_FACE); // this slot was open...
	glRects(2,            y + 2*CHANNELHEIGHT - 2,  
			ACTWIDTH - 2, y + CHANNELHEIGHT + 2);
	glColor3ub(0, 0, 0);
	glRasterPos2f(4, y + CHANNELHEIGHT + 6);
	BMF_DrawString(G.font, "Sliders");

	uiBlockSetEmboss(block, UI_EMBOSSN);

	if (!showsliders) {
		ACTWIDTH = NAMEWIDTH;
		but=uiDefIconButS(block, TOG, B_REDR, 
					  ICON_DISCLOSURE_TRI_RIGHT,
					  NAMEWIDTH - XIC - 5, y + CHANNELHEIGHT,
					  XIC,YIC-2,
					  &(showsliders), 0, 0, 0, 0, 
					  "Show action window sliders");
		// no hilite, the winmatrix is not correct later on...
		uiButSetFlag(but, UI_NO_HILITE);

	}
	else {
		but= uiDefIconButS(block, TOG, B_REDR, 
					  ICON_DISCLOSURE_TRI_DOWN,
					  NAMEWIDTH - XIC - 5, y + CHANNELHEIGHT,
					  XIC,YIC-2,
					  &(showsliders), 0, 0, 0, 0, 
					  "Hide action window sliders");
		// no hilite, the winmatrix is not correct later on...
		uiButSetFlag(but, UI_NO_HILITE);
					  
		ACTWIDTH = NAMEWIDTH + SLIDERWIDTH;

		/* sliders are open so draw them */
		BIF_ThemeColor(TH_FACE); 

		glRects(NAMEWIDTH,  0,  NAMEWIDTH+SLIDERWIDTH,  curarea->winy);
		uiBlockSetEmboss(block, UI_EMBOSS);
		for (i=1 ; i < key->totkey ; ++ i) {
			make_rvk_slider(block, ob, i, 
							x, y, SLIDERWIDTH-2, CHANNELHEIGHT-1, "Slider to control Shape Keys");

			y-=CHANNELHEIGHT+CHANNELSKIP;
			
		}
	}
	uiDrawBlock(block);

}

void draw_cfra_action(void)
{
	Object *ob;
	float vec[2];
	
	vec[0]= (G.scene->r.cfra);
	vec[0]*= G.scene->r.framelen;
	
	vec[1]= G.v2d->cur.ymin;
	glColor3ub(0x60, 0xc0, 0x40);
	glLineWidth(2.0);
	
	glBegin(GL_LINE_STRIP);
	glVertex2fv(vec);
	vec[1]= G.v2d->cur.ymax;
	glVertex2fv(vec);
	glEnd();
	
	ob= (G.scene->basact) ? (G.scene->basact->object) : 0;
	if(ob && ob->sf!=0.0 && (ob->ipoflag & OB_OFFS_OB) ) {
		vec[0]-= ob->sf;
		
		glColor3ub(0x10, 0x60, 0);
		
		glBegin(GL_LINE_STRIP);
		glVertex2fv(vec);
		vec[1]= G.v2d->cur.ymin;
		glVertex2fv(vec);
		glEnd();
	}
	
	glLineWidth(1.0);
}

/* left hand */
static void draw_action_channel_names(bAction	*act) 
{
    bActionChannel *chan;
    bConstraintChannel *conchan;
    float	x, y;

    x = 0.0;
	y= count_action_levels(act)*(CHANNELHEIGHT+CHANNELSKIP);

	for (chan=act->chanbase.first; chan; chan=chan->next){
		if((chan->flag & ACHAN_HIDDEN)==0) {
			BIF_ThemeColorShade(TH_HEADER, 20);
			glRectf(x,  y-CHANNELHEIGHT/2,  (float)NAMEWIDTH,  y+CHANNELHEIGHT/2);
		
			if (chan->flag & ACHAN_SELECTED)
				BIF_ThemeColor(TH_TEXT_HI);
			else
				BIF_ThemeColor(TH_TEXT);
			glRasterPos2f(x+8,  y-4);
			BMF_DrawString(G.font, chan->name);
			y-=CHANNELHEIGHT+CHANNELSKIP;
		
			/* Draw constraint channels */
			for (conchan=chan->constraintChannels.first; 
					conchan; conchan=conchan->next){
				if (conchan->flag & CONSTRAINT_CHANNEL_SELECT)
					BIF_ThemeColor(TH_TEXT_HI);
				else
					BIF_ThemeColor(TH_TEXT);
					
				glRasterPos2f(x+32,  y-4);
				BMF_DrawString(G.font, conchan->name);
				y-=CHANNELHEIGHT+CHANNELSKIP;
			}
		}
	}
}


static void draw_action_mesh_names(Key *key) 
{
	/* draws the names of the rvk keys in the
	 * left side of the action window
	 */
	int	     i;
	char     keyname[32];
	float    x, y;
	KeyBlock *kb;

	x = 0.0;
	y= key->totkey*(CHANNELHEIGHT+CHANNELSKIP);

	kb= key->block.first;

	for (i=1 ; i < key->totkey ; ++ i) {
		glColor3ub(0xAA, 0xAA, 0xAA);
		glRectf(x,	y-CHANNELHEIGHT/2,	(float)NAMEWIDTH,  y+CHANNELHEIGHT/2);

		glColor3ub(0, 0, 0);

		glRasterPos2f(x+8,	y-4);
		kb = kb->next;
		/* Blender now has support for named
		 * key blocks. If a name hasn't
		 * been set for an key block then
		 * just display the key number -- 
		 * otherwise display the name stored
		 * in the keyblock.
		 */
		if (kb->name[0] == '\0') {
		  sprintf(keyname, "Key %d", i);
		  BMF_DrawString(G.font, keyname);
		}
		else {
		  BMF_DrawString(G.font, kb->name);
		}

		y-=CHANNELHEIGHT+CHANNELSKIP;

	}
}

/* left hand part */
static void draw_channel_names(void) 
{
	short ofsx, ofsy = 0; 
	bAction	*act;
	Key *key;

	/* Clip to the scrollable area */
	if(curarea->winx>SCROLLB+10 && curarea->winy>SCROLLH+10) {
		if(G.v2d->scroll) {	
			ofsx= curarea->winrct.xmin;	
			ofsy= curarea->winrct.ymin;
			glViewport(ofsx,  ofsy+G.v2d->mask.ymin, NAMEWIDTH, 
					   (ofsy+G.v2d->mask.ymax) -
					   (ofsy+G.v2d->mask.ymin)); 
			glScissor(ofsx,	 ofsy+G.v2d->mask.ymin, NAMEWIDTH, 
					  (ofsy+G.v2d->mask.ymax) -
					  (ofsy+G.v2d->mask.ymin));
		}
	}
	
	myortho2(0,	NAMEWIDTH, G.v2d->cur.ymin, G.v2d->cur.ymax);	//	Scaling
	
	glColor3ub(0x00, 0x00, 0x00);

	act=G.saction->action;

	if (act) {
		/* if there is a selected action then
		 * draw the channel names
		 */
		draw_action_channel_names(act);
	}
	if ( (key = get_action_mesh_key()) ) {
		/* if there is a mesh selected with rvk's,
			* then draw the RVK names
			*/
		draw_action_mesh_names(key);
    }

    myortho2(0,	NAMEWIDTH, 0, (ofsy+G.v2d->mask.ymax) -
              (ofsy+G.v2d->mask.ymin));	//	Scaling

}

int count_action_levels(bAction *act)
{
	int y=0;
	bActionChannel *achan;

	if (!act)
		return 0;

	for (achan=act->chanbase.first; achan; achan=achan->next){
		if((achan->flag & ACHAN_HIDDEN)==0) {
			y+=1;
			y+=BLI_countlist(&achan->constraintChannels);
		}
	}

	return y;
}

/* sets or clears hidden flags */
void check_action_context(SpaceAction *saction)
{
	bActionChannel *achan;
	
	if(saction->action==NULL) return;
	
	for (achan=saction->action->chanbase.first; achan; achan=achan->next)
		achan->flag &= ~ACHAN_HIDDEN;
	
	if (G.saction->pin==0 && OBACT) {
		Object *ob= OBACT;
		bPoseChannel *pchan;
		bArmature *arm= ob->data;
		
		for (achan=saction->action->chanbase.first; achan; achan=achan->next) {
			pchan= get_pose_channel(ob->pose, achan->name);
			if(pchan)
				if((pchan->bone->layer & arm->layer)==0)
					achan->flag |= ACHAN_HIDDEN;
		}
	}
}

static void draw_channel_strips(SpaceAction *saction)
{
	rcti scr_rct;
	gla2DDrawInfo *di;
	bAction	*act;
	bActionChannel *chan;
	bConstraintChannel *conchan;
	float y, sta, end;
	int act_start, act_end, dummy;
	char col1[3], col2[3];
	
	BIF_GetThemeColor3ubv(TH_SHADE2, col2);
	BIF_GetThemeColor3ubv(TH_HILITE, col1);

	act= saction->action;
	if (!act)
		return;

	scr_rct.xmin= saction->area->winrct.xmin + saction->v2d.mask.xmin;
	scr_rct.ymin= saction->area->winrct.ymin + saction->v2d.mask.ymin;
	scr_rct.xmax= saction->area->winrct.xmin + saction->v2d.hor.xmax;
	scr_rct.ymax= saction->area->winrct.ymin + saction->v2d.mask.ymax; 
	di= glaBegin2DDraw(&scr_rct, &G.v2d->cur);

	/* if in NLA there's a strip active, map the view */
	if (G.saction->pin==0 && OBACT)
		map_active_strip(di, OBACT, 0);
	
	/* start and end of action itself */
	calc_action_range(act, &sta, &end);
	gla2DDrawTranslatePt(di, sta, 0.0f, &act_start, &dummy);
	gla2DDrawTranslatePt(di, end, 0.0f, &act_end, &dummy);
	
	if (G.saction->pin==0 && OBACT)
		map_active_strip(di, OBACT, 1);
	
	/* first backdrop strips */
	y= count_action_levels(act)*(CHANNELHEIGHT+CHANNELSKIP);
	glEnable(GL_BLEND);
	for (chan=act->chanbase.first; chan; chan=chan->next){
		if((chan->flag & ACHAN_HIDDEN)==0) {
			int frame1_x, channel_y;
			
			gla2DDrawTranslatePt(di, G.v2d->cur.xmin, y, &frame1_x, &channel_y);
			
			if (chan->flag & ACHAN_SELECTED) glColor4ub(col1[0], col1[1], col1[2], 0x22);
			else glColor4ub(col2[0], col2[1], col2[2], 0x22);
			glRectf(frame1_x,  channel_y-CHANNELHEIGHT/2,  G.v2d->hor.xmax,  channel_y+CHANNELHEIGHT/2);
			
			if (chan->flag & ACHAN_SELECTED) glColor4ub(col1[0], col1[1], col1[2], 0x22);
			else glColor4ub(col2[0], col2[1], col2[2], 0x22);
			glRectf(act_start,  channel_y-CHANNELHEIGHT/2,  act_end,  channel_y+CHANNELHEIGHT/2);
			
			/*	Increment the step */
			y-=CHANNELHEIGHT+CHANNELSKIP;
			
			/* Draw constraint channels */
			for (conchan=chan->constraintChannels.first; conchan; conchan=conchan->next){
				gla2DDrawTranslatePt(di, 1, y, &frame1_x, &channel_y);
				
				if (conchan->flag & ACHAN_SELECTED) glColor4ub(col1[0], col1[1], col1[2], 0x22);
				else glColor4ub(col2[0], col2[1], col2[2], 0x22);
				glRectf(frame1_x,  channel_y-CHANNELHEIGHT/2+4,  G.v2d->hor.xmax,  channel_y+CHANNELHEIGHT/2-4);
				
				if (conchan->flag & ACHAN_SELECTED) glColor4ub(col1[0], col1[1], col1[2], 0x22);
				else glColor4ub(col2[0], col2[1], col2[2], 0x22);
				glRectf(act_start,  channel_y-CHANNELHEIGHT/2+4,  act_end,  channel_y+CHANNELHEIGHT/2-4);
				
				y-=CHANNELHEIGHT+CHANNELSKIP;
			}
		}
	}		
	glDisable(GL_BLEND);
	
	if (G.saction->pin==0 && OBACT)
		map_active_strip(di, OBACT, 0);
	
	/* dot thingies */
	y= count_action_levels(act)*(CHANNELHEIGHT+CHANNELSKIP);
	for (chan=act->chanbase.first; chan; chan=chan->next){
		if((chan->flag & ACHAN_HIDDEN)==0) {
			
			draw_ipo_channel(di, chan->ipo, 0, y);
			y-=CHANNELHEIGHT+CHANNELSKIP;

			/* Draw constraint channels */
			for (conchan=chan->constraintChannels.first; conchan; conchan=conchan->next){
				draw_ipo_channel(di, conchan->ipo, 0, y);
				y-=CHANNELHEIGHT+CHANNELSKIP;
			}
		}
	}

	if(saction->flag & SACTION_MOVING) {
		int frame1_x, channel_y;
		gla2DDrawTranslatePt(di, saction->timeslide, 0, &frame1_x, &channel_y);
		cpack(0x0);
		glBegin(GL_LINES);
		glVertex2f(frame1_x, G.v2d->mask.ymin - 100);
		glVertex2f(frame1_x, G.v2d->mask.ymax);
		glEnd();
	}
	
	glaEnd2DDraw(di);
}

static void draw_mesh_strips(SpaceAction *saction, Key *key)
{
	/* draw the RVK keyframes as those little square button things
	 */
	rcti scr_rct;
	gla2DDrawInfo *di;
	float	y, ybase;
	IpoCurve *icu;
	char col1[3], col2[3];
	
	BIF_GetThemeColor3ubv(TH_SHADE2, col2);
	BIF_GetThemeColor3ubv(TH_HILITE, col1);

	if (!key->ipo) return;

	scr_rct.xmin= saction->area->winrct.xmin + ACTWIDTH;
	scr_rct.ymin= saction->area->winrct.ymin + saction->v2d.mask.ymin;
	scr_rct.xmax= saction->area->winrct.xmin + saction->v2d.hor.xmax;
	scr_rct.ymax= saction->area->winrct.ymin + saction->v2d.mask.ymax; 
	di= glaBegin2DDraw(&scr_rct, &G.v2d->cur);

	ybase = key->totkey*(CHANNELHEIGHT+CHANNELSKIP);

	for (icu = key->ipo->curve.first; icu ; icu = icu->next) {

		int frame1_x, channel_y;

		/* lets not deal with the "speed" Ipo
		 */
		if (icu->adrcode==0) continue;

		y = ybase	- (CHANNELHEIGHT+CHANNELSKIP)*(icu->adrcode-1);
		gla2DDrawTranslatePt(di, 1, y, &frame1_x, &channel_y);

		/* all frames that have a frame number less than one
		 * get a desaturated orange background
		 */
		glEnable(GL_BLEND);
		glColor4ub(col2[0], col2[1], col2[2], 0x22);
		glRectf(0,        channel_y-CHANNELHEIGHT/2,  
				frame1_x, channel_y+CHANNELHEIGHT/2);

		/* frames one and higher get a saturated orange background
		 */
		glColor4ub(col2[0], col2[1], col2[2], 0x44);
		glRectf(frame1_x,         channel_y-CHANNELHEIGHT/2,  
				G.v2d->hor.xmax,  channel_y+CHANNELHEIGHT/2);
		glDisable(GL_BLEND);

		/* draw the little squares
		 */
		draw_icu_channel(di, icu, 0, y); 
	}

	glaEnd2DDraw(di);
}

/* ********* action panel *********** */


void do_actionbuts(unsigned short event)
{
	switch(event) {
	case REDRAWVIEW3D:
		allqueue(REDRAWVIEW3D, 0);
		break;
	case B_REDR:
		allqueue(REDRAWACTION, 0);
		break;
	}
}


static void action_panel_properties(short cntrl)	// ACTION_HANDLER_PROPERTIES
{
	uiBlock *block;

	block= uiNewBlock(&curarea->uiblocks, "action_panel_properties", UI_EMBOSS, UI_HELV, curarea->win);
	uiPanelControl(UI_PNL_SOLID | UI_PNL_CLOSE | cntrl);
	uiSetPanelHandler(ACTION_HANDLER_PROPERTIES);  // for close and esc
	if(uiNewPanel(curarea, block, "Transform Properties", "Action", 10, 230, 318, 204)==0) return;

	uiDefBut(block, LABEL, 0, "test text",		10,180,300,19, 0, 0, 0, 0, 0, "");

}

static void action_blockhandlers(ScrArea *sa)
{
	SpaceAction *sact= sa->spacedata.first;
	short a;
	
	for(a=0; a<SPACE_MAXHANDLER; a+=2) {
		switch(sact->blockhandler[a]) {

		case ACTION_HANDLER_PROPERTIES:
			action_panel_properties(sact->blockhandler[a+1]);
			break;
		
		}
		/* clear action value for event */
		sact->blockhandler[a+1]= 0;
	}
	uiDrawBlocksPanels(sa, 0);
}

void drawactionspace(ScrArea *sa, void *spacedata)
{
	short ofsx = 0, ofsy = 0;
	Key *key;
	float col[3];
	short maxymin;

	if (!G.saction)
		return;

	/* warning; blocks need to be freed each time, handlers dont remove  */
	uiFreeBlocksWin(&sa->uiblocks, sa->win);

	if (!G.saction->pin) {
		if (OBACT)
			G.saction->action = OBACT->action;
		else
			G.saction->action=NULL;
	}
	key = get_action_mesh_key();

	/* Damn I hate hunting to find my rvk's because
	 * they have scrolled off of the screen ... this
	 * oughta fix it
	 */
		   
	if (key) {
		if (G.v2d->cur.ymin < -CHANNELHEIGHT) 
			G.v2d->cur.ymin = -CHANNELHEIGHT;
		
		maxymin = key->totkey*(CHANNELHEIGHT+CHANNELSKIP);
		if (G.v2d->cur.ymin > maxymin) G.v2d->cur.ymin = maxymin;
	}

	/* Lets make sure the width of the left hand of the screen
	 * is set to an appropriate value based on whether sliders
	 * are showing of not
	 */
	if (key && showsliders) ACTWIDTH = NAMEWIDTH + SLIDERWIDTH;
	else ACTWIDTH = NAMEWIDTH;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;

	calc_scrollrcts(sa, G.v2d, curarea->winx, curarea->winy);

	/* background color for entire window (used in lefthand part tho) */
	BIF_GetThemeColor3fv(TH_HEADER, col);
	glClearColor(col[0], col[1], col[2], 0.0); 
	glClear(GL_COLOR_BUFFER_BIT);
	
	if(curarea->winx>SCROLLB+10 && curarea->winy>SCROLLH+10) {
		if(G.v2d->scroll) {	
			ofsx= curarea->winrct.xmin;	
			ofsy= curarea->winrct.ymin;
			glViewport(ofsx+G.v2d->mask.xmin,  ofsy+G.v2d->mask.ymin, 
					   ( ofsx+G.v2d->mask.xmax-1)-(ofsx+G.v2d->mask.xmin)+1, 
					   ( ofsy+G.v2d->mask.ymax-1)-( ofsy+G.v2d->mask.ymin)+1); 
			glScissor(ofsx+G.v2d->mask.xmin,  ofsy+G.v2d->mask.ymin, 
					  ( ofsx+G.v2d->mask.xmax-1)-(ofsx+G.v2d->mask.xmin)+1, 
					  ( ofsy+G.v2d->mask.ymax-1)-( ofsy+G.v2d->mask.ymin)+1);
		}
	}

	BIF_GetThemeColor3fv(TH_BACK, col);
	glClearColor(col[0], col[1], col[2], 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	myortho2(G.v2d->cur.xmin, G.v2d->cur.xmax, G.v2d->cur.ymin, G.v2d->cur.ymax);
	bwin_clear_viewmat(sa->win);	/* clear buttons view */
	glLoadIdentity();

	/*	Draw backdrop */
	calc_ipogrid();	
	draw_ipogrid();

	check_action_context(G.saction);
	
	/* Draw channel strips */
	draw_channel_strips(G.saction);

	if (key) {
		/* if there is a mesh with rvk's selected,
		 * then draw the key frames in the action window
		 */
		draw_mesh_strips(G.saction, key);
	}

	/* Draw current frame */
	glViewport(ofsx+G.v2d->mask.xmin,  
             ofsy+G.v2d->mask.ymin, 
             ( ofsx+G.v2d->mask.xmax-1)-(ofsx+G.v2d->mask.xmin)+1, 
             ( ofsy+G.v2d->mask.ymax-1)-( ofsy+G.v2d->mask.ymin)+1); 
	glScissor(ofsx+G.v2d->mask.xmin,  
            ofsy+G.v2d->mask.ymin, 
            ( ofsx+G.v2d->mask.xmax-1)-(ofsx+G.v2d->mask.xmin)+1, 
            ( ofsy+G.v2d->mask.ymax-1)-( ofsy+G.v2d->mask.ymin)+1);
	myortho2(G.v2d->cur.xmin, G.v2d->cur.xmax,  G.v2d->cur.ymin, G.v2d->cur.ymax);
	draw_cfra_action();

	/* Draw scroll */
	mywinset(curarea->win);	// reset scissor too
	if(curarea->winx>SCROLLB+10 && curarea->winy>SCROLLH+10) {
      myortho2(-0.375, curarea->winx-0.375, -0.375, curarea->winy-0.375);
      if(G.v2d->scroll) drawscroll(0);
	}

	if(G.v2d->mask.xmin!=0) {
		/* Draw channel names */
		draw_channel_names();

		if(sa->winx > 50 + NAMEWIDTH + SLIDERWIDTH) {
			if ( key ) {
				/* if there is a mesh with rvk's selected,
				 * then draw the key frames in the action window
				 */
				meshactionbuts(G.saction, OBACT, key);
			}
		}
	}
	
	mywinset(curarea->win);	// reset scissor too
	myortho2(-0.375, curarea->winx-0.375, -0.375, curarea->winy-0.375);
	draw_area_emboss(sa);

	/* it is important to end a view in a transform compatible with buttons */
	bwin_scalematrix(sa->win, G.saction->blockscale, G.saction->blockscale, G.saction->blockscale);
	action_blockhandlers(sa);

	curarea->win_swap= WIN_BACK_OK;
}

#if 0
/** Draw a nicely beveled diamond shape (in screen space) */
static void draw_key_but(int x, int y, int w, int h, int sel)
{
	int xmin= x, ymin= y;
	int xmax= x+w-1, ymax= y+h-1;
	int xc= (xmin+xmax)/2, yc= (ymin+ymax)/2;
	
	/* interior */
	if (sel) glColor3ub(0xF1, 0xCA, 0x13);
	else glColor3ub(0xAC, 0xAC, 0xAC);
	
	glBegin(GL_QUADS);
	glVertex2i(xc, ymin);
	glVertex2i(xmax, yc);
	glVertex2i(xc, ymax);
	glVertex2i(xmin, yc);
	glEnd();

	
	/* bevel */
	glBegin(GL_LINE_LOOP);
	
	if (sel) glColor3ub(0xD0, 0x7E, 0x06);
	else glColor3ub(0x8C, 0x8C, 0x8C);
	/* under */
	glVertex2i(xc, ymin+1);
	glVertex2i(xmax-1, yc);
	
	if (sel) glColor3ub(0xF4, 0xEE, 0x8E);
	else glColor3ub(0xDF, 0xDF, 0xDF);
	/* top */
	glVertex2i(xc, ymax-1);
	glVertex2i(xmin+1, yc);
	glEnd();
	
	/* outline */
	glColor3ub(0,0,0);
	glBegin(GL_LINE_LOOP);
	glVertex2i(xc, ymin);
	glVertex2i(xmax, yc);
	glVertex2i(xc, ymax);
	glVertex2i(xmin, yc);
	glEnd();
}

#endif

static void draw_keylist(gla2DDrawInfo *di, int totvert, BezTriple **blist, float ypos)
{
	int v;

	if (!blist)
		return;

	glEnable(GL_BLEND);
	
	for (v = 0; v<totvert; v++){
		if (v==0 || (blist[v]->vec[1][0] != blist[v-1]->vec[1][0])){
			int sc_x, sc_y;
			gla2DDrawTranslatePt(di, blist[v]->vec[1][0], ypos, &sc_x, &sc_y);
			// draw_key_but(sc_x-5, sc_y-6, 13, 13, (blist[v]->f2 & 1));
			
			if(blist[v]->f2 & 1) BIF_icon_draw_blended(sc_x-7, sc_y-6, ICON_SPACE2, TH_HEADER, 0);
			else BIF_icon_draw_blended(sc_x-7, sc_y-6, ICON_SPACE3, TH_HEADER, 0);

		}
	}			
	glDisable(GL_BLEND);
}

void draw_object_channel(gla2DDrawInfo *di, Object *ob, int flags, float ypos)
{
	BezTriple **blist;
	int totvert;

	blist = ob_to_keylist(ob, flags, &totvert);
	if (blist){
		draw_keylist(di,totvert, blist, ypos);
		MEM_freeN(blist);
	}
}

void draw_ipo_channel(gla2DDrawInfo *di, Ipo *ipo, int flags, float ypos)
{
	BezTriple **blist;
	int totvert;

	blist = ipo_to_keylist(ipo, flags, &totvert);
	if (blist){
		draw_keylist(di,totvert, blist, ypos);
		MEM_freeN(blist);
	}
}

void draw_icu_channel(gla2DDrawInfo *di, IpoCurve *icu, int flags, float ypos)
{
	/* draw the keys for an IpoCurve
	 */
	BezTriple **blist;
	int totvert;

	blist = icu_to_keylist(icu, flags, &totvert);
	if (blist){
		draw_keylist(di,totvert, blist, ypos);
		MEM_freeN(blist);
	}
}

void draw_action_channel(gla2DDrawInfo *di, bAction *act, int flags, float ypos)
{
	BezTriple **blist;
	int totvert;

	blist = action_to_keylist(act, flags, &totvert);
	if (blist){
		draw_keylist(di,totvert, blist, ypos);
		MEM_freeN(blist);
	}
}

static BezTriple **ob_to_keylist(Object *ob, int flags, int *totvert)
{
	IpoCurve *icu;
	bConstraintChannel *conchan;
	int v, count=0;

	BezTriple **list = NULL;

	if (ob){

		/* Count Object Keys */
		if (ob->ipo){
			for (icu=ob->ipo->curve.first; icu; icu=icu->next){
				count+=icu->totvert;
			}
		}
		
		/* Count Constraint Keys */
		for (conchan=ob->constraintChannels.first; conchan; conchan=conchan->next){
			if(conchan->ipo)
				for (icu=conchan->ipo->curve.first; icu; icu=icu->next)
					count+=icu->totvert;
		}
		
		/* Count object data keys */

		/* Build the list */
		if (count){
			list = MEM_callocN(sizeof(BezTriple*)*count, "beztlist");
			count=0;
			
			/* Add object keyframes */
			if(ob->ipo) {
				for (icu=ob->ipo->curve.first; icu; icu=icu->next){
					for (v=0; v<icu->totvert; v++){
						list[count++]=&icu->bezt[v];
					}
				}
			}			
			/* Add constraint keyframes */
			for (conchan=ob->constraintChannels.first; conchan; conchan=conchan->next){
				if(conchan->ipo)
					for (icu=conchan->ipo->curve.first; icu; icu=icu->next)
						for (v=0; v<icu->totvert; v++)
							list[count++]=&icu->bezt[v];
			}
			
			/* Add object data keyframes */

			/* Sort */
			qsort(list, count, sizeof(BezTriple*), bezt_compare);
		}
	}
	(*totvert)=count;
	return list;
}

static BezTriple **icu_to_keylist(IpoCurve *icu, int flags, int *totvert)
{
	/* compile a list of all bezier triples in an
	 * IpoCurve.
	 */
	int v, count = 0;

	BezTriple **list = NULL;

	count=icu->totvert;

	if (count){
		list = MEM_callocN(sizeof(BezTriple*)*count, "beztlist");
		count=0;
			
		for (v=0; v<icu->totvert; v++){
			list[count++]=&icu->bezt[v];
		}
		qsort(list, count, sizeof(BezTriple*), bezt_compare);
	}
	(*totvert)=count;
	return list;

}

static BezTriple **ipo_to_keylist(Ipo *ipo, int flags, int *totvert)
{
	IpoCurve *icu;
	int v, count=0;

	BezTriple **list = NULL;

	if (ipo){
		/* Count required keys */
		for (icu=ipo->curve.first; icu; icu=icu->next){
			count+=icu->totvert;
		}
		
		/* Build the list */
		if (count){
			list = MEM_callocN(sizeof(BezTriple*)*count, "beztlist");
			count=0;
			
			for (icu=ipo->curve.first; icu; icu=icu->next){
				for (v=0; v<icu->totvert; v++){
					list[count++]=&icu->bezt[v];
				}
			}			
			qsort(list, count, sizeof(BezTriple*), bezt_compare);
		}
	}
	(*totvert)=count;
	return list;
}

static BezTriple **action_to_keylist(bAction *act, int flags, int *totvert)
{
	IpoCurve *icu;
	bActionChannel *achan;
	bConstraintChannel *conchan;
	int v, count=0;

	BezTriple **list = NULL;

	if (act){
		/* Count required keys */
		for (achan=act->chanbase.first; achan; achan=achan->next){
			if((achan->flag & ACHAN_HIDDEN)==0) {
				/* Count transformation keys */
				if(achan->ipo) {
					for (icu=achan->ipo->curve.first; icu; icu=icu->next)
						count+=icu->totvert;
				}
				/* Count constraint keys */
				for (conchan=achan->constraintChannels.first; conchan; conchan=conchan->next)
					if(conchan->ipo) 
						for (icu=conchan->ipo->curve.first; icu; icu=icu->next)
							count+=icu->totvert;
			}				
		}
		
		/* Build the list */
		if (count){
			list = MEM_callocN(sizeof(BezTriple*)*count, "beztlist");
			count=0;
			
			for (achan=act->chanbase.first; achan; achan=achan->next){
				if((achan->flag & ACHAN_HIDDEN)==0) {
					if(achan->ipo) {
						/* Add transformation keys */
						for (icu=achan->ipo->curve.first; icu; icu=icu->next){
							for (v=0; v<icu->totvert; v++)
								list[count++]=&icu->bezt[v];
						}
					}					
						/* Add constraint keys */
					for (conchan=achan->constraintChannels.first; conchan; conchan=conchan->next){
						if(conchan->ipo)
							for (icu=conchan->ipo->curve.first; icu; icu=icu->next)
								for (v=0; v<icu->totvert; v++)
									list[count++]=&icu->bezt[v];
					}
				}							
			}		
			qsort(list, count, sizeof(BezTriple*), bezt_compare);
			
		}
	}
	(*totvert)=count;
	return list;
}

