/**
 * $Id: node_shader_output.c 32517 2010-10-16 14:32:17Z campbellbarton $
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
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "../node_shader_util.h"
#include "node_shader_noise.h"

static float voronoi_tex(int coloring, float scale, float vec[3], float color[3])
{
	float da[4];
	float pa[4][3];
	float fac;
	float p[3];

	/* compute distance and point coordinate of 4 nearest neighbours */
	mul_v3_v3fl(p, vec, scale);
	voronoi_generic(p, SHD_VORONOI_DISTANCE_SQUARED, 1.0f, da, pa);

	/* output */
	if(coloring == SHD_VORONOI_INTENSITY) {
		fac = fabsf(da[0]);
		color[0]= color[1]= color[2]= fac;
	}
	else {
		cellnoise_color(color, pa[0]);
		fac= (color[0] + color[1] + color[2])*(1.0f/3.0f);
	}

	return fac;
}

/* **************** VORONOI ******************** */

static bNodeSocketTemplate sh_node_tex_voronoi_in[]= {
	{	SOCK_VECTOR, 1, "Vector",		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, PROP_NONE, SOCK_HIDE_VALUE},
	{	SOCK_FLOAT, 1, "Scale",			5.0f, 0.0f, 0.0f, 0.0f, -1000.0f, 1000.0f},
	{	-1, 0, ""	}
};

static bNodeSocketTemplate sh_node_tex_voronoi_out[]= {
	{	SOCK_RGBA, 0, "Color",		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
	{	SOCK_FLOAT, 0, "Fac",		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
	{	-1, 0, ""	}
};

static void node_shader_init_tex_voronoi(bNodeTree *UNUSED(ntree), bNode* node, bNodeTemplate *UNUSED(ntemp))
{
	NodeTexVoronoi *tex = MEM_callocN(sizeof(NodeTexVoronoi), "NodeTexVoronoi");
	default_tex_mapping(&tex->base.tex_mapping);
	default_color_mapping(&tex->base.color_mapping);
	tex->coloring = SHD_VORONOI_INTENSITY;

	node->storage = tex;
}

static void node_shader_exec_tex_voronoi(void *data, bNode *node, bNodeStack **in, bNodeStack **out)
{
	ShaderCallData *scd= (ShaderCallData*)data;
	NodeTexVoronoi *tex= (NodeTexVoronoi*)node->storage;
	bNodeSocket *vecsock = node->inputs.first;
	float vec[3], scale;
	
	if(vecsock->link)
		nodestack_get_vec(vec, SOCK_VECTOR, in[0]);
	else
		copy_v3_v3(vec, scd->co);

	nodestack_get_vec(&scale, SOCK_FLOAT, in[1]);

	out[1]->vec[0]= voronoi_tex(tex->coloring, scale, vec, out[0]->vec);
}

static int node_shader_gpu_tex_voronoi(GPUMaterial *mat, bNode *node, GPUNodeStack *in, GPUNodeStack *out)
{
	if(!in[0].link)
		in[0].link = GPU_attribute(CD_ORCO, "");

	node_shader_gpu_tex_mapping(mat, node, in, out);

	return GPU_stack_link(mat, "node_tex_voronoi", in, out);
}

/* node type definition */
void register_node_type_sh_tex_voronoi(ListBase *lb)
{
	static bNodeType ntype;

	node_type_base(&ntype, SH_NODE_TEX_VORONOI, "Voronoi Texture", NODE_CLASS_TEXTURE, 0);
	node_type_compatibility(&ntype, NODE_NEW_SHADING);
	node_type_socket_templates(&ntype, sh_node_tex_voronoi_in, sh_node_tex_voronoi_out);
	node_type_size(&ntype, 150, 60, 200);
	node_type_init(&ntype, node_shader_init_tex_voronoi);
	node_type_storage(&ntype, "NodeTexVoronoi", node_free_standard_storage, node_copy_standard_storage);
	node_type_exec(&ntype, node_shader_exec_tex_voronoi);
	node_type_gpu(&ntype, node_shader_gpu_tex_voronoi);

	nodeRegisterType(lb, &ntype);
};

