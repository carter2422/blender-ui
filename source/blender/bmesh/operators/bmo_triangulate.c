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
 * Contributor(s): Joseph Eagar.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/bmesh/operators/bmo_triangulate.c
 *  \ingroup bmesh
 */

#include "MEM_guardedalloc.h"
#include "DNA_listBase.h"

#include "BLI_math.h"
#include "BLI_array.h"
#include "BLI_smallhash.h"
#include "BLI_scanfill.h"

#include "bmesh.h"
#include "intern/bmesh_private.h"

#include "intern/bmesh_operators_private.h" /* own include */

#define ELE_NEW		1
#define FACE_MARK	2
#define EDGE_MARK	4

void bmo_triangulate_exec(BMesh *bm, BMOperator *op)
{
	const bool use_beauty = BMO_slot_bool_get(op->slots_in, "use_beauty");
	BMOpSlot *slot_facemap_out = BMO_slot_get(op->slots_out, "face_map.out");

	BM_mesh_elem_hflag_disable_all(bm, BM_FACE | BM_EDGE, BM_ELEM_TAG, false);
	BMO_slot_buffer_hflag_enable(bm, op->slots_in, "faces", BM_FACE, BM_ELEM_TAG, false);

	BM_mesh_triangulate(bm, use_beauty, true, op, slot_facemap_out);

	BMO_slot_buffer_from_enabled_hflag(bm, op, op->slots_out, "edges.out", BM_EDGE, BM_ELEM_TAG);
	BMO_slot_buffer_from_enabled_hflag(bm, op, op->slots_out, "faces.out", BM_FACE, BM_ELEM_TAG);
}

void bmo_beautify_fill_exec(BMesh *bm, BMOperator *op)
{
	BMOIter siter;
	BMIter iter;
	BMFace *f;
	BMEdge *e;
	int stop = 0;
	
	BMO_slot_buffer_flag_enable(bm, op->slots_in, "edges", BM_EDGE, EDGE_MARK);
	
	BMO_ITER (f, &siter, op->slots_in, "faces", BM_FACE) {
		if (f->len == 3) {
			BMO_elem_flag_enable(bm, f, FACE_MARK);
		}
	}

	while (!stop) {
		stop = 1;
		
		BM_ITER_MESH (e, &iter, bm, BM_EDGES_OF_MESH) {
			BMVert *v1, *v2, *v3, *v4;
			
			if (!BM_edge_is_manifold(e) || !BMO_elem_flag_test(bm, e, EDGE_MARK)) {
				continue;
			}

			if (!BMO_elem_flag_test(bm, e->l->f, FACE_MARK) ||
			    !BMO_elem_flag_test(bm, e->l->radial_next->f, FACE_MARK))
			{
				continue;
			}

			v1 = e->l->prev->v;               /* first face vert not attached to 'e' */
			v2 = e->l->v;                     /* e->v1 or e->v2*/
			v3 = e->l->radial_next->prev->v;  /* second face vert not attached to 'e' */
			v4 = e->l->next->v;               /* e->v1 or e->v2*/

			if (UNLIKELY(v1 == v3)) {
				// printf("This should never happen, but does sometimes!\n");
				continue;
			}

			// printf("%p %p %p %p - %p %p\n", v1, v2, v3, v4, e->l->f, e->l->radial_next->f);
			BLI_assert((ELEM3(v1, v2, v3, v4) == false) &&
			           (ELEM3(v2, v1, v3, v4) == false) &&
			           (ELEM3(v3, v1, v2, v4) == false) &&
			           (ELEM3(v4, v1, v2, v3) == false));

			if (is_quad_convex_v3(v1->co, v2->co, v3->co, v4->co)) {
				float len1, len2, len3, len4, len5, len6, opp1, opp2, fac1, fac2;
				/* testing rule:
				 * the area divided by the total edge lengths
				 */
				len1 = len_v3v3(v1->co, v2->co);
				len2 = len_v3v3(v2->co, v3->co);
				len3 = len_v3v3(v3->co, v4->co);
				len4 = len_v3v3(v4->co, v1->co);
				len5 = len_v3v3(v1->co, v3->co);
				len6 = len_v3v3(v2->co, v4->co);

				opp1 = area_tri_v3(v1->co, v2->co, v3->co);
				opp2 = area_tri_v3(v1->co, v3->co, v4->co);

				fac1 = opp1 / (len1 + len2 + len5) + opp2 / (len3 + len4 + len5);

				opp1 = area_tri_v3(v2->co, v3->co, v4->co);
				opp2 = area_tri_v3(v2->co, v4->co, v1->co);

				fac2 = opp1 / (len2 + len3 + len6) + opp2 / (len4 + len1 + len6);
				
				if (fac1 > fac2) {
					e = BM_edge_rotate(bm, e, false, BM_EDGEROT_CHECK_EXISTS);
					if (e) {
						BMO_elem_flag_enable(bm, e, ELE_NEW | EDGE_MARK);

						BMO_elem_flag_enable(bm, e->l->f, FACE_MARK | ELE_NEW);
						BMO_elem_flag_enable(bm, e->l->radial_next->f, FACE_MARK | ELE_NEW);
						stop = 0;
					}
				}
			}
		}
	}
	
	BMO_slot_buffer_from_enabled_flag(bm, op, op->slots_out, "geom.out", BM_EDGE | BM_FACE, ELE_NEW);
}

void bmo_triangle_fill_exec(BMesh *bm, BMOperator *op)
{
	const bool use_beauty = BMO_slot_bool_get(op->slots_in, "use_beauty");
	BMOIter siter;
	BMEdge *e;
	ScanFillContext sf_ctx;
	/* ScanFillEdge *sf_edge; */ /* UNUSED */
	ScanFillVert *sf_vert, *sf_vert_1, *sf_vert_2;
	ScanFillFace *sf_tri;
	SmallHash hash;

	BLI_smallhash_init(&hash);
	
	BLI_scanfill_begin(&sf_ctx);
	
	BMO_ITER (e, &siter, op->slots_in, "edges", BM_EDGE) {
		BMO_elem_flag_enable(bm, e, EDGE_MARK);
		
		if (!BLI_smallhash_haskey(&hash, (uintptr_t)e->v1)) {
			sf_vert = BLI_scanfill_vert_add(&sf_ctx, e->v1->co);
			sf_vert->tmp.p = e->v1;
			BLI_smallhash_insert(&hash, (uintptr_t)e->v1, sf_vert);
		}
		
		if (!BLI_smallhash_haskey(&hash, (uintptr_t)e->v2)) {
			sf_vert = BLI_scanfill_vert_add(&sf_ctx, e->v2->co);
			sf_vert->tmp.p = e->v2;
			BLI_smallhash_insert(&hash, (uintptr_t)e->v2, sf_vert);
		}
		
		sf_vert_1 = BLI_smallhash_lookup(&hash, (uintptr_t)e->v1);
		sf_vert_2 = BLI_smallhash_lookup(&hash, (uintptr_t)e->v2);
		/* sf_edge = */ BLI_scanfill_edge_add(&sf_ctx, sf_vert_1, sf_vert_2);
		/* sf_edge->tmp.p = e; */ /* UNUSED */
	}
	
	BLI_scanfill_calc(&sf_ctx, BLI_SCANFILL_CALC_HOLES);
	
	for (sf_tri = sf_ctx.fillfacebase.first; sf_tri; sf_tri = sf_tri->next) {
		BMFace *f = BM_face_create_quad_tri(bm,
		                                    sf_tri->v1->tmp.p, sf_tri->v2->tmp.p, sf_tri->v3->tmp.p, NULL,
		                                    NULL, true);
		BMLoop *l;
		BMIter liter;
		
		BMO_elem_flag_enable(bm, f, ELE_NEW);
		BM_ITER_ELEM (l, &liter, f, BM_LOOPS_OF_FACE) {
			if (!BMO_elem_flag_test(bm, l->e, EDGE_MARK)) {
				BMO_elem_flag_enable(bm, l->e, ELE_NEW);
			}
		}
	}
	
	BLI_scanfill_end(&sf_ctx);
	BLI_smallhash_release(&hash);
	
	if (use_beauty) {
		BMOperator bmop;

		BMO_op_initf(bm, &bmop, op->flag, "beautify_fill faces=%ff edges=%Fe", ELE_NEW, EDGE_MARK);
		BMO_op_exec(bm, &bmop);
		BMO_slot_buffer_flag_enable(bm, bmop.slots_out, "geom.out", BM_FACE | BM_EDGE, ELE_NEW);
		BMO_op_finish(bm, &bmop);
	}
	
	BMO_slot_buffer_from_enabled_flag(bm, op, op->slots_out, "geom.out", BM_EDGE | BM_FACE, ELE_NEW);
}
