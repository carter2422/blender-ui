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
 * The Original Code is Copyright (C) 2008 by Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): Geoffery Bantle
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/blenlib/intern/BLI_mempool.c
 *  \ingroup bli
 */

/*
 * Simple, fast memory allocator for allocating many elements of the same size.
 */

#include "BLI_utildefines.h"
#include "BLI_listbase.h"

#include "BLI_mempool.h" /* own include */

#include "DNA_listBase.h"

#include "MEM_guardedalloc.h"

#include <string.h>
#include <stdlib.h>

/* note: copied from BKE_utildefines.h, dont use here because we're in BLI */
#ifdef __BIG_ENDIAN__
/* Big Endian */
#  define MAKE_ID(a,b,c,d) ( (int)(a)<<24 | (int)(b)<<16 | (c)<<8 | (d) )
#else
/* Little Endian */
#  define MAKE_ID(a,b,c,d) ( (int)(d)<<24 | (int)(c)<<16 | (b)<<8 | (a) )
#endif

#define FREEWORD MAKE_ID('f', 'r', 'e', 'e')

typedef struct BLI_freenode {
	struct BLI_freenode *next;
	int freeword; /* used to identify this as a freed node */
} BLI_freenode;

typedef struct BLI_mempool_chunk {
	struct BLI_mempool_chunk *next, *prev;
	void *data;
} BLI_mempool_chunk;

struct BLI_mempool {
	struct ListBase chunks;
	int esize, csize, pchunk;        /* size of elements and chunks in bytes
	                                  * and number of elements per chunk*/
	short use_sysmalloc, allow_iter;
	/* keeps aligned to 16 bits */

	BLI_freenode *free;	             /* free element list. Interleaved into chunk datas.*/
	int totalloc, totused;           /* total number of elements allocated in total,
	                                  * and currently in use*/
};

#define MEMPOOL_ELEM_SIZE_MIN (sizeof(void *) * 2)

BLI_mempool *BLI_mempool_create(int esize, int tote, int pchunk,
                                short use_sysmalloc, short allow_iter)
{
	BLI_mempool  *pool = NULL;
	BLI_freenode *lasttail = NULL, *curnode = NULL;
	int i,j, maxchunks;
	char *addr;

	if (esize < MEMPOOL_ELEM_SIZE_MIN)
		esize = MEMPOOL_ELEM_SIZE_MIN;

	/*allocate the pool structure*/
	pool = use_sysmalloc ? malloc(sizeof(BLI_mempool)) : MEM_mallocN(sizeof(BLI_mempool), "memory pool");
	pool->esize = allow_iter ? MAX2(esize, sizeof(BLI_freenode)) : esize;
	pool->use_sysmalloc = use_sysmalloc;
	pool->pchunk = pchunk;
	pool->csize = esize * pchunk;
	pool->chunks.first = pool->chunks.last = NULL;
	pool->totused= 0;
	pool->allow_iter= allow_iter;
	
	maxchunks = tote / pchunk + 1;
	if (maxchunks==0) maxchunks = 1;

	/*allocate the actual chunks*/
	for (i=0; i < maxchunks; i++) {
		BLI_mempool_chunk *mpchunk = use_sysmalloc ? malloc(sizeof(BLI_mempool_chunk)) : MEM_mallocN(sizeof(BLI_mempool_chunk), "BLI_Mempool Chunk");
		mpchunk->next = mpchunk->prev = NULL;
		mpchunk->data = use_sysmalloc ? malloc(pool->csize) : MEM_mallocN(pool->csize, "BLI Mempool Chunk Data");
		BLI_addtail(&(pool->chunks), mpchunk);
		
		if (i==0) {
			pool->free = mpchunk->data; /*start of the list*/
			if (pool->allow_iter)
				pool->free->freeword = FREEWORD;
		}

		/*loop through the allocated data, building the pointer structures*/
		for (addr = mpchunk->data, j=0; j < pool->pchunk; j++) {
			curnode = ((BLI_freenode*)addr);
			addr += pool->esize;
			curnode->next = (BLI_freenode*)addr;
			if (pool->allow_iter) {
				if (j != pool->pchunk-1)
					curnode->next->freeword = FREEWORD;
				curnode->freeword = FREEWORD;
			}
		}
		/*final pointer in the previously allocated chunk is wrong.*/
		if (lasttail) {
			lasttail->next = mpchunk->data;
			if (pool->allow_iter)
				lasttail->freeword = FREEWORD;
		}

		/*set the end of this chunks memoryy to the new tail for next iteration*/
		lasttail = curnode;

		pool->totalloc += pool->pchunk;
	}
	/*terminate the list*/
	curnode->next = NULL;
	return pool;
}

void *BLI_mempool_alloc(BLI_mempool *pool)
{
	void *retval=NULL;

	pool->totused++;

	if (!(pool->free)) {
		BLI_freenode *curnode=NULL;
		char *addr;
		int j;

		/*need to allocate a new chunk*/
		BLI_mempool_chunk *mpchunk = pool->use_sysmalloc ? malloc(sizeof(BLI_mempool_chunk)) :  MEM_mallocN(sizeof(BLI_mempool_chunk), "BLI_Mempool Chunk");
		mpchunk->next = mpchunk->prev = NULL;
		mpchunk->data = pool->use_sysmalloc ? malloc(pool->csize) : MEM_mallocN(pool->csize, "BLI_Mempool Chunk Data");
		BLI_addtail(&(pool->chunks), mpchunk);

		pool->free = mpchunk->data; /*start of the list*/
		if (pool->allow_iter)
			pool->free->freeword = FREEWORD;
		for(addr = mpchunk->data, j=0; j < pool->pchunk; j++){
			curnode = ((BLI_freenode*)addr);
			addr += pool->esize;
			curnode->next = (BLI_freenode*)addr;

			if (pool->allow_iter) {
				curnode->freeword = FREEWORD;
				if (j != pool->pchunk-1)
					curnode->next->freeword = FREEWORD;
			}
		}
		curnode->next = NULL; /*terminate the list*/

		pool->totalloc += pool->pchunk;
	}

	retval = pool->free;
	if (pool->allow_iter)
		pool->free->freeword = 0x7FFFFFFF;

	pool->free = pool->free->next;
	//memset(retval, 0, pool->esize);
	return retval;
}

void *BLI_mempool_calloc(BLI_mempool *pool)
{
	void *retval= BLI_mempool_alloc(pool);
	memset(retval, 0, pool->esize);
	return retval;
}

/* doesnt protect against double frees, dont be stupid! */
void BLI_mempool_free(BLI_mempool *pool, void *addr)
{
	BLI_freenode *newhead = addr;

	if (pool->allow_iter)
		newhead->freeword = FREEWORD;
	newhead->next = pool->free;
	pool->free = newhead;

	pool->totused--;

	/*nothing is in use; free all the chunks except the first*/
	if (pool->totused == 0) {
		BLI_freenode *curnode=NULL;
		char *tmpaddr=NULL;
		int i;

		BLI_mempool_chunk *mpchunk=NULL;
		BLI_mempool_chunk *first= pool->chunks.first;

		BLI_remlink(&pool->chunks, first);

		for (mpchunk = pool->chunks.first; mpchunk; mpchunk = mpchunk->next) {
			if (pool->use_sysmalloc) free(mpchunk->data);
			else                     MEM_freeN(mpchunk->data);
		}

		pool->use_sysmalloc ? BLI_freelist(&(pool->chunks)) : BLI_freelistN(&(pool->chunks));
		
		BLI_addtail(&pool->chunks, first);
		pool->totalloc = pool->pchunk;

		pool->free = first->data; /*start of the list*/
		for (tmpaddr = first->data, i=0; i < pool->pchunk; i++) {
			curnode = ((BLI_freenode*)tmpaddr);
			tmpaddr += pool->esize;
			curnode->next = (BLI_freenode*)tmpaddr;
		}
		curnode->next = NULL; /*terminate the list*/
	}
}

void BLI_mempool_iternew(BLI_mempool *pool, BLI_mempool_iter *iter)
{
	if (!pool->allow_iter) {
		fprintf(stderr, "%s: Error! you can't iterate over this mempool!\n", __func__);
		iter->curchunk = NULL;
		iter->curindex = 0;
		
		return;
	}
	
	iter->pool = pool;
	iter->curchunk = pool->chunks.first;
	iter->curindex = 0;
}

static void *bli_mempool_iternext(BLI_mempool_iter *iter)
{
	void *ret = NULL;
	
	if (!iter->curchunk || !iter->pool->totused) return NULL;
	
	ret = ((char*)iter->curchunk->data) + iter->pool->esize*iter->curindex;
	
	iter->curindex++;
	
	if (iter->curindex >= iter->pool->pchunk) {
		iter->curchunk = iter->curchunk->next;
		iter->curindex = 0;
	}
	
	return ret;
}

void *BLI_mempool_iterstep(BLI_mempool_iter *iter)
{
	BLI_freenode *ret;
	
	do {
		ret = bli_mempool_iternext(iter);
	} while (ret && ret->freeword == FREEWORD);
	
	return ret;
}

void BLI_mempool_destroy(BLI_mempool *pool)
{
	BLI_mempool_chunk *mpchunk=NULL;

	if (pool->use_sysmalloc) {
		for (mpchunk = pool->chunks.first; mpchunk; mpchunk = mpchunk->next) {
			free(mpchunk->data);
		}
		BLI_freelist(&(pool->chunks));
		free(pool);
	}
	else {
		for (mpchunk = pool->chunks.first; mpchunk; mpchunk = mpchunk->next) {
			MEM_freeN(mpchunk->data);
		}
		BLI_freelistN(&(pool->chunks));
		MEM_freeN(pool);
	}
}
