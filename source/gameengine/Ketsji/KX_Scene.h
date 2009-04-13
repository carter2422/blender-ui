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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */
#ifndef __KX_SCENE_H
#define __KX_SCENE_H


#include "KX_PhysicsEngineEnums.h"

#include "MT_CmMatrix4x4.h"

#include <vector>
#include <set>
#include <list>

#include "GEN_Map.h"
#include "GEN_HashedPtr.h"
#include "SG_IObject.h"
#include "SCA_IScene.h"
#include "MT_Transform.h"
#include "SND_Scene.h"
#include "RAS_FramingManager.h"
#include "RAS_Rect.h"

#include "PyObjectPlus.h"

/**
 * @section Forward declarations
 */
struct SM_MaterialProps;
struct SM_ShapeProps;
struct Scene;

class GEN_HashedPtr;
class CListValue;
class CValue;
class SCA_LogicManager;
class SCA_KeyboardManager;
class SCA_TimeEventManager;
class SCA_MouseManager;
class SCA_ISystem;
class SCA_IInputDevice;
class SND_Scene;
class SND_IAudioDevice;
class NG_NetworkDeviceInterface;
class NG_NetworkScene;
class SG_IObject;
class SG_Node;
class SG_Tree;
class KX_WorldInfo;
class KX_Camera;
class KX_GameObject;
class KX_LightObject;
class RAS_BucketManager;
class RAS_BucketManager;
class RAS_MaterialBucket;
class RAS_IPolyMaterial;
class RAS_IRasterizer;
class RAS_IRenderTools;
class SCA_JoystickManager;
class btCollisionShape;
class KX_BlenderSceneConverter;
struct KX_ClientObjectInfo;

/**
 * The KX_Scene holds all data for an independent scene. It relates
 * KX_Objects to the specific objects in the modules.
 * */
class KX_Scene : public PyObjectPlus, public SCA_IScene
{
	Py_Header;

	struct CullingInfo {
		int m_layer;
		CullingInfo(int layer) : m_layer(layer) {}
	};

protected:
	RAS_BucketManager*	m_bucketmanager;
	CListValue*			m_tempObjectList;

	/**
	 * The list of objects which have been removed during the
	 * course of one frame. They are actually destroyed in 
	 * LogicEndFrame() via a call to RemoveObject().
	 */
	CListValue*	m_euthanasyobjects;
	/**
	* The list of objects that couldn't be released during logic update.
	* for example, AddObject actuator sometimes releases an object that was cached from previous frame
	*/
	CListValue*	m_delayReleaseObjects;

	CListValue*			m_objectlist;
	CListValue*			m_parentlist; // all 'root' parents
	CListValue*			m_lightlist;
	CListValue*			m_inactivelist;	// all objects that are not in the active layer

	/**
	 *  The tree of objects in the scene.
	 */
	SG_Tree*			m_objecttree;

	/**
	 * The set of cameras for this scene
	 */
	list<class KX_Camera*>       m_cameras;
	/**
	 * Various SCA managers used by the scene
	 */
	SCA_LogicManager*		m_logicmgr;
	SCA_KeyboardManager*	m_keyboardmgr;
	SCA_MouseManager*		m_mousemgr;
	SCA_TimeEventManager*	m_timemgr;

	// Scene converter where many scene entities are registered
	// Used to deregister objects that are deleted
	class KX_BlenderSceneConverter*		m_sceneConverter;
	/**
	* physics engine abstraction
	*/
	//e_PhysicsEngine m_physicsEngine; //who needs this ?
	class PHY_IPhysicsEnvironment*		m_physicsEnvironment;

	/**
	 * Does this scene clear the z-buffer?
	 */
	bool m_isclearingZbuffer;

	/**
	 * The name of the scene
	 */
	STR_String	m_sceneName;
	
	/**
	 * stores the worldsettings for a scene
	 */
	KX_WorldInfo* m_worldinfo;

	/**
	 * @section Different scenes, linked to ketsji scene
	 */


	/**
	 * Sound scenes
	 */
	SND_Scene* m_soundScene;
	SND_IAudioDevice* m_adi;

	/** 
	 * Network scene.
	 */
	NG_NetworkDeviceInterface*	m_networkDeviceInterface;
	NG_NetworkScene* m_networkScene;

	/**
	 * A temoprary variable used to parent objects together on
	 * replication. Don't get confused by the name it is not
	 * the scene's root node!
	 */
	SG_Node* m_rootnode;

	/**
	 * The active camera for the scene
	 */
	KX_Camera* m_active_camera;

	/** 
	 * The projection and view matrices of this scene 
	 * The projection matrix is computed externally by KX_Engine	
	 * The view mat is stored as a side effect of GetViewMatrix()
	 * and is totally unnessary.
	 */
	MT_CmMatrix4x4				m_projectionmat;
	MT_CmMatrix4x4				m_viewmat;

	/** Desired canvas width set at design time. */
	unsigned int m_canvasDesignWidth;
	/** Desired canvas height set at design time. */
	unsigned int m_canvasDesignHeight;

	/**
	 * Another temporary variable outstaying its welcome
	 * used in AddReplicaObject to map game objects to their
	 * replicas so pointers can be updated.
	 */
	GEN_Map	<GEN_HashedPtr, void*> m_map_gameobject_to_replica;

	/**
	 * Another temporary variable outstaying its welcome
	 * used in AddReplicaObject to keep a record of all added 
	 * objects. Logic can only be updated when all objects 
	 * have been updated. This stores a list of the new objects.
	 */
	std::vector<KX_GameObject*>	m_logicHierarchicalGameObjects;
	
	/**
	 * This temporary variable will contain the list of 
	 * object that can be added during group instantiation.
	 * objects outside this list will not be added (can 
	 * happen with children that are outside the group).
	 * Used in AddReplicaObject. If the list is empty, it
	 * means don't care.
	 */
	std::set<CValue*>	m_groupGameObjects;
	
	/** 
	 * Pointer to system variable passed in in constructor
	 * only used in constructor so we do not need to keep it
	 * around in this class.
	 */

	SCA_ISystem* m_kxsystem;

	/**
	 * The execution priority of replicated object actuators?
	 */	
	int	m_ueberExecutionPriority;

	/**
	 * Activity 'bubble' settings :
	 * Suspend (freeze) the entire scene.
	 */
	bool m_suspend;

	/**
	 * Radius in Manhattan distance of the box for activity culling.
	 */
	float m_activity_box_radius;

	/**
	 * Toggle to enable or disable activity culling.
	 */
	bool m_activity_culling;
	
	/**
	 * Toggle to enable or disable culling via DBVT broadphase of Bullet.
	 */
	bool m_dbvt_culling;
	
	/**
	 * Occlusion culling resolution
	 */ 
	int m_dbvt_occlusion_res;

	/**
	 * The framing settings used by this scene
	 */

	RAS_FrameSettings m_frame_settings;

	/** 
	 * This scenes viewport into the game engine
	 * canvas.Maintained externally, initially [0,0] -> [0,0]
	 */
	RAS_Rect m_viewport;
	
	/**
	 * Visibility testing functions.
	 */
	void MarkVisible(SG_Tree *node, RAS_IRasterizer* rasty, KX_Camera*cam,int layer=0);
	void MarkSubTreeVisible(SG_Tree *node, RAS_IRasterizer* rasty, bool visible, KX_Camera*cam,int layer=0);
	void MarkVisible(RAS_IRasterizer* rasty, KX_GameObject* gameobj, KX_Camera*cam, int layer=0);
	static void PhysicsCullingCallback(KX_ClientObjectInfo* objectInfo, void* cullingInfo);

	double				m_suspendedtime;
	double				m_suspendeddelta;
	
	/**
	 * This stores anything from python
	 */
	PyObject* m_attrlist;

	struct Scene* m_blenderScene;

public:
	KX_Scene(class SCA_IInputDevice* keyboarddevice,
		class SCA_IInputDevice* mousedevice,
		class NG_NetworkDeviceInterface* ndi,
		class SND_IAudioDevice* adi,
		const STR_String& scenename,
		struct Scene* scene);

	virtual	
	~KX_Scene();

	RAS_BucketManager* GetBucketManager();
	RAS_MaterialBucket*	FindBucket(RAS_IPolyMaterial* polymat, bool &bucketCreated);
	void RenderBuckets(const MT_Transform& cameratransform,
						RAS_IRasterizer* rasty,
						RAS_IRenderTools* rendertools);
	/**
	 * Update all transforms according to the scenegraph.
	 */
	void UpdateParents(double curtime);
	void DupliGroupRecurse(CValue* gameobj, int level);
	bool IsObjectInGroup(CValue* gameobj)
	{ 
		return (m_groupGameObjects.empty() || 
				m_groupGameObjects.find(gameobj) != m_groupGameObjects.end());
	}
	SCA_IObject* AddReplicaObject(CValue* gameobj,
								  CValue* locationobj,
								  int lifespan=0);
	KX_GameObject* AddNodeReplicaObject(SG_IObject* node,
										CValue* gameobj);
	void RemoveNodeDestructObject(SG_IObject* node,
								  CValue* gameobj);
	void RemoveObject(CValue* gameobj);
	void DelayedRemoveObject(CValue* gameobj);
	
	void DelayedReleaseObject(CValue* gameobj);

	int NewRemoveObject(CValue* gameobj);
	void ReplaceMesh(CValue* gameobj,
					 void* meshobj);
	/**
	 * @section Logic stuff
	 * Initiate an update of the logic system.
	 */
	void LogicBeginFrame(double curtime);
	void LogicUpdateFrame(double curtime, bool frame);

		void						
	LogicEndFrame(
	);

		CListValue*				
	GetObjectList(
	);

		CListValue*				
	GetInactiveList(
	);

		CListValue*				
	GetRootParentList(
	);

		CListValue*				
	GetLightList(
	);

		SCA_LogicManager*		
	GetLogicManager(
	);

		SCA_TimeEventManager*	
	GetTimeEventManager(
	);

		list<class KX_Camera*>*
	GetCameras(
	);
 

	/** Find a camera in the scene by pointer. */
		KX_Camera*              
	FindCamera(
		KX_Camera*
	);

	/** Find a scene in the scene by name. */
		KX_Camera*              
	FindCamera(
		STR_String&
	);

	/** Add a camera to this scene. */
		void                    
	AddCamera(
		KX_Camera*
	);

	/** Find the currently active camera. */
		KX_Camera*				
	GetActiveCamera(
	);

	/** 
	 * Set this camera to be the active camera in the scene. If the
	 * camera is not present in the camera list, it will be added
	 */

		void					
	SetActiveCamera(
		class KX_Camera*
	);

	/**
	 * Move this camera to the end of the list so that it is rendered last.
	 * If the camera is not on the list, it will be added
	 */
		void
	SetCameraOnTop(
		class KX_Camera*
	);

	/** Return the viewmatrix as used by the last frame. */
		MT_CmMatrix4x4&			
	GetViewMatrix(
	);

	/** 
	 * Return the projectionmatrix as used by the last frame. This is
	 * set by hand :) 
	 */
		MT_CmMatrix4x4&			
	GetProjectionMatrix(
	);

	/** Sets the projection matrix. */
		void					
	SetProjectionMatrix(
		MT_CmMatrix4x4& pmat
	);

	/**
	 * Activates new desired canvas width set at design time.
	 * @param width	The new desired width.
	 */
		void					
	SetCanvasDesignWidth(
		unsigned int width
	);
	/**
	 * Activates new desired canvas height set at design time.
	 * @param width	The new desired height.
	 */
		void					
	SetCanvasDesignHeight(
		unsigned int height
	);
	/**
	 * Returns the current desired canvas width set at design time.
	 * @return The desired width.
	 */
		unsigned int			
	GetCanvasDesignWidth(
		void
	) const;

	/**
	 * Returns the current desired canvas height set at design time.
	 * @return The desired height.
	 */
		unsigned int			
	GetCanvasDesignHeight(
		void
	) const;

	/**
	 * Set the framing options for this scene
	 */

		void
	SetFramingType(
		RAS_FrameSettings & frame_settings
	);

	/**
	 * Return a const reference to the framing 
	 * type set by the above call.
	 * The contents are not guarenteed to be sensible
	 * if you don't call the above function.
	 */

	const
		RAS_FrameSettings &
	GetFramingType(
	) const;

	/**
	 * Store the current scene's viewport on the 
	 * game engine canvas.
	 */
	void SetSceneViewport(const RAS_Rect &viewport);

	/**
	 * Get the current scene's viewport on the
	 * game engine canvas. This maintained 
	 * externally in KX_GameEngine
	 */
	const RAS_Rect& GetSceneViewport() const;
	
	/**
	 * @section Accessors to different scenes of this scene
	 */
	void SetNetworkDeviceInterface(NG_NetworkDeviceInterface* newInterface);
	void SetNetworkScene(NG_NetworkScene *newScene);
	void SetWorldInfo(class KX_WorldInfo* wi);
	KX_WorldInfo* GetWorldInfo();
	void CalculateVisibleMeshes(RAS_IRasterizer* rasty, KX_Camera *cam, int layer=0);
	void UpdateMeshTransformations();
	KX_Camera* GetpCamera();
	SND_Scene* GetSoundScene();
	NG_NetworkDeviceInterface* GetNetworkDeviceInterface();
	NG_NetworkScene* GetNetworkScene();

	/**
	 * Replicate the logic bricks associated to this object.
	 */

	void ReplicateLogic(class KX_GameObject* newobj);
	static SG_Callbacks	m_callbacks;

	const STR_String& GetName();
	
	// Suspend the entire scene.
	void Suspend();

	// Resume a suspended scene.
	void Resume();
	
	// Update the activity box settings for objects in this scene, if needed.
	void UpdateObjectActivity(void);

	// Enable/disable activity culling.
	void SetActivityCulling(bool b);

	// Set the radius of the activity culling box.
	void SetActivityCullingRadius(float f);
	bool IsSuspended();
	bool IsClearingZBuffer();
	void EnableZBufferClearing(bool isclearingZbuffer);
	// use of DBVT tree for camera culling
	void SetDbvtCulling(bool b) { m_dbvt_culling = b; };
	bool GetDbvtCulling() { return m_dbvt_culling; };
	void SetDbvtOcclusionRes(int i) { m_dbvt_occlusion_res = i; };
	int GetDbvtOcclusionRes() { return m_dbvt_occlusion_res; };
	
	void SetSceneConverter(class KX_BlenderSceneConverter* sceneConverter);

	class PHY_IPhysicsEnvironment*		GetPhysicsEnvironment()
	{
		return m_physicsEnvironment;
	}

	void SetPhysicsEnvironment(class PHY_IPhysicsEnvironment*	physEnv);

	void	SetGravity(const MT_Vector3& gravity);
	
	/**
	 * Sets the node tree for this scene.
	 */
	void SetNodeTree(SG_Tree* root);

	KX_PYMETHOD_DOC_NOARGS(KX_Scene, getLightList);
	KX_PYMETHOD_DOC_NOARGS(KX_Scene, getObjectList);
	KX_PYMETHOD_DOC_NOARGS(KX_Scene, getName);
	KX_PYMETHOD_DOC(KX_Scene, addObject);
/*	
	KX_PYMETHOD_DOC(KX_Scene, getActiveCamera);
	KX_PYMETHOD_DOC(KX_Scene, getActiveCamera);
	KX_PYMETHOD_DOC(KX_Scene, findCamera);
	
	KX_PYMETHOD_DOC(KX_Scene, getGravity);
	
	KX_PYMETHOD_DOC(KX_Scene, setActivityCulling);
	KX_PYMETHOD_DOC(KX_Scene, setActivityCullingRadius);
	
	KX_PYMETHOD_DOC(KX_Scene, setSceneViewport);
	KX_PYMETHOD_DOC(KX_Scene, setSceneViewport);
	*/

	/* attributes */
	static PyObject*	pyattr_get_name(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_objects(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	static PyObject*	pyattr_get_active_camera(void* self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	
	/* for dir(), python3 uses __dir__() */
	static PyObject*	pyattr_get_dir_dict(void *self_v, const KX_PYATTRIBUTE_DEF *attrdef);
	
	static int py_base_setattro_scene(PyObject * self, PyObject *attr, PyObject *value)
	{
		if (value==NULL)
			return ((PyObjectPlus*) self)->py_delattro(attr);
		
		int ret= ((PyObjectPlus*) self)->py_setattro(attr, value);
		
		if (ret==PY_SET_ATTR_MISSING) {
			if (PyDict_SetItem(((KX_Scene *) self)->m_attrlist, attr, value)==0) {
				PyErr_Clear();
				ret= PY_SET_ATTR_SUCCESS;
			}
			else {
				PyErr_Format(PyExc_AttributeError, "failed assigning value to KX_Scenes internal dictionary");
				ret= PY_SET_ATTR_FAIL;
			}
		}
		
		return ret;
	}
	
	

	virtual PyObject* py_getattro(PyObject *attr); /* name, active_camera, gravity, suspended, viewport, framing, activity_culling, activity_culling_radius */
	virtual int py_setattro(PyObject *attr, PyObject *pyvalue);
	virtual int py_delattro(PyObject *attr);
	virtual PyObject* py_repr(void) { return PyString_FromString(GetName().ReadPtr()); }

		
	/**
	 * Sets the time the scene was suspended
	 */ 
	void setSuspendedTime(double suspendedtime);
	/**
	 * Returns the "curtime" the scene was suspended
	 */ 
	double getSuspendedTime();
	/**
	 * Sets the difference between the local time of the scene (when it
	 * was running and not suspended) and the "curtime"
	 */ 
	void setSuspendedDelta(double suspendeddelta);
	/**
	 * Returns the difference between the local time of the scene (when it
	 * was running and not suspended) and the "curtime"
	 */
	double getSuspendedDelta();
	/**
	 * Returns the Blender scene this was made from
	 */
	struct Scene *GetBlenderScene() { return m_blenderScene; }
};

typedef std::vector<KX_Scene*> KX_SceneList;

#endif //__KX_SCENE_H

