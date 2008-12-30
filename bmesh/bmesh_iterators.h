/*
 * BMESH ITERATORS
 * 
 * The functions and structures in this file 
 * provide a unified method for iterating over 
 * the elements of a mesh and answering simple
 * adjacency queries. Tool authors should use
 * the iterators provided in this file instead
 * of inspecting the structure directly.
 *
*/

#ifndef BMESH_ITERATORS_H
#define BMESH_ITERATORS_H

/*Defines for passing to BMIter_New*/
#define BM_VERTS 			1
#define BM_EDGES 			2
#define BM_FACES 			3
#define BM_EDGES_OF_VERT 			4
#define BM_FACES_OF_VERT 			5
#define BM_VERTS_OF_EDGE 			6
#define BM_FACES_OF_EDGE 			7
#define BM_VERTS_OF_FACE 			8
#define BM_FACEVERTS_OF_FACE 		9
#define BM_EDGES_OF_FACE 			10
#define BM_LOOPS_OF_FACE 			11

/*Iterator Structure*/
typedef struct BMIter{
	struct BMVert *firstvert, *nextvert, *vdata;
	struct BMEdge *firstedge, *nextedge, *edata;
	struct BMLoop *firstloop, *nextloop, *ldata;
	struct BMFace *firstpoly, *nextpoly, *pdata;
	struct BMesh *bm;
	void (*begin)(struct BMIter *iter);
	void *(*step)(struct BMIter *iter);
	union{
		void		*p;
		int			i;
		long		l;
		float		f;
	}filter;
	int type, count;
}BMIter;

void *BMIter_New(struct BMIter *iter, struct BMesh *bm, int type, void *data);
void *BMIter_Step(struct BMIter *iter);

#endif
