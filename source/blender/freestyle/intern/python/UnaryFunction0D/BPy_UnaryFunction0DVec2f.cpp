#include "BPy_UnaryFunction0DVec2f.h"

#include "../BPy_Convert.h"
#include "../Iterator/BPy_Interface0DIterator.h"

#include "UnaryFunction0D_Vec2f/BPy_Normal2DF0D.h"
#include "UnaryFunction0D_Vec2f/BPy_VertexOrientation2DF0D.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for UnaryFunction0DVec2f instance  -----------*/
static int UnaryFunction0DVec2f___init__(BPy_UnaryFunction0DVec2f* self);
static void UnaryFunction0DVec2f___dealloc__(BPy_UnaryFunction0DVec2f* self);
static PyObject * UnaryFunction0DVec2f___repr__(BPy_UnaryFunction0DVec2f* self);

static PyObject * UnaryFunction0DVec2f_getName( BPy_UnaryFunction0DVec2f *self);
static PyObject * UnaryFunction0DVec2f___call__( BPy_UnaryFunction0DVec2f *self, PyObject *args);

/*----------------------UnaryFunction0DVec2f instance definitions ----------------------------*/
static PyMethodDef BPy_UnaryFunction0DVec2f_methods[] = {
	{"getName", ( PyCFunction ) UnaryFunction0DVec2f_getName, METH_NOARGS, "（ ）Returns the string of the name of the unary 0D function."},
	{"__call__", ( PyCFunction ) UnaryFunction0DVec2f___call__, METH_VARARGS, "（Interface0DIterator it ）Executes the operator ()	on the iterator it pointing onto the point at which we wish to evaluate the function." },
	{NULL, NULL, 0, NULL}
};

/*-----------------------BPy_UnaryFunction0DVec2f type definition ------------------------------*/

PyTypeObject UnaryFunction0DVec2f_Type = {
	PyObject_HEAD_INIT( NULL ) 
	0,							/* ob_size */
	"UnaryFunction0DVec2f",				/* tp_name */
	sizeof( BPy_UnaryFunction0DVec2f ),	/* tp_basicsize */
	0,							/* tp_itemsize */
	
	/* methods */
	(destructor)UnaryFunction0DVec2f___dealloc__,	/* tp_dealloc */
	NULL,                       				/* printfunc tp_print; */
	NULL,                       				/* getattrfunc tp_getattr; */
	NULL,                       				/* setattrfunc tp_setattr; */
	NULL,										/* tp_compare */
	(reprfunc)UnaryFunction0DVec2f___repr__,					/* tp_repr */

	/* Method suites for standard classes */

	NULL,                       /* PyNumberMethods *tp_as_number; */
	NULL,                       /* PySequenceMethods *tp_as_sequence; */
	NULL,                       /* PyMappingMethods *tp_as_mapping; */

	/* More standard operations (here for binary compatibility) */

	NULL,						/* hashfunc tp_hash; */
	NULL,                       /* ternaryfunc tp_call; */
	NULL,                       /* reprfunc tp_str; */
	NULL,                       /* getattrofunc tp_getattro; */
	NULL,                       /* setattrofunc tp_setattro; */

	/* Functions to access object as input/output buffer */
	NULL,                       /* PyBufferProcs *tp_as_buffer; */

  /*** Flags to define presence of optional/expanded features ***/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 		/* long tp_flags; */

	NULL,                       /*  char *tp_doc;  Documentation string */
  /*** Assigned meaning in release 2.0 ***/
	/* call function for all accessible objects */
	NULL,                       /* traverseproc tp_traverse; */

	/* delete references to contained objects */
	NULL,                       /* inquiry tp_clear; */

  /***  Assigned meaning in release 2.1 ***/
  /*** rich comparisons ***/
	NULL,                       /* richcmpfunc tp_richcompare; */

  /***  weak reference enabler ***/
	0,                          /* long tp_weaklistoffset; */

  /*** Added in release 2.2 ***/
	/*   Iterators */
	NULL,                       /* getiterfunc tp_iter; */
	NULL,                       /* iternextfunc tp_iternext; */

  /*** Attribute descriptor and subclassing stuff ***/
	BPy_UnaryFunction0DVec2f_methods,	/* struct PyMethodDef *tp_methods; */
	NULL,                       	/* struct PyMemberDef *tp_members; */
	NULL,         					/* struct PyGetSetDef *tp_getset; */
	&UnaryFunction0D_Type,							/* struct _typeobject *tp_base; */
	NULL,							/* PyObject *tp_dict; */
	NULL,							/* descrgetfunc tp_descr_get; */
	NULL,							/* descrsetfunc tp_descr_set; */
	0,                          	/* long tp_dictoffset; */
	(initproc)UnaryFunction0DVec2f___init__, /* initproc tp_init; */
	NULL,							/* allocfunc tp_alloc; */
	NULL,		/* newfunc tp_new; */
	
	/*  Low-level free-memory routine */
	NULL,                       /* freefunc tp_free;  */
	
	/* For PyObject_IS_GC */
	NULL,                       /* inquiry tp_is_gc;  */
	NULL,                       /* PyObject *tp_bases; */
	
	/* method resolution order */
	NULL,                       /* PyObject *tp_mro;  */
	NULL,                       /* PyObject *tp_cache; */
	NULL,                       /* PyObject *tp_subclasses; */
	NULL,                       /* PyObject *tp_weaklist; */
	NULL
};

//-------------------MODULE INITIALIZATION--------------------------------

PyMODINIT_FUNC UnaryFunction0DVec2f_Init( PyObject *module ) {

	if( module == NULL )
		return;

	if( PyType_Ready( &UnaryFunction0DVec2f_Type ) < 0 )
		return;
	Py_INCREF( &UnaryFunction0DVec2f_Type );
	PyModule_AddObject(module, "UnaryFunction0DVec2f", (PyObject *)&UnaryFunction0DVec2f_Type);
	
	if( PyType_Ready( &Normal2DF0D_Type ) < 0 )
		return;
	Py_INCREF( &Normal2DF0D_Type );
	PyModule_AddObject(module, "Normal2DF0D", (PyObject *)&Normal2DF0D_Type);
	
	if( PyType_Ready( &VertexOrientation2DF0D_Type ) < 0 )
		return;
	Py_INCREF( &VertexOrientation2DF0D_Type );
	PyModule_AddObject(module, "VertexOrientation2DF0D", (PyObject *)&VertexOrientation2DF0D_Type);
		
}

//------------------------INSTANCE METHODS ----------------------------------

int UnaryFunction0DVec2f___init__(BPy_UnaryFunction0DVec2f* self)
{
	self->uf0D_vec2f = new UnaryFunction0D<Vec2f>();
	return 0;
}

void UnaryFunction0DVec2f___dealloc__(BPy_UnaryFunction0DVec2f* self)
{
	delete self->uf0D_vec2f;
	UnaryFunction0D_Type.tp_dealloc((PyObject*)self);
}


PyObject * UnaryFunction0DVec2f___repr__(BPy_UnaryFunction0DVec2f* self)
{
	return PyString_FromFormat("type: %s - address: %p", self->uf0D_vec2f->getName().c_str(), self->uf0D_vec2f );
}

PyObject * UnaryFunction0DVec2f_getName( BPy_UnaryFunction0DVec2f *self )
{
	return PyString_FromString( self->uf0D_vec2f->getName().c_str() );
}

PyObject * UnaryFunction0DVec2f___call__( BPy_UnaryFunction0DVec2f *self, PyObject *args)
{
	PyObject *obj;

	if( !PyArg_ParseTuple(args, "O", &obj) && BPy_Interface0DIterator_Check(obj) ) {
		cout << "ERROR: UnaryFunction0DVec2f___call__ " << endl;		
		return NULL;
	}
	
	Vec2f v( self->uf0D_vec2f->operator()(*( ((BPy_Interface0DIterator *) obj)->if0D_it )) );
	return Vector_from_Vec2f( v );

}

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
