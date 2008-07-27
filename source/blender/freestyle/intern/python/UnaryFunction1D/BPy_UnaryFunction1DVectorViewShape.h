#ifndef FREESTYLE_PYTHON_UNARYFUNCTION1DVECTORVIEWSHAPE_H
#define FREESTYLE_PYTHON_UNARYFUNCTION1DVECTORVIEWSHAPE_H

#include "../BPy_UnaryFunction1D.h"

#include <vector>
#include "../../view_map/ViewMap.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

#include <Python.h>

extern PyTypeObject UnaryFunction1DVectorViewShape_Type;

#define BPy_UnaryFunction1DVectorViewShape_Check(v)	(( (PyObject *) v)->ob_type == &UnaryFunction1DVectorViewShape_Type)

/*---------------------------Python BPy_UnaryFunction1DVectorViewShape structure definition----------*/
typedef struct {
	BPy_UnaryFunction1D py_uf1D;
	UnaryFunction1D< std::vector<ViewShape*> > *uf1D_vectorviewshape;
} BPy_UnaryFunction1DVectorViewShape;

/*---------------------------Python BPy_UnaryFunction1DVectorViewShape visible prototypes-----------*/
PyMODINIT_FUNC UnaryFunction1DVectorViewShape_Init( PyObject *module );


///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* FREESTYLE_PYTHON_UNARYFUNCTION1DVECTORVIEWSHAPE_H */
