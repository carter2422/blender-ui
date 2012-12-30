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
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef __PBVH_INTERN_H__
#define __PBVH_INTERN_H__

/* Axis-aligned bounding box */
typedef struct {
	float bmin[3], bmax[3];
} BB;

/* Axis-aligned bounding box with centroid */
typedef struct {
	float bmin[3], bmax[3], bcentroid[3];
} BBC;

/* Note: this structure is getting large, might want to split it into
 * union'd structs */
struct PBVHNode {
	/* Opaque handle for drawing code */
	GPU_Buffers *draw_buffers;

	/* Voxel bounds */
	BB vb;
	BB orig_vb;

	/* For internal nodes, the offset of the children in the PBVH
	 * 'nodes' array. */
	int children_offset;

	/* Pointer into the PBVH prim_indices array and the number of
	 * primitives used by this leaf node.
	 *
	 * Used for leaf nodes in both mesh- and multires-based PBVHs.
	 */
	int *prim_indices;
	unsigned int totprim;

	/* Array of indices into the mesh's MVert array. Contains the
	 * indices of all vertices used by faces that are within this
	 * node's bounding box.
	 *
	 * Note that a vertex might be used by a multiple faces, and
	 * these faces might be in different leaf nodes. Such a vertex
	 * will appear in the vert_indices array of each of those leaf
	 * nodes.
	 *
	 * In order to support cases where you want access to multiple
	 * nodes' vertices without duplication, the vert_indices array
	 * is ordered such that the first part of the array, up to
	 * index 'uniq_verts', contains "unique" vertex indices. These
	 * vertices might not be truly unique to this node, but if
	 * they appear in another node's vert_indices array, they will
	 * be above that node's 'uniq_verts' value.
	 *
	 * Used for leaf nodes in a mesh-based PBVH (not multires.)
	 */
	int *vert_indices;
	unsigned int uniq_verts, face_verts;

	/* An array mapping face corners into the vert_indices
	 * array. The array is sized to match 'totprim', and each of
	 * the face's corners gets an index into the vert_indices
	 * array, in the same order as the corners in the original
	 * MFace. The fourth value should not be used if the original
	 * face is a triangle.
	 *
	 * Used for leaf nodes in a mesh-based PBVH (not multires.)
	 */
	int (*face_vert_indices)[4];

	/* Indicates whether this node is a leaf or not; also used for
	 * marking various updates that need to be applied. */
	PBVHNodeFlags flag : 16;

	/* Used for raycasting: how close bb is to the ray point. */
	float tmin;

	/* Scalar displacements for sculpt mode's layer brush. */
	float *layer_disp;

	int proxy_count;
	PBVHProxyNode *proxies;

	/* Dyntopo */
	GHash *bm_faces;
	GHash *bm_unique_verts;
	GHash *bm_other_verts;
	float (*bm_orco)[3];
	int (*bm_ortri)[3];
	int bm_tot_ortri;
};

typedef enum {
	PBVH_DYNTOPO_SMOOTH_SHADING = 1
} PBVHFlags;

typedef struct PBVHBMeshLog PBVHBMeshLog;

struct PBVH {
	PBVHType type;
	PBVHFlags flags;

	PBVHNode *nodes;
	int node_mem_count, totnode;

	int *prim_indices;
	int totprim;
	int totvert;

	int leaf_limit;

	/* Mesh data */
	MVert *verts;
	MFace *faces;
	CustomData *vdata;

	/* Grid Data */
	CCGKey gridkey;
	CCGElem **grids;
	DMGridAdjacency *gridadj;
	void **gridfaces;
	const DMFlagMat *grid_flag_mats;
	int totgrid;
	BLI_bitmap *grid_hidden;

	/* Only used during BVH build and update,
	 * don't need to remain valid after */
	BLI_bitmap vert_bitmap;

#ifdef PERFCNTRS
	int perf_modified;
#endif

	/* flag are verts/faces deformed */
	int deformed;

	int show_diffuse_color;

	/* Dynamic topology */
	BMesh *bm;
	GHash *bm_face_to_node;
	GHash *bm_vert_to_node;
	float bm_max_edge_len;
	float bm_min_edge_len;

	struct BMLog *bm_log;
};

/* pbvh.c */
void BB_reset(BB *bb);
void BB_expand(BB *bb, const float co[3]);
void BB_expand_with_bb(BB *bb, BB *bb2);
void BBC_update_centroid(BBC *bbc);
int BB_widest_axis(const BB *bb);
void pbvh_grow_nodes(PBVH *bvh, int totnode);
int ray_face_intersection(const float ray_start[3], const float ray_normal[3],
						  const float *t0, const float *t1, const float *t2,
						  const float *t3, float *fdist);
void pbvh_update_BB_redraw(PBVH *bvh, PBVHNode **nodes, int totnode, int flag);

/* pbvh_bmesh.c */
int pbvh_bmesh_node_raycast(PBVHNode *node, const float ray_start[3],
							const float ray_normal[3], float *dist,
							int use_original);

void pbvh_bmesh_normals_update(PBVHNode **nodes, int totnode);

#endif
