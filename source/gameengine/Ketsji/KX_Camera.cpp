/*
 * $Id$
 *
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 * Camera in the gameengine. Cameras are also used for views.
 */
 
#include "KX_Camera.h"

#include "KX_Python.h"
#include "KX_PyMath.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

KX_Camera::KX_Camera(void* sgReplicationInfo,
					 SG_Callbacks callbacks,
					 const RAS_CameraData& camdata,
					 bool frustum_culling)
					:
					KX_GameObject(sgReplicationInfo,callbacks),
					m_camdata(camdata),
					m_dirty(true),
					m_frustum_culling(frustum_culling),
					m_set_projection_matrix(false)
{
	// setting a name would be nice...
	m_name = "cam";
	m_projection_matrix.setIdentity();
	m_modelview_matrix.setIdentity();
	SetProperty("camera",new CIntValue(1));
}



KX_Camera::~KX_Camera()
{
}	


	
MT_Transform KX_Camera::GetWorldToCamera() const
{ 
	MT_Transform camtrans;
	MT_Transform trans;
	
	trans.setBasis(NodeGetWorldOrientation());
	trans.setOrigin(NodeGetWorldPosition());
	
	camtrans.invert(trans);
	
	return camtrans;
}


	 
MT_Transform KX_Camera::GetCameraToWorld() const
{
	MT_Transform trans;
	trans.setBasis(NodeGetWorldOrientation());
	trans.setOrigin(NodeGetWorldPosition());
	
	return trans;
}



void KX_Camera::CorrectLookUp(MT_Scalar speed)
{
}



const MT_Point3 KX_Camera::GetCameraLocation() const
{
	/* this is the camera locatio in cam coords... */
	//return m_trans1.getOrigin();
	//return MT_Point3(0,0,0);   <-----
	/* .... I want it in world coords */
	//MT_Transform trans;
	//trans.setBasis(NodeGetWorldOrientation());
	
	return NodeGetWorldPosition();		
}



/* I want the camera orientation as well. */
const MT_Quaternion KX_Camera::GetCameraOrientation() const
{
	MT_Transform trans;
	trans.setBasis(NodeGetWorldOrientation());
	trans.setOrigin(NodeGetWorldPosition());

	return trans.getRotation();
}



/**
* Sets the projection matrix that is used by the rasterizer.
*/
void KX_Camera::SetProjectionMatrix(const MT_Matrix4x4 & mat)
{
	m_projection_matrix = mat;
	m_dirty = true;
	m_set_projection_matrix = true;
}



/**
* Sets the modelview matrix that is used by the rasterizer.
*/
void KX_Camera::SetModelviewMatrix(const MT_Matrix4x4 & mat)
{
	m_modelview_matrix = mat;
	m_dirty = true;
}



/**
* Gets the projection matrix that is used by the rasterizer.
*/
const MT_Matrix4x4& KX_Camera::GetProjectionMatrix() const
{
	return m_projection_matrix;
}



/**
* Gets the modelview matrix that is used by the rasterizer.
*/
const MT_Matrix4x4& KX_Camera::GetModelviewMatrix() const
{
	return m_modelview_matrix;
}


bool KX_Camera::hasValidProjectionMatrix() const
{
	return m_set_projection_matrix;
}

/*
* These getters retrieve the clip data and the focal length
*/
float KX_Camera::GetLens() const
{
	return m_camdata.m_lens;
}



float KX_Camera::GetCameraNear() const
{
	return m_camdata.m_clipstart;
}



float KX_Camera::GetCameraFar() const
{
	return m_camdata.m_clipend;
}



RAS_CameraData*	KX_Camera::GetCameraData()
{
	return &m_camdata; 
}

void KX_Camera::ExtractClipPlanes()
{
	MT_Matrix4x4 m = m_projection_matrix * GetWorldToCamera();
	// Left clip plane
	m_planes[0] = m[3] + m[0];
	// Right clip plane
	m_planes[1] = m[3] - m[0];
	// Top clip plane
	m_planes[2] = m[3] - m[1];
	// Bottom clip plane
	m_planes[3] = m[3] + m[1];
	// Near clip plane
	m_planes[4] = m[3] + m[2];
	// Far clip plane
	m_planes[5] = m[3] - m[2];
	
	m_dirty = false;
}

bool KX_Camera::PointInsideFrustum(const MT_Point3& x)
{
	if (m_dirty)
		ExtractClipPlanes();
	
	for( unsigned int i = 0; i < 6 ; i++ )
	{
		if (m_planes[i][0]*x[0] + m_planes[i][1]*x[1] + m_planes[i][2]*x[2] + m_planes[i][3] < 0.)
			return false;
	}
	return true;
}

int KX_Camera::BoxInsideFrustum(const MT_Point3 *box)
{
	if (m_dirty)
		ExtractClipPlanes();
	
	unsigned int insideCount = 0;
	for( unsigned int p = 0; p < 6 ; p++ )
	{
		unsigned int behindCount = 0;
		for (unsigned int v = 0; v < 8 ; v++)
		{
			if (m_planes[p][0]*box[v][0] + m_planes[p][1]*box[v][1] + m_planes[p][2]*box[v][2] + m_planes[p][3] < 0.)
				behindCount++;
		}
		
		if (behindCount == 8)
			return OUTSIDE;

		if (!behindCount)
			insideCount++;
	}
	
	if (insideCount == 6)
		return INSIDE;
	
	return INTERSECT;
}

int KX_Camera::SphereInsideFrustum(const MT_Point3& centre, const MT_Scalar &radius)
{
	MT_Scalar distance;
	for (unsigned int p = 0; p < 6; p++)
	{
		distance = m_planes[p][0]*centre[0] + m_planes[p][1]*centre[1] + m_planes[p][2]*centre[2] + m_planes[p][3];
		if (distance < -radius)
			return OUTSIDE;
		if (fabs(distance) < radius)
			return INTERSECT;
	}
	return INSIDE;
}

bool KX_Camera::GetFrustumCulling() const
{
	return m_frustum_culling;
}

//----------------------------------------------------------------------------
//Python


PyMethodDef KX_Camera::Methods[] = {
	KX_PYMETHODTABLE(KX_Camera, sphereInsideFrustum),
	KX_PYMETHODTABLE(KX_Camera, boxInsideFrustum),
	KX_PYMETHODTABLE(KX_Camera, pointInsideFrustum),
	KX_PYMETHODTABLE(KX_Camera, getCameraToWorld),
	KX_PYMETHODTABLE(KX_Camera, getWorldToCamera),
	KX_PYMETHODTABLE(KX_Camera, getProjectionMatrix),
	KX_PYMETHODTABLE(KX_Camera, setProjectionMatrix),
	
	{NULL,NULL} //Sentinel
};

char KX_Camera::doc[] = "Module KX_Camera\n\n"
"Constants:\n"
"\tINSIDE\n"
"\tINTERSECT\n"
"\tOUTSIDE\n"
"Attributes:\n"
"\tlens -> float\n"
"\t\tThe camera's lens value\n"
"\tnear -> float\n"
"\t\tThe camera's near clip distance\n"
"\tfar -> float\n"
"\t\tThe camera's far clip distance\n"
"\tfrustum_culling -> bool\n"
"\t\tNon zero if this camera is frustum culling.\n"
"\tprojection_matrix -> [[float]]\n"
"\t\tThis camera's projection matrix.\n"
"\tmodelview_matrix -> [[float]] (read only)\n"
"\t\tThis camera's model view matrix.\n"
"\t\tRegenerated every frame from the camera's position and orientation.\n"
"\tcamera_to_world -> [[float]] (read only)\n"
"\t\tThis camera's camera to world transform.\n"
"\t\tRegenerated every frame from the camera's position and orientation.\n"
"\tworld_to_camera -> [[float]] (read only)\n"
"\t\tThis camera's world to camera transform.\n"
"\t\tRegenerated every frame from the camera's position and orientation.\n"
"\t\tThis is camera_to_world inverted.\n";

PyTypeObject KX_Camera::Type = {
	PyObject_HEAD_INIT(&PyType_Type)
		0,
		"KX_Camera",
		sizeof(KX_Camera),
		0,
		PyDestructor,
		0,
		__getattr,
		__setattr,
		0, //&MyPyCompare,
		__repr,
		0, //&cvalue_as_number,
		0,
		0,
		0,
		0, 0, 0, 0, 0, 0,
		doc
};

PyParentObject KX_Camera::Parents[] = {
	&KX_Camera::Type,
	&KX_GameObject::Type,
		&SCA_IObject::Type,
		&CValue::Type,
		NULL
};

PyObject* KX_Camera::_getattr(const STR_String& attr)
{
	if (attr == "INSIDE")
		return PyInt_FromLong(INSIDE);
	if (attr == "OUTSIDE")
		return PyInt_FromLong(OUTSIDE);
	if (attr == "INTERSECT")
		return PyInt_FromLong(INTERSECT);
	
	if (attr == "lens")
		return PyFloat_FromDouble(GetLens());
	if (attr == "near")
		return PyFloat_FromDouble(GetCameraNear());
	if (attr == "far")
		return PyFloat_FromDouble(GetCameraFar());
	if (attr == "frustum_culling")
		return PyInt_FromLong(m_frustum_culling);
	if (attr == "projection_matrix")
		return PyObjectFromMT_Matrix4x4(GetProjectionMatrix());
	if (attr == "modelview_matrix")
		return PyObjectFromMT_Matrix4x4(GetModelviewMatrix());
	if (attr == "camera_to_world")
		return PyObjectFromMT_Matrix4x4(GetCameraToWorld());
	if (attr == "world_to_camera")
		return PyObjectFromMT_Matrix4x4(GetWorldToCamera());
	
	_getattr_up(KX_GameObject);
}

int KX_Camera::_setattr(const STR_String &attr, PyObject *pyvalue)
{
	if (PyInt_Check(pyvalue))
	{
		if (attr == "frustum_culling")
		{
			m_frustum_culling = PyInt_AsLong(pyvalue);
			return 0;
		}
	}
	
	if (PyFloat_Check(pyvalue))
	{
		if (attr == "lens")
		{
			m_camdata.m_lens = PyFloat_AsDouble(pyvalue);
			m_set_projection_matrix = false;
			return 0;
		}
		if (attr == "near")
		{
			m_camdata.m_clipstart = PyFloat_AsDouble(pyvalue);
			m_set_projection_matrix = false;
			return 0;
		}
		if (attr == "far")
		{
			m_camdata.m_clipend = PyFloat_AsDouble(pyvalue);
			m_set_projection_matrix = false;
			return 0;
		}
	}
	
	if (attr == "projection_matrix")
	{
		PysetProjectionMatrix((PyObject*) this, pyvalue, NULL);
		return 0;
	}
	
	return KX_GameObject::_setattr(attr, pyvalue);
}

KX_PYMETHODDEF_DOC(KX_Camera, sphereInsideFrustum,
"sphereInsideFrustum(centre, radius) -> Integer\n"
"\treturns INSIDE, OUTSIDE or INTERSECT if the given sphere is\n"
"\tinside/outside/intersects this camera's viewing frustum.\n\n"
"\tcentre = the centre of the sphere (in world coordinates.)\n"
"\tradius = the radius of the sphere\n\n"
"\tExample:\n"
"\timport GameLogic\n\n"
"\tco = GameLogic.getCurrentController()\n"
"\tcam = co.GetOwner()\n\n"
"\t# A sphere of radius 4.0 located at [x, y, z] = [1.0, 1.0, 1.0]\n"
"\tif (cam.sphereInsideFrustum([1.0, 1.0, 1.0], 4) != cam.OUTSIDE):\n"
"\t\t# Sphere is inside frustum !\n"
"\t\t# Do something useful !\n"
"\telse:\n"
"\t\t# Sphere is outside frustum\n"
)
{
	PyObject *pycentre;
	float radius;
	if (PyArg_ParseTuple(args, "Of", &pycentre, &radius))
	{
		MT_Point3 centre = MT_Point3FromPyList(pycentre);
		if (PyErr_Occurred())
		{
			PyErr_SetString(PyExc_TypeError, "Expected list for argument centre.");
			return Py_None;
		}
		
		return PyInt_FromLong(SphereInsideFrustum(centre, radius));
	}

	PyErr_SetString(PyExc_TypeError, "Expected arguments: (centre, radius)");
	
	return Py_None;
}

KX_PYMETHODDEF_DOC(KX_Camera, boxInsideFrustum,
"boxInsideFrustum(box) -> Integer\n"
"\treturns INSIDE, OUTSIDE or INTERSECT if the given box is\n"
"\tinside/outside/intersects this camera's viewing frustum.\n\n"
"\tbox = a list of the eight (8) corners of the box (in world coordinates.)\n\n"
"\tExample:\n"
"\timport GameLogic\n\n"
"\tco = GameLogic.getCurrentController()\n"
"\tcam = co.GetOwner()\n\n"
"\tbox = []\n"
"\tbox.append([-1.0, -1.0, -1.0])\n"
"\tbox.append([-1.0, -1.0,  1.0])\n"
"\tbox.append([-1.0,  1.0, -1.0])\n"
"\tbox.append([-1.0,  1.0,  1.0])\n"
"\tbox.append([ 1.0, -1.0, -1.0])\n"
"\tbox.append([ 1.0, -1.0,  1.0])\n"
"\tbox.append([ 1.0,  1.0, -1.0])\n"
"\tbox.append([ 1.0,  1.0,  1.0])\n\n"
"\tif (cam.boxInsideFrustum(box) != cam.OUTSIDE):\n"
"\t\t# Box is inside/intersects frustum !\n"
"\t\t# Do something useful !\n"
"\telse:\n"
"\t\t# Box is outside the frustum !\n"
)
{
	PyObject *pybox;
	if (PyArg_ParseTuple(args, "O", &pybox))
	{
		unsigned int num_points = PySequence_Size(pybox);
		if (num_points != 8)
		{
			PyErr_Format(PyExc_TypeError, "Expected eight (8) points, got %d", num_points);
			return Py_None;
		}
		
		MT_Point3 box[8];
		for (unsigned int p = 0; p < 8 ; p++)
		{
			box[p] = MT_Point3FromPyList(PySequence_GetItem(pybox, p));
			if (PyErr_Occurred())
			{
				return Py_None;
			}
		}
		
		return PyInt_FromLong(BoxInsideFrustum(box));
	}
	
	PyErr_SetString(PyExc_TypeError, "Expected argument: list of points.");
	
	return Py_None;
}

KX_PYMETHODDEF_DOC(KX_Camera, pointInsideFrustum,
"pointInsideFrustum(point) -> Bool\n"
"\treturns 1 if the given point is inside this camera's viewing frustum.\n\n"
"\tpoint = The point to test (in world coordinates.)\n\n"
"\tExample:\n"
"\timport GameLogic\n\n"
"\tco = GameLogic.getCurrentController()\n"
"\tcam = co.GetOwner()\n\n"
"\t# Test point [0.0, 0.0, 0.0]"
"\tif (cam.pointInsideFrustum([0.0, 0.0, 0.0])):\n"
"\t\t# Point is inside frustum !\n"
"\t\t# Do something useful !\n"
"\telse:\n"
"\t\t# Box is outside the frustum !\n"
)
{
	PyObject *pypoint;
	if (PyArg_ParseTuple(args, "O", &pypoint))
	{
		MT_Point3 point = MT_Point3FromPyList(pypoint);
		if (PyErr_Occurred())
			return Py_None;
			
		return PyInt_FromLong(PointInsideFrustum(point));
	}
	
	PyErr_SetString(PyExc_TypeError, "Expected point argument.");
	return Py_None;
}

KX_PYMETHODDEF_DOC(KX_Camera, getCameraToWorld,
"getCameraToWorld() -> Matrix4x4\n"
"\treturns the camera to world transformation matrix, as a list of four lists of four values.\n\n"
"\tie: [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]])\n"
)
{
	return PyObjectFromMT_Matrix4x4(GetCameraToWorld());
}

KX_PYMETHODDEF_DOC(KX_Camera, getWorldToCamera,
"getWorldToCamera() -> Matrix4x4\n"
"\treturns the world to camera transformation matrix, as a list of four lists of four values.\n\n"
"\tie: [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]])\n"
)
{
	return PyObjectFromMT_Matrix4x4(GetWorldToCamera());
}

KX_PYMETHODDEF_DOC(KX_Camera, getProjectionMatrix,
"getProjectionMatrix() -> Matrix4x4\n"
"\treturns this camera's projection matrix, as a list of four lists of four values.\n\n"
"\tie: [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]])\n"
)
{
	return PyObjectFromMT_Matrix4x4(GetProjectionMatrix());
}

KX_PYMETHODDEF_DOC(KX_Camera, setProjectionMatrix,
"setProjectionMatrix(MT_Matrix4x4 m) -> None\n"
"\tSets this camera's projection matrix\n"
"\n"
"\tExample:\n"
"\timport GameLogic\n"
"\t# Set a perspective projection matrix\n"
"\tdef Perspective(left, right, bottom, top, near, far):\n"
"\t\tm = MT_Matrix4x4()\n"
"\t\tm[0][0] = m[0][2] = right - left\n"
"\t\tm[1][1] = m[1][2] = top - bottom\n"
"\t\tm[2][2] = m[2][3] = -far - near\n"
"\t\tm[3][2] = -1\n"
"\t\tm[3][3] = 0\n"
"\t\treturn m\n"
"\n"
"\t# Set an orthographic projection matrix\n"
"\tdef Orthographic(left, right, bottom, top, near, far):\n"
"\t\tm = MT_Matrix4x4()\n"
"\t\tm[0][0] = right - left\n"
"\t\tm[0][3] = -right - left\n"
"\t\tm[1][1] = top - bottom\n"
"\t\tm[1][3] = -top - bottom\n"
"\t\tm[2][2] = far - near\n"
"\t\tm[2][3] = -far - near\n"
"\t\tm[3][3] = 1\n"
"\t\treturn m\n"
"\n"
"\t# Set an isometric projection matrix\n"
"\tdef Isometric(left, right, bottom, top, near, far):\n"
"\t\tm = MT_Matrix4x4()\n"
"\t\tm[0][0] = m[0][2] = m[1][1] = 0.8660254037844386\n"
"\t\tm[1][0] = 0.25\n"
"\t\tm[1][2] = -0.25\n"
"\t\tm[3][3] = 1\n"
"\t\treturn m\n"
"\n"
"\t"
"\tco = GameLogic.getCurrentController()\n"
"\tcam = co.getOwner()\n"
"\tcam.setProjectionMatrix(Perspective(-1.0, 1.0, -1.0, 1.0, 0.1, 1))\n")
{
	PyObject *pymat;
	if (PyArg_ParseTuple(args, "O", &pymat))
	{
		MT_Matrix4x4 mat = MT_Matrix4x4FromPyObject(pymat);
		if (PyErr_Occurred())
		{
			return Py_None;
		}
		
		SetProjectionMatrix(mat);
		return Py_None;
	}

	PyErr_SetString(PyExc_TypeError, "Expected 4x4 list as matrix argument.");
	return Py_None;
}
