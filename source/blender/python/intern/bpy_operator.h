
/**
 * $Id$
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
 * Contributor(s): Campbell Barton
 *
 * ***** END GPL LICENSE BLOCK *****
 */
#ifndef BPY_OPERATOR_H
#define BPY_OPERATOR_H

#include <Python.h>

#include "RNA_access.h"
#include "RNA_types.h"
#include "DNA_windowmanager_types.h"
#include "BKE_context.h"

extern PyTypeObject pyop_base_Type;

#define BPy_OperatorBase_Check(v)	(PyObject_TypeCheck(v, &pyop_base_Type))

typedef struct {
	PyObject_HEAD /* required python macro   */
} BPy_OperatorBase;

PyObject *BPY_operator_module(void);

/* fill in properties from a python dict */
int PYOP_props_from_dict(PointerRNA *ptr, PyObject *kw);

#endif
