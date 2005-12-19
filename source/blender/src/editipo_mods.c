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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * Contributor(s): Blender Foundation, 2005. Full recode
 *
 * ***** END GPL LICENSE BLOCK *****
 */


/* ********** Selection and set Handle code for editing Ipos in Blender ************* */
/*
  mouse_select_ipo() is in editipo.c
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif   

#include "BLI_blenlib.h"
#include "BLI_arithb.h"

#include "DNA_curve_types.h"
#include "DNA_ipo_types.h"
#include "DNA_key_types.h"
#include "DNA_object_types.h"
#include "DNA_space_types.h"
#include "DNA_scene_types.h"
#include "DNA_view3d_types.h"

#include "BKE_global.h"
#include "BKE_ipo.h"
#include "BKE_utildefines.h"

#include "BIF_interface.h"
#include "BIF_screen.h"
#include "BIF_space.h"
#include "BIF_toolbox.h"

#include "BSE_edit.h"
#include "BSE_editipo_types.h"
#include "BSE_editipo.h"
#include "BSE_drawipo.h"
#include "BSE_trans_types.h"

#include "BDR_drawobject.h"

#include "blendef.h"
#include "mydevice.h"

/* copy from editipo.c */
#define BEZSELECTED(bezt)   (((bezt)->f1 & 1) || ((bezt)->f2 & 1) || ((bezt)->f3 & 1))

#define ISPOIN(a, b, c)                       ( (a->b) && (a->c) )
#define ISPOIN3(a, b, c, d)           ( (a->b) && (a->c) && (a->d) )
#define ISPOIN4(a, b, c, d, e)        ( (a->b) && (a->c) && (a->d) && (a->e) )   

extern int totipo_edit, totipo_sel, totipo_vertsel, totipo_vis;

void ipo_toggle_showkey(void) 
{
	if(G.sipo->showkey) {
		G.sipo->showkey= 0;
		swap_selectall_editipo();	/* sel all */
	}
	else G.sipo->showkey= 1;
	free_ipokey(&G.sipo->ipokey);
	if(G.sipo->ipo) G.sipo->ipo->showkey= G.sipo->showkey;
	
	BIF_undo_push("Toggle show key Ipo");
}

void swap_selectall_editipo(void)
{
	Object *ob;
	EditIpo *ei;
	IpoKey *ik;
	BezTriple *bezt;
	int a, b; /*  , sel=0; */
	
	get_status_editipo();
	
	if(G.sipo->showkey) {
		ik= G.sipo->ipokey.first;
		while(ik) {
			if(totipo_vertsel) ik->flag &= ~1;
			else ik->flag |= 1;
			ik= ik->next;
		}
		update_editipo_flags();
		
		if(G.sipo->showkey && G.sipo->blocktype==ID_OB ) {
			ob= OBACT;
			if(ob && (ob->ipoflag & OB_DRAWKEY)) draw_object_ext(BASACT);
		}
	}
	else if(totipo_edit==0) {
		ei= G.sipo->editipo;
		if (ei){
			for(a=0; a<G.sipo->totipo; a++) {
				if( ei->flag & IPO_VISIBLE ) {
					if(totipo_sel) ei->flag &= ~IPO_SELECT;
					else ei->flag |= IPO_SELECT;
				}
				ei++;
			}
			update_editipo_flags();
		}
		get_status_editipo();
	}
	else {
		ei= G.sipo->editipo;
		for(a=0; a<G.sipo->totipo; a++) {
			if (ISPOIN3(ei, flag & IPO_VISIBLE, flag & IPO_EDIT, icu )) {
				bezt= ei->icu->bezt;
				if(bezt) {
					b= ei->icu->totvert;
					while(b--) {
						if(totipo_vertsel) {
							bezt->f1= bezt->f2= bezt->f3= 0;
						}
						else {
							bezt->f1= bezt->f2= bezt->f3= 1;
						}
						bezt++;
					}
				}
			}
			ei++;
		}
		
	}
	
	BIF_undo_push("Swap select all Ipo");
	scrarea_queue_winredraw(curarea);
	
}

void swap_visible_editipo(void)
{
	EditIpo *ei;
	Object *ob;
	int a; /*  , sel=0; */
	
	get_status_editipo();
	
	
	ei= G.sipo->editipo;
	for(a=0; a<G.sipo->totipo; a++) {
		if(totipo_vis==0) {
			if(ei->icu) {
				ei->flag |= IPO_VISIBLE;
				ei->flag |= IPO_SELECT;
			}
		}
		else ei->flag &= ~IPO_VISIBLE;
		ei++;
	}
	
	update_editipo_flags();
	
	if(G.sipo->showkey) {
		
		make_ipokey();
		
		ob= OBACT;
		if(ob && (ob->ipoflag & OB_DRAWKEY)) allqueue(REDRAWVIEW3D, 0);
	}
	
	scrarea_queue_winredraw(curarea);
	BIF_undo_push("Swap Visible Ipo");	
}

void deselectall_editipo(void)
{
	EditIpo *ei;
	IpoKey *ik;
	BezTriple *bezt;
	int a, b; /*  , sel=0; */
	
	get_status_editipo();
	
	if(G.sipo->showkey) {
		ik= G.sipo->ipokey.first;
		while(ik) {
			ik->flag &= ~1;
			ik= ik->next;
		}
		update_editipo_flags();
		
	}
	else if(totipo_edit==0) {
		
		ei= G.sipo->editipo;
		for(a=0; a<G.sipo->totipo; a++) {
			if( ei->flag & IPO_VISIBLE ) {
				ei->flag &= ~IPO_SELECT;
			}
			ei++;
		}
		update_editipo_flags();
	}
	else {
		ei= G.sipo->editipo;
		for(a=0; a<G.sipo->totipo; a++) {
			if (ISPOIN3(ei, flag & IPO_VISIBLE, flag & IPO_EDIT, icu )) {
				if(ei->icu->bezt) {
					bezt= ei->icu->bezt;
					b= ei->icu->totvert;
					while(b--) {
						bezt->f1= bezt->f2= bezt->f3= 0;
						bezt++;
					}
				}
			}
			ei++;
		}
	}
	
	BIF_undo_push("(De)select all Ipo");
	scrarea_queue_winredraw(curarea);
}


static int icu_keys_bezier_loop(IpoCurve *icu,
                         int (*bezier_function)(BezTriple *),
                         void (ipocurve_function)(struct IpoCurve *icu)) 
{
    /*  This loops through the beziers in the Ipocurve, and executes 
     *  the generic user provided 'bezier_function' on each one. 
     *  Optionally executes the generic function ipocurve_function on the 
     *  IPO curve after looping (eg. calchandles_ipocurve)
     */

    int b;
    BezTriple *bezt;

    b    = icu->totvert;
    bezt = icu->bezt;

    /* if bezier_function has been specified
     * then loop through each bezier executing
     * it.
     */

    if (bezier_function != NULL) {
        while(b--) {
            /* exit with return code 1 if the bezier function 
             * returns 1 (good for when you are only interested
             * in finding the first bezier that
             * satisfies a condition).
             */
            if (bezier_function(bezt)) return 1;
            bezt++;
        }
    }

    /* if ipocurve_function has been specified 
     * then execute it
     */
    if (ipocurve_function != NULL)
        ipocurve_function(icu);

    return 0;

}

static int ipo_keys_bezier_loop(Ipo *ipo,
                         int (*bezier_function)(BezTriple *),
                         void (ipocurve_function)(struct IpoCurve *icu))
{
    /*  This loops through the beziers that are attached to
     *  the selected keys on the Ipocurves of the Ipo, and executes 
     *  the generic user provided 'bezier_function' on each one. 
     *  Optionally executes the generic function ipocurve_function on a 
     *  IPO curve after looping (eg. calchandles_ipocurve)
     */

    IpoCurve *icu;
	
	if(ipo==NULL) return 0;
	
    /* Loop through each curve in the Ipo
     */
    for (icu=ipo->curve.first; icu; icu=icu->next){
        if (icu_keys_bezier_loop(icu,bezier_function, ipocurve_function))
            return 1;
    }

    return 0;
}

static int selected_bezier_loop(int (*looptest)(EditIpo *),
                         int (*bezier_function)(BezTriple *),
                         void (ipocurve_function)(struct IpoCurve *icu))
{
	/*  This loops through the beziers that are attached to
	 *  selected keys in editmode in the IPO window, and executes 
	 *  the generic user-provided 'bezier_function' on each one 
	 *  that satisfies the 'looptest' function. Optionally executes
	 *  the generic function ipocurve_function on a IPO curve
	 *  after looping (eg. calchandles_ipocurve)
	 */

	EditIpo *ei;
	BezTriple *bezt;
	int a, b;

	/* Get the first Edit Ipo from the selected Ipos
	 */
	ei= G.sipo->editipo;

	/* Loop throught all of the selected Ipo's
	 */
	for(a=0; a<G.sipo->totipo; a++, ei++) {
		/* Do a user provided test on the Edit Ipo
		 * to determine whether we want to process it
		 */
		if (looptest(ei)) {
			/* Loop through the selected
			 * beziers on the Edit Ipo
			 */
			bezt = ei->icu->bezt;
			b    = ei->icu->totvert;
			
			/* if bezier_function has been specified
			 * then loop through each bezier executing
			 * it.
			 */
			if (bezier_function != NULL) {
				while(b--) {
					/* exit with return code 1 if the bezier function 
					 * returns 1 (good for when you are only interested
					 * in finding the first bezier that
					 * satisfies a condition).
					 */
					if (bezier_function(bezt)) return 1;
					bezt++;
				}
			}

			/* if ipocurve_function has been specified 
			 * then execute it
			 */
			if (ipocurve_function != NULL)
				ipocurve_function(ei->icu);
		}
		/* nufte flourdje zim ploopydu <-- random dutch looking comment ;) */
		/* looks more like russian to me! (ton) */
	}

	return 0;
}

int select_bezier_add(BezTriple *bezt) 
{
	/* Select the bezier triple */
	bezt->f1 |= 1;
	bezt->f2 |= 1;
	bezt->f3 |= 1;
	return 0;
}

int select_bezier_subtract(BezTriple *bezt) 
{
	/* Deselect the bezier triple */
	bezt->f1 &= ~1;
	bezt->f2 &= ~1;
	bezt->f3 &= ~1;
	return 0;
}

int select_bezier_invert(BezTriple *bezt) 
{
	/* Invert the selection for the bezier triple */
	bezt->f2 ^= 1;
	if ( bezt->f2 & 1 ) {
		bezt->f1 |= 1;
		bezt->f3 |= 1;
	}
	else {
		bezt->f1 &= ~1;
		bezt->f3 &= ~1;
	}
	return 0;
}

static int set_bezier_auto(BezTriple *bezt) 
{
	/* Sets the selected bezier handles to type 'auto' 
	 */

	/* is a handle selected? If so
	 * set it to type auto
	 */
	if(bezt->f1 || bezt->f3) {
		if(bezt->f1) bezt->h1= 1; /* the secret code for auto */
		if(bezt->f3) bezt->h2= 1;

		/* if the handles are not of the same type, set them
		 * to type free
		 */
		if(bezt->h1!=bezt->h2) {
			if ELEM(bezt->h1, HD_ALIGN, HD_AUTO) bezt->h1= HD_FREE;
			if ELEM(bezt->h2, HD_ALIGN, HD_AUTO) bezt->h2= HD_FREE;
		}
	}
	return 0;
}

static int set_bezier_vector(BezTriple *bezt) 
{
	/* Sets the selected bezier handles to type 'vector' 
	 */

	/* is a handle selected? If so
	 * set it to type vector
	 */
	if(bezt->f1 || bezt->f3) {
		if(bezt->f1) bezt->h1= 2; /* the code for vector */
		if(bezt->f3) bezt->h2= 2;
    
		/* if the handles are not of the same type, set them
		 * to type free
		 */
		if(bezt->h1!=bezt->h2) {
			if ELEM(bezt->h1, HD_ALIGN, HD_AUTO) bezt->h1= HD_FREE;
			if ELEM(bezt->h2, HD_ALIGN, HD_AUTO) bezt->h2= HD_FREE;
		}
	}
	return 0;
}

static int bezier_isfree(BezTriple *bezt) 
{
	/* queries whether the handle should be set
	 * to type 'free' (I think)
	 */
	if(bezt->f1 && bezt->h1) return 1;
	if(bezt->f3 && bezt->h2) return 1;
	return 0;
}

static int set_bezier_free(BezTriple *bezt) 
{
	/* Sets selected bezier handles to type 'free' 
	 */
	if(bezt->f1) bezt->h1= HD_FREE;
	if(bezt->f3) bezt->h2= HD_FREE;
	return 0;
}

static int set_bezier_align(BezTriple *bezt) 
{
	/* Sets selected bezier handles to type 'align' 
	 */
	if(bezt->f1) bezt->h1= HD_ALIGN;
	if(bezt->f3) bezt->h2= HD_ALIGN;
	return 0;
}

static int vis_edit_icu_bez(EditIpo *ei) 
{
	/* A 4 part test for an EditIpo :
	 *   is it a) visible
	 *         b) in edit mode
	 *         c) does it contain an Ipo Curve
	 *         d) does that ipo curve have a bezier
	 *
	 * (The reason why I don't just use the macro
	 * is I need a pointer to a function.)
	 */
	return (ISPOIN4(ei, flag & IPO_VISIBLE, flag & IPO_EDIT, icu, icu->bezt));
}

void select_ipo_bezier_keys(Ipo *ipo, int selectmode)
{
  /* Select all of the beziers in all
   * of the Ipo curves belonging to the
   * Ipo, using the selection mode.
   */
  switch (selectmode) {
  case SELECT_ADD:
    ipo_keys_bezier_loop(ipo, select_bezier_add, NULL);
    break;
  case SELECT_SUBTRACT:
    ipo_keys_bezier_loop(ipo, select_bezier_subtract, NULL);
    break;
  case SELECT_INVERT:
    ipo_keys_bezier_loop(ipo, select_bezier_invert, NULL);
    break;
  }
}

void sethandles_ipo_keys(Ipo *ipo, int code)
{
	/* this function lets you set bezier handles all to
	 * one type for some Ipo's (e.g. with hotkeys through
	 * the action window).
	 */ 

	/* code==1: set autohandle */
	/* code==2: set vectorhandle */
	/* als code==3 (HD_ALIGN) toggelt het, vectorhandles worden HD_FREE */
	
	switch(code) {
	case 1:
		/*** Set to auto ***/
		ipo_keys_bezier_loop(ipo, set_bezier_auto,
							 calchandles_ipocurve);
		break;
	case 2:
		/*** Set to vector ***/
		ipo_keys_bezier_loop(ipo, set_bezier_vector,
                         calchandles_ipocurve);
		break;
	default:
		if ( ipo_keys_bezier_loop(ipo, bezier_isfree, NULL) ) {
			/*** Set to free ***/
			ipo_keys_bezier_loop(ipo, set_bezier_free,
                           calchandles_ipocurve);
		}
		else {
			/*** Set to align ***/
			ipo_keys_bezier_loop(ipo, set_bezier_align,
                           calchandles_ipocurve);
		}
		break;
	}
}

static int snap_bezier(BezTriple *bezt)
{
	if(bezt->f2 & SELECT)
		bezt->vec[1][0]= (float)(floor(bezt->vec[1][0]+0.5));
	return 0;
}

void snap_ipo_keys(Ipo *ipo)
{
	ipo_keys_bezier_loop(ipo, snap_bezier, calchandles_ipocurve);
}

static void ipo_curves_auto_horiz(void)
{
    EditIpo *ei;
	int a, set= 1;
	
	ei= G.sipo->editipo;
	for(a=0; a<G.sipo->totipo; a++, ei++) {
		if (ISPOIN3(ei, flag & IPO_VISIBLE, flag & IPO_SELECT, icu))
			if(ei->flag & IPO_AUTO_HORIZ) set= 0;
	}
	
	ei= G.sipo->editipo;
	for(a=0; a<G.sipo->totipo; a++, ei++) {
		if (ISPOIN3(ei, flag & IPO_VISIBLE, flag & IPO_SELECT, icu)) {
			if(set) ei->flag |= IPO_AUTO_HORIZ;
			else ei->flag &= ~IPO_AUTO_HORIZ;
		}
	}
	update_editipo_flags();
}

void sethandles_ipo(int code)
{
	/* this function lets you set bezier handles all to
	 * one type for some selected keys in edit mode in the
	 * IPO window (e.g. with hotkeys)
	 */ 

	/* code==1: set autohandle */
	/* code==2: set vectorhandle */
	/* als code==3 (HD_ALIGN) toggelt het, vectorhandles worden HD_FREE */

	if(G.sipo->ipo && G.sipo->ipo->id.lib) return;

	switch(code) {
	case HD_AUTO:
		/*** Set to auto ***/
		selected_bezier_loop(vis_edit_icu_bez, set_bezier_auto,
                         calchandles_ipocurve);
		break;
	case HD_VECT:
		/*** Set to vector ***/
		selected_bezier_loop(vis_edit_icu_bez, set_bezier_vector,
                         calchandles_ipocurve);
		break;
	case HD_AUTO_ANIM:
		/* set to enforce autohandles to be horizontal on extremes */
		ipo_curves_auto_horiz();
		
		break;
	default:
		if (selected_bezier_loop(vis_edit_icu_bez, bezier_isfree, NULL) ) {
			/*** Set to free ***/
			selected_bezier_loop(vis_edit_icu_bez, set_bezier_free,
								 calchandles_ipocurve);
		}
		else {
			/*** Set to align ***/
			selected_bezier_loop(vis_edit_icu_bez, set_bezier_align,
								 calchandles_ipocurve);
		}
		break;
	}

	editipo_changed(G.sipo, 1);
	BIF_undo_push("Set handles Ipo");
}


static void set_ipocurve_constant(struct IpoCurve *icu) {
	/* Sets the type of the IPO curve to constant
	 */
	icu->ipo= IPO_CONST;
}

static void set_ipocurve_linear(struct IpoCurve *icu) {
	/* Sets the type of the IPO curve to linear
	 */
	icu->ipo= IPO_LIN;
}

static void set_ipocurve_bezier(struct IpoCurve *icu) {
	/* Sets the type of the IPO curve to bezier
	 */
	icu->ipo= IPO_BEZ;
}


void setipotype_ipo(Ipo *ipo, int code)
{
	/* Sets the type of the each ipo curve in the
	 * Ipo to a value based on the code
	 */
	switch (code) {
	case 1:
		ipo_keys_bezier_loop(ipo, NULL, set_ipocurve_constant);
		break;
	case 2:
		ipo_keys_bezier_loop(ipo, NULL, set_ipocurve_linear);
		break;
	case 3:
		ipo_keys_bezier_loop(ipo, NULL, set_ipocurve_bezier);
		break;
	}
}

void setexprap_ipoloop(Ipo *ipo, int code)
{
	IpoCurve *icu;

    	/* Loop through each curve in the Ipo
     	*/
    	for (icu=ipo->curve.first; icu; icu=icu->next)
        	icu->extrap= code;
}

void set_ipotype(void)
{
	EditIpo *ei;
	int a;
	short event;

	if(G.sipo->ipo && G.sipo->ipo->id.lib) return;
	if(G.sipo->showkey) return;
	get_status_editipo();
	
	if(G.sipo->blocktype==ID_KE && totipo_edit==0 && totipo_sel==0) {
		Key *key= (Key *)G.sipo->from;
		Object *ob= OBACT;
		KeyBlock *kb;
		
		if(key==NULL) return;
		kb= BLI_findlink(&key->block, ob->shapenr-1);
		
		event= pupmenu("Key Type %t|Linear %x1|Cardinal %x2|B Spline %x3");
		if(event < 1) return;

		kb->type= 0;
		if(event==1) kb->type= KEY_LINEAR;
		if(event==2) kb->type= KEY_CARDINAL;
		if(event==3) kb->type= KEY_BSPLINE;
	}
	else {
		event= pupmenu("Ipo Type %t|Constant %x1|Linear %x2|Bezier %x3");
		if(event < 1) return;
		
		ei= G.sipo->editipo;
		for(a=0; a<G.sipo->totipo; a++, ei++) {
			if (ISPOIN3(ei, flag & IPO_VISIBLE, flag & IPO_SELECT, icu)) {
				if(event==1) ei->icu->ipo= IPO_CONST;
				else if(event==2) ei->icu->ipo= IPO_LIN;
				else ei->icu->ipo= IPO_BEZ;
			}
		}
	}
	BIF_undo_push("Set ipo type");
	scrarea_queue_winredraw(curarea);
}

void borderselect_ipo(void)
{
	EditIpo *ei;
	IpoKey *ik;
	BezTriple *bezt;
	rcti rect;
	rctf rectf;
	int a, b, val;
	short mval[2];

	get_status_editipo();
	
	val= get_border(&rect, 3);

	if(val) {
		mval[0]= rect.xmin;
		mval[1]= rect.ymin;
		areamouseco_to_ipoco(G.v2d, mval, &rectf.xmin, &rectf.ymin);
		mval[0]= rect.xmax;
		mval[1]= rect.ymax;
		areamouseco_to_ipoco(G.v2d, mval, &rectf.xmax, &rectf.ymax);

		if(G.sipo->showkey) {
			ik= G.sipo->ipokey.first;
			while(ik) {
				if(rectf.xmin<ik->val && rectf.xmax>ik->val) {
					if(val==LEFTMOUSE) ik->flag |= 1;
					else ik->flag &= ~1;
				}
				ik= ik->next;
			}
			update_editipo_flags();
		}
		else if(totipo_edit==0) {
			if(rect.xmin<rect.xmax && rect.ymin<rect.ymax)
				select_proj_ipo(&rectf, val);
		}
		else {
			
			ei= G.sipo->editipo;
			for(a=0; a<G.sipo->totipo; a++, ei++) {
				if (ISPOIN3(ei, flag & IPO_VISIBLE, flag & IPO_EDIT, icu)) {
					if(ei->icu->bezt) {
						b= ei->icu->totvert;
						bezt= ei->icu->bezt;
						while(b--) {
							int bit= (val==LEFTMOUSE);
							
							if(BLI_in_rctf(&rectf, bezt->vec[0][0], bezt->vec[0][1]))
								bezt->f1 = (bezt->f1&~1) | bit;
							if(BLI_in_rctf(&rectf, bezt->vec[1][0], bezt->vec[1][1]))
								bezt->f2 = (bezt->f2&~1) | bit;
							if(BLI_in_rctf(&rectf, bezt->vec[2][0], bezt->vec[2][1]))
								bezt->f3 = (bezt->f3&~1) | bit;

							bezt++;
						}
					}
				}
			}
		}
		BIF_undo_push("Border select Ipo");
		scrarea_queue_winredraw(curarea);
	}
}



void nextkey(ListBase *elems, int dir)
{
	IpoKey *ik, *previk;
	int totsel;
	
	if(dir==1) ik= elems->last;
	else ik= elems->first;
	previk= 0;
	totsel= 0;
	
	while(ik) {
		
		if(ik->flag) totsel++;
		
		if(previk) {
			if(G.qual & LR_SHIFTKEY) {
				if(ik->flag) previk->flag= 1;
			}
			else previk->flag= ik->flag;
		}
		
		previk= ik;
		if(dir==1) ik= ik->prev;
		else ik= ik->next;
		
		if(G.qual & LR_SHIFTKEY);
		else if(ik==0) previk->flag= 0;
	}
	
	/* when no key select: */
	if(totsel==0) {
		if(dir==1) ik= elems->first;
		else ik= elems->last;
		
		if(ik) ik->flag= 1;
	}
}


void nextkey_ipo(int dir)			/* call from ipo queue */
{
	IpoKey *ik;
	int a;
	
	if(G.sipo->showkey==0) return;
	
	nextkey(&G.sipo->ipokey, dir);
	
	/* copy to beziers */
	ik= G.sipo->ipokey.first;
	while(ik) {
		for(a=0; a<G.sipo->totipo; a++) {
			if(ik->data[a]) ik->data[a]->f1= ik->data[a]->f2= ik->data[a]->f3= ik->flag;
		}
		ik= ik->next;
	}		
	
	allqueue(REDRAWNLA, 0);
	allqueue(REDRAWACTION, 0);
	allqueue(REDRAWIPO, 0);
	if(G.sipo->blocktype == ID_OB) allqueue(REDRAWVIEW3D, 0);
}

void nextkey_obipo(int dir)		/* only call external from view3d queue */
{
	Base *base;
	Object *ob;
	ListBase elems;
	IpoKey *ik;
	int a;
	
	/* problem: this doesnt work when you mix dLoc keys with Loc keys */
	
	base= FIRSTBASE;
	while(base) {
		if TESTBASE(base) {
			ob= base->object;
			if( (ob->ipoflag & OB_DRAWKEY) && ob->ipo && ob->ipo->showkey) {
				elems.first= elems.last= 0;
				make_ipokey_transform(ob, &elems, 0);
				
				if(elems.first) {
					
					nextkey(&elems, dir);
					
					/* copy to beziers */
					ik= elems.first;
					while(ik) {
						for(a=0; a<OB_TOTIPO; a++) {
							if(ik->data[a]) ik->data[a]->f1= ik->data[a]->f2= ik->data[a]->f3= ik->flag;
						}
						ik= ik->next;
					}
					
					free_ipokey(&elems);
				}
			}
		}
		
		base= base->next;
	}
	allqueue(REDRAWNLA, 0);
	allqueue(REDRAWACTION, 0);
	allqueue(REDRAWVIEW3D, 0);
	allspace(REMAKEIPO, 0);
	allqueue(REDRAWIPO, 0);
}

int is_ipo_key_selected(Ipo *ipo)
{
	int i;
	IpoCurve *icu;
	
	if (!ipo)
		return 0;
	
	for (icu=ipo->curve.first; icu; icu=icu->next){
		for (i=0; i<icu->totvert; i++)
			if (BEZSELECTED(&icu->bezt[i]))
				return 1;
	}
	
	return 0;
}

void set_ipo_key_selection(Ipo *ipo, int sel)
{
	int i;
	IpoCurve *icu;
	
	if (!ipo)
		return;
	
	for (icu=ipo->curve.first; icu; icu=icu->next){
		for (i=0; i<icu->totvert; i++){
			if (sel){
				icu->bezt[i].f1|=1;
				icu->bezt[i].f2|=1;
				icu->bezt[i].f3|=1;
			}
			else{
				icu->bezt[i].f1&=~1;
				icu->bezt[i].f2&=~1;
				icu->bezt[i].f3&=~1;
			}
		}
	}
}

int fullselect_ipo_keys(Ipo *ipo)
{
	int i;
	IpoCurve *icu;
	int tvtot = 0;
	
	if (!ipo)
		return tvtot;
	
	for (icu=ipo->curve.first; icu; icu=icu->next){
		for (i=0; i<icu->totvert; i++){
			if (icu->bezt[i].f2 & 1){
				tvtot+=3;
				icu->bezt[i].f1 |= 1;
				icu->bezt[i].f3 |= 1;
			}
		}
	}
	
	return tvtot;
}


void borderselect_icu_key(IpoCurve *icu, float xmin, float xmax, 
						  int (*select_function)(BezTriple *))
{
	/* Selects all bezier triples in the Ipocurve 
	* between times xmin and xmax, using the selection
	* function.
	*/
	
	int i;
	
	/* loop through all of the bezier triples in
	* the Ipocurve -- if the triple occurs between
	* times xmin and xmax then select it using the selection
	* function
	*/
	for (i=0; i<icu->totvert; i++){
		if (icu->bezt[i].vec[1][0] > xmin && icu->bezt[i].vec[1][0] < xmax ){
			select_function(&(icu->bezt[i]));
		}
	}
}

void borderselect_ipo_key(Ipo *ipo, float xmin, float xmax, int selectmode)
{
	/* Selects all bezier triples in each Ipocurve of the
	* Ipo between times xmin and xmax, using the selection mode.
	*/
	
	IpoCurve *icu;
	int (*select_function)(BezTriple *);
	
	/* If the ipo is no good then return */
	if (!ipo)
		return;
	
	/* Set the selection function based on the
		* selection mode.
		*/
	switch(selectmode) {
		case SELECT_ADD:
			select_function = select_bezier_add;
			break;
		case SELECT_SUBTRACT:
			select_function = select_bezier_subtract;
			break;
		case SELECT_INVERT:
			select_function = select_bezier_invert;
			break;
		default:
			return;
	}
	
	/* loop through all of the bezier triples in all
		* of the Ipocurves -- if the triple occurs between
		* times xmin and xmax then select it using the selection
		* function
		*/
	for (icu=ipo->curve.first; icu; icu=icu->next){
		borderselect_icu_key(icu, xmin, xmax, select_function);
	}
}

void select_ipo_key(Ipo *ipo, float selx, int selectmode)
{
	/* Selects all bezier triples in each Ipocurve of the
	* Ipo at time selx, using the selection mode.
	*/
	int i;
	IpoCurve *icu;
	int (*select_function)(BezTriple *);
	
	/* If the ipo is no good then return */
	if (!ipo)
		return;
	
	/* Set the selection function based on the
		* selection mode.
		*/
	switch(selectmode) {
		case SELECT_ADD:
			select_function = select_bezier_add;
			break;
		case SELECT_SUBTRACT:
			select_function = select_bezier_subtract;
			break;
		case SELECT_INVERT:
			select_function = select_bezier_invert;
			break;
		default:
			return;
	}
	
	/* loop through all of the bezier triples in all
		* of the Ipocurves -- if the triple occurs at
		* time selx then select it using the selection
		* function
		*/
	for (icu=ipo->curve.first; icu; icu=icu->next){
		for (i=0; i<icu->totvert; i++){
			if (icu->bezt[i].vec[1][0]==selx){
				select_function(&(icu->bezt[i]));
			}
		}
	}
}

void select_icu_key(IpoCurve *icu, float selx, int selectmode)
{
    /* Selects all bezier triples in the Ipocurve
	* at time selx, using the selection mode.
	* This is kind of sloppy the obvious similarities
	* with the above function, forgive me ...
	*/
    int i;
    int (*select_function)(BezTriple *);
	
    /* If the icu is no good then return */
    if (!icu)
        return;
	
    /* Set the selection function based on the
		* selection mode.
		*/
    switch(selectmode) {
		case SELECT_ADD:
			select_function = select_bezier_add;
			break;
		case SELECT_SUBTRACT:
			select_function = select_bezier_subtract;
			break;
		case SELECT_INVERT:
			select_function = select_bezier_invert;
			break;
		default:
			return;
    }
	
    /* loop through all of the bezier triples in
		* the Ipocurve -- if the triple occurs at
		* time selx then select it using the selection
		* function
		*/
    for (i=0; i<icu->totvert; i++){
        if (icu->bezt[i].vec[1][0]==selx){
            select_function(&(icu->bezt[i]));
        }
    }
}

void set_exprap_ipo(int mode)
{
	EditIpo *ei;
	int a;
	
	if(G.sipo->ipo && G.sipo->ipo->id.lib) return;
	/* in case of keys: always ok */
	
	ei= G.sipo->editipo;
	for(a=0; a<G.sipo->totipo; a++, ei++) {
		if (ISPOIN(ei, flag & IPO_VISIBLE, icu)) {
			if( (ei->flag & IPO_EDIT) || (ei->flag & IPO_SELECT) || (G.sipo->showkey) ) {
				ei->icu->extrap= mode;
			}
		}
	}
	
	editipo_changed(G.sipo, 1);
	BIF_undo_push("Set extrapolation Ipo");
}
