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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 * Efficient memory allocation for lots of similar small chunks.
 */

/** \file blender/blenlib/intern/BLI_memarena.c
 *  \ingroup bli
 */

#include <string.h>

#include "MEM_guardedalloc.h"

#include "BLI_memarena.h"
#include "BLI_linklist.h"
#include "BLI_strict_flags.h"

#ifdef WITH_MEM_VALGRIND
#  include "valgrind/memcheck.h"
#endif

struct MemArena {
	unsigned char *curbuf;
	int bufsize, cursize;
	const char *name;

	int use_calloc;
	int align;

	LinkNode *bufs;
};

MemArena *BLI_memarena_new(const int bufsize, const char *name)
{
	MemArena *ma = MEM_callocN(sizeof(*ma), "memarena");
	ma->bufsize = bufsize;
	ma->align = 8;
	ma->name = name;

#ifdef WITH_MEM_VALGRIND
	VALGRIND_CREATE_MEMPOOL(ma, 0, false);
#endif

	return ma;
}

void BLI_memarena_use_calloc(MemArena *ma)
{
	ma->use_calloc = 1;
}

void BLI_memarena_use_malloc(MemArena *ma)
{
	ma->use_calloc = 0;
}

void BLI_memarena_use_align(struct MemArena *ma, const int align)
{
	/* align should be a power of two */
	ma->align = align;
}

void BLI_memarena_free(MemArena *ma)
{
	BLI_linklist_freeN(ma->bufs);
	MEM_freeN(ma);
}

/* amt must be power of two */
#define PADUP(num, amt) (((num) + ((amt) - 1)) & ~((amt) - 1))

/* align alloc'ed memory (needed if align > 8) */
static void memarena_curbuf_align(MemArena *ma)
{
	unsigned char *tmp;

	tmp = (unsigned char *)PADUP( (intptr_t) ma->curbuf, ma->align);
	ma->cursize -= (int)(tmp - ma->curbuf);
	ma->curbuf = tmp;
}

void *BLI_memarena_alloc(MemArena *ma, int size)
{
	void *ptr;

	/* ensure proper alignment by rounding
	 * size up to multiple of 8 */
	size = PADUP(size, ma->align);

	if (size > ma->cursize) {
		if (size > ma->bufsize - (ma->align - 1)) {
			ma->cursize = PADUP(size + 1, ma->align);
		}
		else
			ma->cursize = ma->bufsize;

		if (ma->use_calloc)
			ma->curbuf = MEM_callocN((size_t)ma->cursize, ma->name);
		else
			ma->curbuf = MEM_mallocN((size_t)ma->cursize, ma->name);

		BLI_linklist_prepend(&ma->bufs, ma->curbuf);
		memarena_curbuf_align(ma);
	}

	ptr = ma->curbuf;
	ma->curbuf += size;
	ma->cursize -= size;

#ifdef WITH_MEM_VALGRIND
	VALGRIND_MEMPOOL_ALLOC(ma, ptr, size);
#endif

	return ptr;
}

/**
 * Clear for reuse, avoids re-allocation when an arena may
 * otherwise be free'd and recreated.
 */
void BLI_memarena_clear(MemArena *ma)
{
	if (ma->bufs) {
		unsigned char *curbuf_prev;
		int curbuf_used;

		if (ma->bufs->next) {
			BLI_linklist_freeN(ma->bufs->next);
			ma->bufs->next = NULL;
		}

		curbuf_prev = ma->curbuf;
		ma->curbuf = ma->bufs->link;
		memarena_curbuf_align(ma);

		/* restore to original size */
		curbuf_used = (int)(curbuf_prev - ma->curbuf);
		ma->cursize += curbuf_used;

		if (ma->use_calloc) {
			memset(ma->curbuf, 0, (size_t)curbuf_used);
		}
	}

#ifdef WITH_MEM_VALGRIND
	VALGRIND_DESTROY_MEMPOOL(ma);
	VALGRIND_CREATE_MEMPOOL(ma, 0, false);
#endif

}
