#include "BPy_FrsMaterial.h"

#include "BPy_Convert.h"

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////

/*---------------  Python API function prototypes for FrsMaterial instance  -----------*/
static int FrsMaterial___init__(BPy_FrsMaterial *self, PyObject *args, PyObject *kwds);
static void FrsMaterial___dealloc__(BPy_FrsMaterial *self);
static PyObject * FrsMaterial___repr__(BPy_FrsMaterial *self);

static PyObject * FrsMaterial_diffuse( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_diffuseR( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_diffuseG( BPy_FrsMaterial* self) ;
static PyObject * FrsMaterial_diffuseB( BPy_FrsMaterial* self) ;
static PyObject * FrsMaterial_diffuseA( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_specular( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_specularR( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_specularG( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_specularB( BPy_FrsMaterial* self) ;
static PyObject * FrsMaterial_specularA( BPy_FrsMaterial* self) ;
static PyObject * FrsMaterial_ambient( BPy_FrsMaterial* self) ;
static PyObject * FrsMaterial_ambientR( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_ambientG( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_ambientB( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_ambientA( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_emission( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_emissionR( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_emissionG( BPy_FrsMaterial* self) ;
static PyObject * FrsMaterial_emissionB( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_emissionA( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_shininess( BPy_FrsMaterial* self);
static PyObject * FrsMaterial_setDiffuse( BPy_FrsMaterial *self, PyObject *args );
static PyObject * FrsMaterial_setSpecular( BPy_FrsMaterial *self, PyObject *args );
static PyObject * FrsMaterial_setAmbient( BPy_FrsMaterial *self, PyObject *args );
static PyObject * FrsMaterial_setEmission( BPy_FrsMaterial *self, PyObject *args );
static PyObject * FrsMaterial_setShininess( BPy_FrsMaterial *self, PyObject *args );

/*----------------------FrsMaterial instance definitions ----------------------------*/
static PyMethodDef BPy_FrsMaterial_methods[] = {
	{"diffuse", ( PyCFunction ) FrsMaterial_diffuse, METH_NOARGS, "() Returns the diffuse color as a 4 float array"},
	{"diffuseR", ( PyCFunction ) FrsMaterial_diffuseR, METH_NOARGS, "() Returns the red component of the diffuse color "},
	{"diffuseG", ( PyCFunction ) FrsMaterial_diffuseG, METH_NOARGS, "() Returns the green component of the diffuse color "},
	{"diffuseB", ( PyCFunction ) FrsMaterial_diffuseB, METH_NOARGS, "() Returns the blue component of the diffuse color "},
	{"diffuseA", ( PyCFunction ) FrsMaterial_diffuseA, METH_NOARGS, "() Returns the alpha component of the diffuse color "},
	{"specular", ( PyCFunction ) FrsMaterial_specular, METH_NOARGS, "() Returns the specular color as a 4 float array"},
	{"specularR", ( PyCFunction ) FrsMaterial_specularR, METH_NOARGS, "() Returns the red component of the specular color "},
	{"specularG", ( PyCFunction ) FrsMaterial_specularG, METH_NOARGS, "() Returns the green component of the specular color "},
	{"specularB", ( PyCFunction ) FrsMaterial_specularB, METH_NOARGS, "() Returns the blue component of the specular color "},
	{"specularA", ( PyCFunction ) FrsMaterial_specularA, METH_NOARGS, "() Returns the alpha component of the specular color "},
	{"ambient", ( PyCFunction ) FrsMaterial_ambient, METH_NOARGS, "() Returns the ambient color as a 4 float array"},
	{"ambientR", ( PyCFunction ) FrsMaterial_ambientR, METH_NOARGS, "() Returns the red component of the ambient color "},
	{"ambientG", ( PyCFunction ) FrsMaterial_ambientG, METH_NOARGS, "() Returns the green component of the ambient color "},
	{"ambientB", ( PyCFunction ) FrsMaterial_ambientB, METH_NOARGS, "() Returns the blue component of the ambient color "},
	{"ambientA", ( PyCFunction ) FrsMaterial_ambientA, METH_NOARGS, "() Returns the alpha component of the ambient color "},
	{"emission", ( PyCFunction ) FrsMaterial_emission, METH_NOARGS, "() Returns the emission color as a 4 float array"},
	{"emissionR", ( PyCFunction ) FrsMaterial_emissionR, METH_NOARGS, "() Returns the red component of the emission color "},
	{"emissionG", ( PyCFunction ) FrsMaterial_emissionG, METH_NOARGS, "() Returns the green component of the emission color "},
	{"emissionB", ( PyCFunction ) FrsMaterial_emissionB, METH_NOARGS, "() Returns the blue component of the emission color "},
	{"emissionA", ( PyCFunction ) FrsMaterial_emissionA, METH_NOARGS, "() Returns the alpha component of the emission color "},
	{"shininess", ( PyCFunction ) FrsMaterial_shininess, METH_NOARGS, "() Returns the shininess coefficient "},
	{"setDiffuse", ( PyCFunction ) FrsMaterial_setDiffuse, METH_NOARGS, "(float r, float g, float b, float a) Sets the diffuse color"},
	{"setSpecular", ( PyCFunction ) FrsMaterial_setSpecular, METH_NOARGS, "(float r, float g, float b, float a) Sets the specular color"},
	{"setAmbient", ( PyCFunction ) FrsMaterial_setAmbient, METH_NOARGS, "(float r, float g, float b, float a) Sets the ambient color"},
	{"setEmission", ( PyCFunction ) FrsMaterial_setEmission, METH_NOARGS, "(float r, float g, float b, float a) Sets the emission color"},
	{"setShininess", ( PyCFunction ) FrsMaterial_setShininess, METH_NOARGS, "(float r, float g, float b, float a) Sets the shininess color"},
	{NULL, NULL, 0, NULL}
};

/*-----------------------BPy_FrsMaterial type definition ------------------------------*/

PyTypeObject FrsMaterial_Type = {
	PyObject_HEAD_INIT( NULL ) 
	0,							/* ob_size */
	"FrsMaterial",				/* tp_name */
	sizeof( BPy_FrsMaterial ),	/* tp_basicsize */
	0,							/* tp_itemsize */
	
	/* methods */
	(destructor)FrsMaterial___dealloc__,	/* tp_dealloc */
	NULL,                       				/* printfunc tp_print; */
	NULL,                       				/* getattrfunc tp_getattr; */
	NULL,                       				/* setattrfunc tp_setattr; */
	NULL,										/* tp_compare */
	(reprfunc)FrsMaterial___repr__,					/* tp_repr */

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
	BPy_FrsMaterial_methods,	/* struct PyMethodDef *tp_methods; */
	NULL,                       	/* struct PyMemberDef *tp_members; */
	NULL,         					/* struct PyGetSetDef *tp_getset; */
	NULL,							/* struct _typeobject *tp_base; */
	NULL,							/* PyObject *tp_dict; */
	NULL,							/* descrgetfunc tp_descr_get; */
	NULL,							/* descrsetfunc tp_descr_set; */
	0,                          	/* long tp_dictoffset; */
	(initproc)FrsMaterial___init__, /* initproc tp_init; */
	NULL,							/* allocfunc tp_alloc; */
	PyType_GenericNew,		/* newfunc tp_new; */
	
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
PyMODINIT_FUNC FrsMaterial_Init( PyObject *module )
{
	if( module == NULL )
		return;

	if( PyType_Ready( &FrsMaterial_Type ) < 0 )
		return;

	Py_INCREF( &FrsMaterial_Type );
	PyModule_AddObject(module, "FrsMaterial", (PyObject *)&FrsMaterial_Type);
}

//------------------------INSTANCE METHODS ----------------------------------

static int Vec4(PyObject *obj, float *v)
{
	if (VectorObject_Check(obj) && ((VectorObject *)obj)->size == 4) {
		for (int i = 0; i < 4; i++)
			v[i] = ((VectorObject *)obj)->vec[i];
	} else if( PyList_Check(obj) && PyList_Size(obj) == 4 ) {
		for (int i = 0; i < 4; i++)
			v[i] = PyFloat_AsDouble(PyList_GetItem(obj, i));
	} else if( PyTuple_Check(obj) && PyTuple_Size(obj) == 4 ) {
		for (int i = 0; i < 4; i++)
			v[i] = PyFloat_AsDouble(PyTuple_GetItem(obj, i));
	} else {
		return 0;
	}
	return 1;
}

int FrsMaterial___init__(BPy_FrsMaterial *self, PyObject *args, PyObject *kwds)
{
	PyObject *obj1 = 0, *obj2 = 0, *obj3 = 0, *obj4 = 0;
	float f1[4], f2[4], f3[4], f4[4], f5 = 0.;

    if (! PyArg_ParseTuple(args, "|OOOOf", &obj1, &obj2, &obj3, &obj4, &f5) )
        return -1;

	if( !obj1 ){
		self->m = new FrsMaterial();

	} else if( BPy_FrsMaterial_Check(obj1) && !obj2 ) {
		FrsMaterial *m = ((BPy_FrsMaterial *) obj1)->m;
		if( !m ) {
			PyErr_SetString(PyExc_RuntimeError, "invalid FrsMaterial object");
			return -1;
		}
		self->m = new FrsMaterial( *m );

	} else if( Vec4(obj1, f1) && obj2 && Vec4(obj2, f2) && obj3 && Vec4(obj3, f3) && obj4 && Vec4(obj4, f4) ) {
		self->m = new FrsMaterial(f1, f2, f3, f4, f5);

	} else {
		PyErr_SetString(PyExc_TypeError, "invalid argument(s)");
		return -1;
	}
	self->borrowed = 0;

	return 0;
}

void FrsMaterial___dealloc__( BPy_FrsMaterial* self)
{
	if( self->m && !self->borrowed )
		delete self->m;
    self->ob_type->tp_free((PyObject*)self);
}


PyObject * FrsMaterial___repr__( BPy_FrsMaterial* self)
{
    return PyString_FromFormat("FrsMaterial - address: %p", self->m );
}

PyObject * FrsMaterial_diffuse( BPy_FrsMaterial* self) {
	const float *diffuse = self->m->diffuse();
	PyObject *py_diffuse = PyTuple_New(4);
	
	PyTuple_SetItem( py_diffuse, 0, PyFloat_FromDouble( diffuse[0] ) );
	PyTuple_SetItem( py_diffuse, 1, PyFloat_FromDouble( diffuse[1] ) );
	PyTuple_SetItem( py_diffuse, 2, PyFloat_FromDouble( diffuse[2] ) );
	PyTuple_SetItem( py_diffuse, 3, PyFloat_FromDouble( diffuse[3] ) );
	
	return py_diffuse;
}

PyObject * FrsMaterial_diffuseR( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->diffuseR() );
}

PyObject * FrsMaterial_diffuseG( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->diffuseG() );
}

PyObject * FrsMaterial_diffuseB( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->diffuseB() );
}

PyObject * FrsMaterial_diffuseA( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->diffuseA() );
}

PyObject * FrsMaterial_specular( BPy_FrsMaterial* self) {
	const float *specular = self->m->specular();
	PyObject *py_specular = PyTuple_New(4);
	
	PyTuple_SetItem( py_specular, 0, PyFloat_FromDouble( specular[0] ) );
	PyTuple_SetItem( py_specular, 1, PyFloat_FromDouble( specular[1] ) );
	PyTuple_SetItem( py_specular, 2, PyFloat_FromDouble( specular[2] ) );
	PyTuple_SetItem( py_specular, 3, PyFloat_FromDouble( specular[3] ) );
	
	return py_specular;
}

PyObject * FrsMaterial_specularR( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->specularR() );
}

PyObject * FrsMaterial_specularG( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->specularG() );
}

PyObject * FrsMaterial_specularB( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->specularB() );
}

PyObject * FrsMaterial_specularA( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->specularA() );
}

PyObject * FrsMaterial_ambient( BPy_FrsMaterial* self) {
	const float *ambient = self->m->ambient();
	PyObject *py_ambient = PyTuple_New(4);
	
	PyTuple_SetItem( py_ambient, 0, PyFloat_FromDouble( ambient[0] ) );
	PyTuple_SetItem( py_ambient, 1, PyFloat_FromDouble( ambient[1] ) );
	PyTuple_SetItem( py_ambient, 2, PyFloat_FromDouble( ambient[2] ) );
	PyTuple_SetItem( py_ambient, 3, PyFloat_FromDouble( ambient[3] ) );
	
	return py_ambient;
}

PyObject * FrsMaterial_ambientR( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->ambientR() );
}

PyObject * FrsMaterial_ambientG( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->ambientG() );
}

PyObject * FrsMaterial_ambientB( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->ambientB() );
}

PyObject * FrsMaterial_ambientA( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->ambientA() );
}

PyObject * FrsMaterial_emission( BPy_FrsMaterial* self) {
	const float *emission = self->m->emission();
	PyObject *py_emission = PyTuple_New(4);
	
	PyTuple_SetItem( py_emission, 0, PyFloat_FromDouble( emission[0] ) );
	PyTuple_SetItem( py_emission, 1, PyFloat_FromDouble( emission[1] ) );
	PyTuple_SetItem( py_emission, 2, PyFloat_FromDouble( emission[2] ) );
	PyTuple_SetItem( py_emission, 3, PyFloat_FromDouble( emission[3] ) );
	
	return py_emission;
}

PyObject * FrsMaterial_emissionR( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->emissionR() );
}

PyObject * FrsMaterial_emissionG( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->emissionG() );
}

PyObject * FrsMaterial_emissionB( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->emissionB() );
}

PyObject * FrsMaterial_emissionA( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->emissionA() );
}

PyObject * FrsMaterial_shininess( BPy_FrsMaterial* self) {
	return PyFloat_FromDouble( self->m->shininess() );
}

PyObject * FrsMaterial_setDiffuse( BPy_FrsMaterial *self, PyObject *args ) {
	float f1, f2, f3, f4;

	if(!( PyArg_ParseTuple(args, "ffff", &f1, &f2, &f3, &f4)  ))
		return NULL;

	self->m->setDiffuse(f1, f2, f3, f4);

	Py_RETURN_NONE;
}
 
PyObject * FrsMaterial_setSpecular( BPy_FrsMaterial *self, PyObject *args ) {
	float f1, f2, f3, f4;

	if(!( PyArg_ParseTuple(args, "ffff", &f1, &f2, &f3, &f4)  ))
		return NULL;

	self->m->setSpecular(f1, f2, f3, f4);

	Py_RETURN_NONE;
}

PyObject * FrsMaterial_setAmbient( BPy_FrsMaterial *self, PyObject *args ) {
	float f1, f2, f3, f4;

	if(!( PyArg_ParseTuple(args, "ffff", &f1, &f2, &f3, &f4)  ))
		return NULL;

	self->m->setAmbient(f1, f2, f3, f4);

	Py_RETURN_NONE;
}

PyObject * FrsMaterial_setEmission( BPy_FrsMaterial *self, PyObject *args ) {
	float f1, f2, f3, f4;

	if(!( PyArg_ParseTuple(args, "ffff", &f1, &f2, &f3, &f4)  ))
		return NULL;

	self->m->setEmission(f1, f2, f3, f4);

	Py_RETURN_NONE;
}

PyObject * FrsMaterial_setShininess( BPy_FrsMaterial *self, PyObject *args ) {
	float f;

	if(!( PyArg_ParseTuple(args, "f", &f)  ))
		return NULL;

	self->m->setShininess(f);

	Py_RETURN_NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
