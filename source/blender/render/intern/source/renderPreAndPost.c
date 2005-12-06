/**
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
 * Prepare the scene data for rendering.
 * 
 * $Id$
 */

#include "MEM_guardedalloc.h"

#include "DNA_group_types.h"

#include "render.h"
#include "renderPreAndPost.h"
#include "RE_callbacks.h"

#include "shadbuf.h"
#include "envmap.h"
#include "renderHelp.h"
#include "radio.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/**
 * Rotate all objects, make shadowbuffers and environment maps.
 */
void prepareScene()
{
	GroupObject *go;
	extern void makeoctree(void);
	
	RE_local_get_renderdata();

	/* SHADOW BUFFER */
	for(go=R.lights.first; go; go= go->next) {
		LampRen *lar= go->lampren;
		if(lar->shb) makeshadowbuf(lar);
		if(RE_local_test_break()) break;
	}

	/* yafray: 'direct' radiosity, environment maps and octree init not needed for yafray render */
	/* although radio mode could be useful at some point, later */
	if (R.r.renderer==R_INTERN) {

		/* RADIO */
		if(R.r.mode & R_RADIO) do_radio_render();

		/* octree */
		if(R.r.mode & R_RAYTRACE) makeoctree();

		/* ENVIRONMENT MAPS */
		make_envmaps();
		
	}
}

void finalizeScene(void)
{
	extern void freeoctree(void);

	/* Among other things, it releases the shadow buffers. */
	RE_local_free_renderdata();
	/* yafray: freeoctree not needed after yafray render, not initialized, see above */
	if (R.r.renderer==R_INTERN) {
		if(R.r.mode & R_RAYTRACE) freeoctree();
	}
}


void doClipping( void (*projectfunc)(float *, float *) )
{
	setzbufvlaggen(projectfunc);
}

/* eof */
