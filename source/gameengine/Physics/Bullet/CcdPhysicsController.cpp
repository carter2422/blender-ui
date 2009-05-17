/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "CcdPhysicsController.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"

#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"

#include "PHY_IMotionState.h"
#include "CcdPhysicsEnvironment.h"
#include "RAS_MeshObject.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody//btSoftBodyInternals.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "LinearMath/btConvexHull.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"


#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

extern "C"{
#include "BKE_cdderivedmesh.h"
}

class BP_Proxy;

///todo: fill all the empty CcdPhysicsController methods, hook them up to the btRigidBody class

//'temporarily' global variables
//float	gDeactivationTime = 2.f;
//bool	gDisableDeactivation = false;
extern float gDeactivationTime;
extern bool gDisableDeactivation;


float gLinearSleepingTreshold = 0.8f;
float gAngularSleepingTreshold = 1.0f;


btVector3 startVel(0,0,0);//-10000);

CcdPhysicsController::CcdPhysicsController (const CcdConstructionInfo& ci)
:m_cci(ci)
{
	m_prototypeTransformInitialized = false;
	m_softbodyMappingDone = false;
	m_collisionDelay = 0;
	m_newClientInfo = 0;
	m_registerCount = 0;
	m_softBodyTransformInitialized = false;
	m_parentCtrl = 0;
	// copy pointers locally to allow smart release
	m_MotionState = ci.m_MotionState;
	m_collisionShape = ci.m_collisionShape;
	// apply scaling before creating rigid body
	m_collisionShape->setLocalScaling(m_cci.m_scaling);
	if (m_cci.m_mass)
		m_collisionShape->calculateLocalInertia(m_cci.m_mass, m_cci.m_localInertiaTensor);
	// shape info is shared, increment ref count
	m_shapeInfo = ci.m_shapeInfo;
	if (m_shapeInfo)
		m_shapeInfo->AddRef();
	
	m_bulletMotionState = 0;
	
	
	CreateRigidbody();
	

///???
/*#ifdef WIN32
	if (GetRigidBody() && !GetRigidBody()->isStaticObject())
		GetRigidBody()->setLinearVelocity(startVel);
#endif*/

}

btTransform&	CcdPhysicsController::GetTransformFromMotionState(PHY_IMotionState* motionState)
{
	static btTransform trans;
	btVector3 tmp;
	motionState->getWorldPosition(tmp.m_floats[0], tmp.m_floats[1], tmp.m_floats[2]);
	trans.setOrigin(tmp);

	float ori[12];
	motionState->getWorldOrientation(ori);
	trans.getBasis().setFromOpenGLSubMatrix(ori);
	//btQuaternion orn;
	//motionState->getWorldOrientation(orn[0],orn[1],orn[2],orn[3]);
	//trans.setRotation(orn);
	return trans;

}

class	BlenderBulletMotionState : public btMotionState
{
	PHY_IMotionState*	m_blenderMotionState;

public:

	BlenderBulletMotionState(PHY_IMotionState* bms)
		:m_blenderMotionState(bms)
	{

	}

	void	getWorldTransform(btTransform& worldTrans ) const
	{
		btVector3 pos;
		float ori[12];

		m_blenderMotionState->getWorldPosition(pos.m_floats[0],pos.m_floats[1],pos.m_floats[2]);
		m_blenderMotionState->getWorldOrientation(ori);
		worldTrans.setOrigin(pos);
		worldTrans.getBasis().setFromOpenGLSubMatrix(ori);
	}

	void	setWorldTransform(const btTransform& worldTrans)
	{
		m_blenderMotionState->setWorldPosition(worldTrans.getOrigin().getX(),worldTrans.getOrigin().getY(),worldTrans.getOrigin().getZ());
		btQuaternion rotQuat = worldTrans.getRotation();
		m_blenderMotionState->setWorldOrientation(rotQuat[0],rotQuat[1],rotQuat[2],rotQuat[3]);
		m_blenderMotionState->calculateWorldTransformations();
	}

};


btRigidBody* CcdPhysicsController::GetRigidBody()
{
	return btRigidBody::upcast(m_object);
}
btCollisionObject*	CcdPhysicsController::GetCollisionObject()
{
	return m_object;
}
btSoftBody* CcdPhysicsController::GetSoftBody()
{
	return btSoftBody::upcast(m_object);
}

#include "BulletSoftBody/btSoftBodyHelpers.h"



void CcdPhysicsController::CreateRigidbody()
{

	//btTransform trans = GetTransformFromMotionState(m_MotionState);
	m_bulletMotionState = new BlenderBulletMotionState(m_MotionState);

	///either create a btCollisionObject, btRigidBody or btSoftBody

	//create a collision object

	int shapeType = m_cci.m_collisionShape ? m_cci.m_collisionShape->getShapeType() : 0;

	//disable soft body until first sneak preview is ready
	if (m_cci.m_bSoft && m_cci.m_collisionShape && 
		(shapeType == CONVEX_HULL_SHAPE_PROXYTYPE)|
		(shapeType == TRIANGLE_MESH_SHAPE_PROXYTYPE) |
		(shapeType == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE))
	{
		btRigidBody::btRigidBodyConstructionInfo rbci(m_cci.m_mass,m_bulletMotionState,m_collisionShape,m_cci.m_localInertiaTensor * m_cci.m_inertiaFactor);
		rbci.m_linearDamping = m_cci.m_linearDamping;
		rbci.m_angularDamping = m_cci.m_angularDamping;
		rbci.m_friction = m_cci.m_friction;
		rbci.m_restitution = m_cci.m_restitution;

		
		int nodecount = 0;
		
		int numtriangles = 1;
		
		btVector3 p(0,0,0);// = getOrigin();
		btScalar h = 1.f;
		
		btSoftRigidDynamicsWorld* softDynaWorld = (btSoftRigidDynamicsWorld*)m_cci.m_physicsEnv->getDynamicsWorld();

		PHY__Vector3	grav;
		grav[0] = softDynaWorld->getGravity().getX();
		grav[1] = softDynaWorld->getGravity().getY();
		grav[2] = softDynaWorld->getGravity().getZ();
		softDynaWorld->getWorldInfo().m_gravity.setValue(grav[0],grav[1],grav[2]); //??

	
		//btSoftBody*	psb=btSoftBodyHelpers::CreateRope(sbi,	btVector3(-10,0,i*0.25),btVector3(10,0,i*0.25),	16,1+2);

		btSoftBody* psb  = 0;

		if (m_cci.m_collisionShape->getShapeType() == CONVEX_HULL_SHAPE_PROXYTYPE)
		{
			btConvexHullShape* convexHull = (btConvexHullShape* )m_cci.m_collisionShape;

			//psb = btSoftBodyHelpers::CreateFromConvexHull(sbi,&transformedVertices[0],convexHull->getNumPoints());

			{
				int nvertices = convexHull->getNumPoints();
				const btVector3* vertices = convexHull->getPoints();
				btSoftBodyWorldInfo& worldInfo = softDynaWorld->getWorldInfo();

				HullDesc		hdsc(QF_TRIANGLES,nvertices,vertices);
				HullResult		hres;
				HullLibrary		hlib;/*??*/ 
				hdsc.mMaxVertices=nvertices;
				hlib.CreateConvexHull(hdsc,hres);
				
				psb=new btSoftBody(&worldInfo,(int)hres.mNumOutputVertices,
					&hres.m_OutputVertices[0],0);
				for(int i=0;i<(int)hres.mNumFaces;++i)
				{
					const int idx[]={	hres.m_Indices[i*3+0],
						hres.m_Indices[i*3+1],
						hres.m_Indices[i*3+2]};
					if(idx[0]<idx[1]) psb->appendLink(	idx[0],idx[1]);
					if(idx[1]<idx[2]) psb->appendLink(	idx[1],idx[2]);
					if(idx[2]<idx[0]) psb->appendLink(	idx[2],idx[0]);
					psb->appendFace(idx[0],idx[1],idx[2]);
				}
				
				

				hlib.ReleaseResult(hres);

				
			}






		} else
		{
			
			btSoftBodyWorldInfo& sbi= softDynaWorld->getWorldInfo();

			if (m_cci.m_collisionShape->getShapeType() ==SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
			{
				btScaledBvhTriangleMeshShape* scaledtrimeshshape = (btScaledBvhTriangleMeshShape*) m_cci.m_collisionShape;
				btBvhTriangleMeshShape* trimeshshape = scaledtrimeshshape->getChildShape();

				///only deal with meshes that have 1 sub part/component, for now
				if (trimeshshape->getMeshInterface()->getNumSubParts()==1)
				{
					unsigned char* vertexBase;
					PHY_ScalarType vertexType;
					int numverts;
					int vertexstride;
					unsigned char* indexbase;
					int indexstride;
					int numtris;
					PHY_ScalarType indexType;
					trimeshshape->getMeshInterface()->getLockedVertexIndexBase(&vertexBase,numverts,vertexType,vertexstride,&indexbase,indexstride,numtris,indexType);
					
					psb = btSoftBodyHelpers::CreateFromTriMesh(sbi,(const btScalar*)vertexBase,(const int*)indexbase,numtris);
				}
			} else
			{
				btBvhTriangleMeshShape* trimeshshape = (btBvhTriangleMeshShape*) m_cci.m_collisionShape;
				///only deal with meshes that have 1 sub part/component, for now
				if (trimeshshape->getMeshInterface()->getNumSubParts()==1)
				{
					unsigned char* vertexBase;
					PHY_ScalarType vertexType;
					int numverts;
					int vertexstride;
					unsigned char* indexbase;
					int indexstride;
					int numtris;
					PHY_ScalarType indexType;
					trimeshshape->getMeshInterface()->getLockedVertexIndexBase(&vertexBase,numverts,vertexType,vertexstride,&indexbase,indexstride,numtris,indexType);
					
					psb = btSoftBodyHelpers::CreateFromTriMesh(sbi,(const btScalar*)vertexBase,(const int*)indexbase,numtris);
				}
			

				//psb = btSoftBodyHelpers::CreateFromTriMesh(sbi,&pts[0].getX(),triangles,numtriangles);
			}

		}

	
		
		m_object = psb;

		//psb->m_cfg.collisions	=	btSoftBody::fCollision::SDF_RS;//btSoftBody::fCollision::CL_SS+	btSoftBody::fCollision::CL_RS;
		
		//psb->m_cfg.collisions	=	btSoftBody::fCollision::SDF_RS + btSoftBody::fCollision::VF_SS;//CL_SS;
		
		
		//btSoftBody::Material*	pm=psb->appendMaterial();
		btSoftBody::Material*	pm=psb->m_materials[0];
		pm->m_kLST				=	m_cci.m_soft_linStiff;
		pm->m_kAST				=	m_cci.m_soft_angStiff;
		pm->m_kVST				=	m_cci.m_soft_volume;
		psb->m_cfg.collisions = 0;

		if (m_cci.m_soft_collisionflags & CCD_BSB_COL_CL_RS)
		{
			psb->m_cfg.collisions	+=	btSoftBody::fCollision::CL_RS;
		} else
		{
			psb->m_cfg.collisions	+=	btSoftBody::fCollision::SDF_RS;
		}
		if (m_cci.m_soft_collisionflags & CCD_BSB_COL_CL_SS)
		{
			psb->m_cfg.collisions += btSoftBody::fCollision::CL_SS;
		} else
		{
			psb->m_cfg.collisions += btSoftBody::fCollision::VF_SS;
		}


		psb->m_cfg.kSRHR_CL = m_cci.m_soft_kSRHR_CL;		/* Soft vs rigid hardness [0,1] (cluster only) */
		psb->m_cfg.kSKHR_CL = m_cci.m_soft_kSKHR_CL;		/* Soft vs kinetic hardness [0,1] (cluster only) */
		psb->m_cfg.kSSHR_CL = m_cci.m_soft_kSSHR_CL;		/* Soft vs soft hardness [0,1] (cluster only) */
		psb->m_cfg.kSR_SPLT_CL = m_cci.m_soft_kSR_SPLT_CL;	/* Soft vs rigid impulse split [0,1] (cluster only) */

		psb->m_cfg.kSK_SPLT_CL = m_cci.m_soft_kSK_SPLT_CL;	/* Soft vs rigid impulse split [0,1] (cluster only) */
		psb->m_cfg.kSS_SPLT_CL = m_cci.m_soft_kSS_SPLT_CL;	/* Soft vs rigid impulse split [0,1] (cluster only) */
		psb->m_cfg.kVCF = m_cci.m_soft_kVCF;			/* Velocities correction factor (Baumgarte) */
		psb->m_cfg.kDP = m_cci.m_soft_kDP;			/* Damping coefficient [0,1] */

		psb->m_cfg.kDG = m_cci.m_soft_kDG;			/* Drag coefficient [0,+inf] */
		psb->m_cfg.kLF = m_cci.m_soft_kLF;			/* Lift coefficient [0,+inf] */
		psb->m_cfg.kPR = m_cci.m_soft_kPR;			/* Pressure coefficient [-inf,+inf] */
		psb->m_cfg.kVC = m_cci.m_soft_kVC;			/* Volume conversation coefficient [0,+inf] */

		psb->m_cfg.kDF = m_cci.m_soft_kDF;			/* Dynamic friction coefficient [0,1] */
		psb->m_cfg.kMT = m_cci.m_soft_kMT;			/* Pose matching coefficient [0,1] */
		psb->m_cfg.kCHR = m_cci.m_soft_kCHR;			/* Rigid contacts hardness [0,1] */
		psb->m_cfg.kKHR = m_cci.m_soft_kKHR;			/* Kinetic contacts hardness [0,1] */

		psb->m_cfg.kSHR = m_cci.m_soft_kSHR;			/* Soft contacts hardness [0,1] */
		psb->m_cfg.kAHR = m_cci.m_soft_kAHR;			/* Anchors hardness [0,1] */



		if (m_cci.m_gamesoftFlag & CCD_BSB_BENDING_CONSTRAINTS)//OB_SB_GOAL)
		{
			psb->generateBendingConstraints(2,pm);
		}

		psb->m_cfg.piterations = m_cci.m_soft_piterations;
		psb->m_cfg.viterations = m_cci.m_soft_viterations;
		psb->m_cfg.diterations = m_cci.m_soft_diterations;
		psb->m_cfg.citerations = m_cci.m_soft_citerations;

		if (m_cci.m_gamesoftFlag & CCD_BSB_SHAPE_MATCHING)//OB_SB_GOAL)
		{
			psb->setPose(false,true);//
		} else
		{
			psb->setPose(true,false);
		}


		
		psb->randomizeConstraints();

		if (m_cci.m_soft_collisionflags & (CCD_BSB_COL_CL_RS+CCD_BSB_COL_CL_SS))
		{
			psb->generateClusters(m_cci.m_soft_numclusteriterations);
		}

//		psb->activate();
//		psb->setActivationState(1);
//		psb->setDeactivationTime(1.f);
		
		//psb->m_materials[0]->m_kLST	=	0.1+(i/(btScalar)(n-1))*0.9;
		psb->setTotalMass(m_cci.m_mass);
		
		psb->setCollisionFlags(0);

		///create a mapping between graphics mesh vertices and soft body vertices
		{
			RAS_MeshObject* rasMesh= GetShapeInfo()->GetMesh();

			if (rasMesh && !m_softbodyMappingDone)
			{
				
				//printf("apply\n");
				RAS_MeshSlot::iterator it;
				RAS_MeshMaterial *mmat;
				RAS_MeshSlot *slot;
				size_t i;

				//for each material
				for (int m=0;m<rasMesh->NumMaterials();m++)
				{
					// The vertex cache can only be updated for this deformer:
					// Duplicated objects with more than one ploymaterial (=multiple mesh slot per object)
					// share the same mesh (=the same cache). As the rendering is done per polymaterial
					// cycling through the objects, the entire mesh cache cannot be updated in one shot.
					mmat = rasMesh->GetMeshMaterial(m);

					slot = mmat->m_baseslot;
					for(slot->begin(it); !slot->end(it); slot->next(it))
					{
						int index = 0;
						for(i=it.startvertex; i<it.endvertex; i++,index++) 
						{
							RAS_TexVert* vertex = &it.vertex[i];
							

							//search closest index, and store it in vertex
							vertex->setSoftBodyIndex(0);
							btScalar maxDistSqr = 1e30;
							btSoftBody::tNodeArray&   nodes(psb->m_nodes);
							btVector3 xyz = btVector3(vertex->getXYZ()[0],vertex->getXYZ()[1],vertex->getXYZ()[2]);
							for (int n=0;n<nodes.size();n++)
							{
								btScalar distSqr = (nodes[n].m_x - xyz).length2();
								if (distSqr<maxDistSqr)
								{
									maxDistSqr = distSqr;
									
									vertex->setSoftBodyIndex(n);
								}
							}
						}
					}
				}
			}
		}
		
		m_softbodyMappingDone = true;






//		m_object->setCollisionShape(rbci.m_collisionShape);
		btTransform startTrans;

		if (rbci.m_motionState)
		{
			rbci.m_motionState->getWorldTransform(startTrans);
		} else
		{
			startTrans = rbci.m_startWorldTransform;
		}
		//startTrans.setIdentity();

		//m_object->setWorldTransform(startTrans);
		//m_object->setInterpolationWorldTransform(startTrans);
		m_MotionState->setWorldPosition(startTrans.getOrigin().getX(),startTrans.getOrigin().getY(),startTrans.getOrigin().getZ());
		m_MotionState->setWorldOrientation(0,0,0,1);

		if (!m_prototypeTransformInitialized)
		{
			m_prototypeTransformInitialized = true;
			m_softBodyTransformInitialized = true;
			GetSoftBody()->transform(startTrans);
		}

//		btVector3 wp = m_softBody->getWorldTransform().getOrigin();
//		MT_Point3 center(wp.getX(),wp.getY(),wp.getZ());
//		m_gameobj->NodeSetWorldPosition(center);


	} else
	{
		btRigidBody::btRigidBodyConstructionInfo rbci(m_cci.m_mass,m_bulletMotionState,m_collisionShape,m_cci.m_localInertiaTensor * m_cci.m_inertiaFactor);
		rbci.m_linearDamping = m_cci.m_linearDamping;
		rbci.m_angularDamping = m_cci.m_angularDamping;
		rbci.m_friction = m_cci.m_friction;
		rbci.m_restitution = m_cci.m_restitution;
		m_object = new btRigidBody(rbci);
	}
	
	//
	// init the rigidbody properly
	//
	
	//setMassProps this also sets collisionFlags
	//convert collision flags!
	//special case: a near/radar sensor controller should not be defined static or it will
	//generate loads of static-static collision messages on the console
	if (m_cci.m_bSensor)
	{
		// reset the flags that have been set so far
		GetCollisionObject()->setCollisionFlags(0);
		// sensor must never go to sleep: they need to detect continously
		GetCollisionObject()->setActivationState(DISABLE_DEACTIVATION);
	}
	GetCollisionObject()->setCollisionFlags(m_object->getCollisionFlags() | m_cci.m_collisionFlags);
	btRigidBody* body = GetRigidBody();

	if (body)
	{
		body->setGravity( m_cci.m_gravity);
		body->setDamping(m_cci.m_linearDamping, m_cci.m_angularDamping);

		if (!m_cci.m_bRigid)
		{
			body->setAngularFactor(0.f);
		}
	}
	if (m_object && m_cci.m_do_anisotropic)
	{
		m_object->setAnisotropicFriction(m_cci.m_anisotropicFriction);
	}
		
}

static void DeleteBulletShape(btCollisionShape* shape)
{
	if (shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
	{
		// shapes based on meshes use an interface that contains the vertices.
		btTriangleMeshShape* meshShape = static_cast<btTriangleMeshShape*>(shape);
		btStridingMeshInterface* meshInterface = meshShape->getMeshInterface();
		if (meshInterface)
			delete meshInterface;
	}
	delete shape;
}

CcdPhysicsController::~CcdPhysicsController()
{
	//will be reference counted, due to sharing
	if (m_cci.m_physicsEnv)
		m_cci.m_physicsEnv->removeCcdPhysicsController(this);

	if (m_MotionState)
		delete m_MotionState;
	if (m_bulletMotionState)
		delete m_bulletMotionState;
	delete m_object;

	if (m_collisionShape)
	{
		// collision shape is always unique to the controller, can delete it here
		if (m_collisionShape->isCompound())
		{
			// bullet does not delete the child shape, must do it here
			btCompoundShape* compoundShape = (btCompoundShape*)m_collisionShape;
			int numChild = compoundShape->getNumChildShapes();
			for (int i=numChild-1 ; i >= 0; i--)
			{
				btCollisionShape* childShape = compoundShape->getChildShape(i);
				DeleteBulletShape(childShape);
			}
		}
		DeleteBulletShape(m_collisionShape);
	}
	if (m_shapeInfo)
	{
		m_shapeInfo->Release();
	}
}


		/**
			SynchronizeMotionStates ynchronizes dynas, kinematic and deformable entities (and do 'late binding')
		*/
bool		CcdPhysicsController::SynchronizeMotionStates(float time)
{
	//sync non-static to motionstate, and static from motionstate (todo: add kinematic etc.)

	btSoftBody* sb = GetSoftBody();
	if (sb)
	{
		if (sb->m_pose.m_bframe) 
		{
			btVector3 worldPos = sb->m_pose.m_com;
			btQuaternion worldquat;
			btMatrix3x3	trs = sb->m_pose.m_rot*sb->m_pose.m_scl;
			trs.getRotation(worldquat);
			m_MotionState->setWorldPosition(worldPos[0],worldPos[1],worldPos[2]);
			m_MotionState->setWorldOrientation(worldquat[0],worldquat[1],worldquat[2],worldquat[3]);
		}
		else 
		{
			btVector3 aabbMin,aabbMax;
			sb->getAabb(aabbMin,aabbMax);
			btVector3 worldPos  = (aabbMax+aabbMin)*0.5f;
			m_MotionState->setWorldPosition(worldPos[0],worldPos[1],worldPos[2]);
		}
		m_MotionState->calculateWorldTransformations();
		return true;
	}

	btRigidBody* body = GetRigidBody();

	if (body && !body->isStaticObject())
	{
		
		if ((m_cci.m_clamp_vel_max>0.0) || (m_cci.m_clamp_vel_min>0.0))
		{
			const btVector3& linvel = body->getLinearVelocity();
			float len= linvel.length();
			
			if((m_cci.m_clamp_vel_max>0.0) && (len > m_cci.m_clamp_vel_max))
					body->setLinearVelocity(linvel * (m_cci.m_clamp_vel_max / len));
			
			else if ((m_cci.m_clamp_vel_min>0.0) && btFuzzyZero(len)==0 && (len < m_cci.m_clamp_vel_min))
				body->setLinearVelocity(linvel * (m_cci.m_clamp_vel_min / len));
		}
		
		const btTransform& xform = body->getCenterOfMassTransform();
		const btMatrix3x3& worldOri = xform.getBasis();
		const btVector3& worldPos = xform.getOrigin();
		float ori[12];
		worldOri.getOpenGLSubMatrix(ori);
		m_MotionState->setWorldOrientation(ori);
		m_MotionState->setWorldPosition(worldPos[0],worldPos[1],worldPos[2]);
		m_MotionState->calculateWorldTransformations();

		float scale[3];
		m_MotionState->getWorldScaling(scale[0],scale[1],scale[2]);
		btVector3 scaling(scale[0],scale[1],scale[2]);
		GetCollisionShape()->setLocalScaling(scaling);
	} else
	{
		btVector3 worldPos;
		btQuaternion worldquat;

/*		m_MotionState->getWorldPosition(worldPos[0],worldPos[1],worldPos[2]);
		m_MotionState->getWorldOrientation(worldquat[0],worldquat[1],worldquat[2],worldquat[3]);
		btTransform oldTrans = m_body->getCenterOfMassTransform();
		btTransform newTrans(worldquat,worldPos);
				
		SetCenterOfMassTransform(newTrans);
		//need to keep track of previous position for friction effects...
		
		m_MotionState->calculateWorldTransformations();
*/
		float scale[3];
		m_MotionState->getWorldScaling(scale[0],scale[1],scale[2]);
		btVector3 scaling(scale[0],scale[1],scale[2]);
		GetCollisionShape()->setLocalScaling(scaling);
	}
	return true;

}

		/**
			WriteMotionStateToDynamics synchronizes dynas, kinematic and deformable entities (and do 'late binding')
		*/
		
void		CcdPhysicsController::WriteMotionStateToDynamics(bool nondynaonly)
{
	btTransform& xform = CcdPhysicsController::GetTransformFromMotionState(m_MotionState);
	SetCenterOfMassTransform(xform);
}

void		CcdPhysicsController::WriteDynamicsToMotionState()
{
}
		// controller replication
void		CcdPhysicsController::PostProcessReplica(class PHY_IMotionState* motionstate,class PHY_IPhysicsController* parentctrl)
{
	
	m_softBodyTransformInitialized=false;
	m_MotionState = motionstate;
	m_registerCount = 0;
	m_collisionShape = NULL;

	// always create a new shape to avoid scaling bug
	if (m_shapeInfo)
	{
		m_shapeInfo->AddRef();
		m_collisionShape = m_shapeInfo->CreateBulletShape(m_cci.m_margin);

		if (m_collisionShape)
		{
			// new shape has no scaling, apply initial scaling
			//m_collisionShape->setMargin(m_cci.m_margin);
			m_collisionShape->setLocalScaling(m_cci.m_scaling);
			
			if (m_cci.m_mass)
				m_collisionShape->calculateLocalInertia(m_cci.m_mass, m_cci.m_localInertiaTensor);
		}
	}

	m_object = 0;
	CreateRigidbody();

	btRigidBody* body = GetRigidBody();

	if (body)
	{
		if (m_cci.m_mass)
		{
			body->setMassProps(m_cci.m_mass, m_cci.m_localInertiaTensor * m_cci.m_inertiaFactor);
		}
	}	
	// sensor object are added when needed
	if (!m_cci.m_bSensor)
		m_cci.m_physicsEnv->addCcdPhysicsController(this);


/*	SM_Object* dynaparent=0;
	SumoPhysicsController* sumoparentctrl = (SumoPhysicsController* )parentctrl;
	
	if (sumoparentctrl)
	{
		dynaparent = sumoparentctrl->GetSumoObject();
	}
	
	SM_Object* orgsumoobject = m_sumoObj;
	
	
	m_sumoObj	=	new SM_Object(
		orgsumoobject->getShapeHandle(), 
		orgsumoobject->getMaterialProps(),			
		orgsumoobject->getShapeProps(),
		dynaparent);
	
	m_sumoObj->setRigidBody(orgsumoobject->isRigidBody());
	
	m_sumoObj->setMargin(orgsumoobject->getMargin());
	m_sumoObj->setPosition(orgsumoobject->getPosition());
	m_sumoObj->setOrientation(orgsumoobject->getOrientation());
	//if it is a dyna, register for a callback
	m_sumoObj->registerCallback(*this);
	
	m_sumoScene->add(* (m_sumoObj));
	*/



}


void	CcdPhysicsController::SetCenterOfMassTransform(btTransform& xform)
{
	btRigidBody* body = GetRigidBody();
	if (body)
	{
		body->setCenterOfMassTransform(xform);
	} else
	{
		//either collision object or soft body?
		if (GetSoftBody())
		{

		} else
		{

			if (m_object->isStaticOrKinematicObject())
			{
				m_object->setInterpolationWorldTransform(m_object->getWorldTransform());
			} else
			{
				m_object->setInterpolationWorldTransform(xform);
			}
			if (body)
			{
				body->setInterpolationLinearVelocity(body->getLinearVelocity());
				body->setInterpolationAngularVelocity(body->getAngularVelocity());
				body->updateInertiaTensor();
			}
			m_object->setWorldTransform(xform);
		}
	}
}

		// kinematic methods
void		CcdPhysicsController::RelativeTranslate(float dlocX,float dlocY,float dlocZ,bool local)
{
	if (m_object)
	{
		m_object->activate(true);
		if (m_object->isStaticObject() && !m_cci.m_bSensor)
		{
			m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}

		btRigidBody* body = GetRigidBody();

		btVector3 dloc(dlocX,dlocY,dlocZ);
		btTransform xform = m_object->getWorldTransform();
	
		if (local)
		{
			dloc = xform.getBasis()*dloc;
		}

		xform.setOrigin(xform.getOrigin() + dloc);
		SetCenterOfMassTransform(xform);
	}

}

void		CcdPhysicsController::RelativeRotate(const float rotval[9],bool local)
{
	if (m_object)
	{
		m_object->activate(true);
		if (m_object->isStaticObject() && !m_cci.m_bSensor)
		{
			m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}

		btMatrix3x3 drotmat(	rotval[0],rotval[4],rotval[8],
								rotval[1],rotval[5],rotval[9],
								rotval[2],rotval[6],rotval[10]);


		btMatrix3x3 currentOrn;
		GetWorldOrientation(currentOrn);

		btTransform xform = m_object->getWorldTransform();
		
		xform.setBasis(xform.getBasis()*(local ? 
		drotmat : (currentOrn.inverse() * drotmat * currentOrn)));

		SetCenterOfMassTransform(xform);
	}
}


void CcdPhysicsController::GetWorldOrientation(btMatrix3x3& mat)
{
	float orn[4];
	m_MotionState->getWorldOrientation(orn[0],orn[1],orn[2],orn[3]);
	btQuaternion quat(orn[0],orn[1],orn[2],orn[3]);
	mat.setRotation(quat);
}

void		CcdPhysicsController::getOrientation(float &quatImag0,float &quatImag1,float &quatImag2,float &quatReal)
{
	btQuaternion q = m_object->getWorldTransform().getRotation();
	quatImag0 = q[0];
	quatImag1 = q[1];
	quatImag2 = q[2];
	quatReal = q[3];
}
void		CcdPhysicsController::setOrientation(float quatImag0,float quatImag1,float quatImag2,float quatReal)
{
	if (m_object)
	{
		m_object->activate(true);
		if (m_object->isStaticObject() && !m_cci.m_bSensor)
		{
			m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}
		// not required
		//m_MotionState->setWorldOrientation(quatImag0,quatImag1,quatImag2,quatReal);
		btTransform xform  = m_object->getWorldTransform();
		xform.setRotation(btQuaternion(quatImag0,quatImag1,quatImag2,quatReal));
		SetCenterOfMassTransform(xform);
		// not required
		//m_bulletMotionState->setWorldTransform(xform);
		
		

	}

}

void CcdPhysicsController::setWorldOrientation(const btMatrix3x3& orn)
{
	if (m_object)
	{
		m_object->activate(true);
		if (m_object->isStaticObject() && !m_cci.m_bSensor)
		{
			m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}
		// not required
		//m_MotionState->setWorldOrientation(quatImag0,quatImag1,quatImag2,quatReal);
		btTransform xform  = m_object->getWorldTransform();
		xform.setBasis(orn);
		SetCenterOfMassTransform(xform);
		// not required
		//m_bulletMotionState->setWorldTransform(xform);
		//only once!
		if (!m_softBodyTransformInitialized && GetSoftBody())
		{
			m_softbodyStartTrans.setBasis(orn);
			xform.setOrigin(m_softbodyStartTrans.getOrigin());
			GetSoftBody()->transform(xform);
			m_softBodyTransformInitialized = true;
		}

	}

}

void		CcdPhysicsController::setPosition(float posX,float posY,float posZ)
{
	if (m_object)
	{
		m_object->activate(true);
		if (m_object->isStaticObject() && !m_cci.m_bSensor)
		{
			m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}
		// not required, this function is only used to update the physic controller
		//m_MotionState->setWorldPosition(posX,posY,posZ);
		btTransform xform  = m_object->getWorldTransform();
		xform.setOrigin(btVector3(posX,posY,posZ));
		SetCenterOfMassTransform(xform);
		if (!m_softBodyTransformInitialized)
			m_softbodyStartTrans.setOrigin(xform.getOrigin());
		// not required
		//m_bulletMotionState->setWorldTransform(xform);
	}
}

void CcdPhysicsController::forceWorldTransform(const btMatrix3x3& mat, const btVector3& pos)
{
	if (m_object)
	{
		btTransform& xform = m_object->getWorldTransform();
		xform.setBasis(mat);
		xform.setOrigin(pos);
	}
}


void		CcdPhysicsController::resolveCombinedVelocities(float linvelX,float linvelY,float linvelZ,float angVelX,float angVelY,float angVelZ)
{
}

void 		CcdPhysicsController::getPosition(PHY__Vector3&	pos) const
{
	const btTransform& xform = m_object->getWorldTransform();
	pos[0] = xform.getOrigin().x();
	pos[1] = xform.getOrigin().y();
	pos[2] = xform.getOrigin().z();
}

void		CcdPhysicsController::setScaling(float scaleX,float scaleY,float scaleZ)
{
	if (!btFuzzyZero(m_cci.m_scaling.x()-scaleX) ||
		!btFuzzyZero(m_cci.m_scaling.y()-scaleY) ||
		!btFuzzyZero(m_cci.m_scaling.z()-scaleZ))
	{
		m_cci.m_scaling = btVector3(scaleX,scaleY,scaleZ);

		if (m_object && m_object->getCollisionShape())
		{
			m_object->getCollisionShape()->setLocalScaling(m_cci.m_scaling);
			
			//printf("no inertia recalc for fixed objects with mass=0\n");
			btRigidBody* body = GetRigidBody();
			if (body && m_cci.m_mass)
			{
				body->getCollisionShape()->calculateLocalInertia(m_cci.m_mass, m_cci.m_localInertiaTensor);
				body->setMassProps(m_cci.m_mass, m_cci.m_localInertiaTensor * m_cci.m_inertiaFactor);
			} 
			
		}
	}
}
		
		// physics methods
void		CcdPhysicsController::ApplyTorque(float torqueX,float torqueY,float torqueZ,bool local)
{
	btVector3 torque(torqueX,torqueY,torqueZ);
	btTransform xform = m_object->getWorldTransform();
	

	if (m_object && torque.length2() > (SIMD_EPSILON*SIMD_EPSILON))
	{
		btRigidBody* body = GetRigidBody();
		m_object->activate();
		if (m_object->isStaticObject())
		{
			if (!m_cci.m_bSensor)
				m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			return;
		}
		if (local)
		{
			torque	= xform.getBasis()*torque;
		}
		if (body)
		{
			//workaround for incompatibility between 'DYNAMIC' game object, and angular factor
			//a DYNAMIC object has some inconsistency: it has no angular effect due to collisions, but still has torque
			const btVector3& angFac = body->getAngularFactor();
			body->setAngularFactor(1.f);
			body->applyTorque(torque);
			body->setAngularFactor(angFac);
		}
	}
}

void		CcdPhysicsController::ApplyForce(float forceX,float forceY,float forceZ,bool local)
{
	btVector3 force(forceX,forceY,forceZ);
	

	if (m_object && force.length2() > (SIMD_EPSILON*SIMD_EPSILON))
	{
		m_object->activate();
		if (m_object->isStaticObject())
		{
			if (!m_cci.m_bSensor)
				m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			return;
		}
		btTransform xform = m_object->getWorldTransform();
		
		if (local)
		{	
			force	= xform.getBasis()*force;
		}
		btRigidBody* body = GetRigidBody();
		if (body)
			body->applyCentralForce(force);
		btSoftBody* soft = GetSoftBody();
		if (soft)
		{
			// the force is applied on each node, must reduce it in the same extend
			if (soft->m_nodes.size() > 0)
				force /= soft->m_nodes.size();
			soft->addForce(force);
		}
	}
}
void		CcdPhysicsController::SetAngularVelocity(float ang_velX,float ang_velY,float ang_velZ,bool local)
{
	btVector3 angvel(ang_velX,ang_velY,ang_velZ);
	if (m_object && angvel.length2() > (SIMD_EPSILON*SIMD_EPSILON))
	{
		m_object->activate(true);
		if (m_object->isStaticObject())
		{
			if (!m_cci.m_bSensor)
				m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			return;
		}
		btTransform xform = m_object->getWorldTransform();
		if (local)
		{
			angvel	= xform.getBasis()*angvel;
		}
		btRigidBody* body = GetRigidBody();
		if (body)
			body->setAngularVelocity(angvel);
	}

}
void		CcdPhysicsController::SetLinearVelocity(float lin_velX,float lin_velY,float lin_velZ,bool local)
{

	btVector3 linVel(lin_velX,lin_velY,lin_velZ);
	if (m_object/* && linVel.length2() > (SIMD_EPSILON*SIMD_EPSILON)*/)
	{
		m_object->activate(true);
		if (m_object->isStaticObject())
		{
			if (!m_cci.m_bSensor)
				m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			return;
		}
		
		btSoftBody* soft = GetSoftBody();
		if (soft)
		{
			if (local)
			{
				linVel	= m_softbodyStartTrans.getBasis()*linVel;
			}
			soft->setVelocity(linVel);
		} else
		{
			btTransform xform = m_object->getWorldTransform();
			if (local)
			{
				linVel	= xform.getBasis()*linVel;
			}
			btRigidBody* body = GetRigidBody();
			if (body)
				body->setLinearVelocity(linVel);
		}
	}
}
void		CcdPhysicsController::applyImpulse(float attachX,float attachY,float attachZ, float impulseX,float impulseY,float impulseZ)
{
	btVector3 impulse(impulseX,impulseY,impulseZ);

	if (m_object && impulse.length2() > (SIMD_EPSILON*SIMD_EPSILON))
	{
		m_object->activate();
		if (m_object->isStaticObject())
		{
			if (!m_cci.m_bSensor)
				m_object->setCollisionFlags(m_object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			return;
		}
		
		btVector3 pos(attachX,attachY,attachZ);
		btRigidBody* body = GetRigidBody();
		if (body)
			body->applyImpulse(impulse,pos);
			
	}

}
void		CcdPhysicsController::SetActive(bool active)
{
}
		// reading out information from physics
void		CcdPhysicsController::GetLinearVelocity(float& linvX,float& linvY,float& linvZ)
{
	btRigidBody* body = GetRigidBody();
	if (body)
	{
		const btVector3& linvel = body->getLinearVelocity();
		linvX = linvel.x();
		linvY = linvel.y();
		linvZ = linvel.z();
	} else
	{
		linvX = 0.f;
		linvY = 0.f;
		linvZ = 0.f;
	}

}

void		CcdPhysicsController::GetAngularVelocity(float& angVelX,float& angVelY,float& angVelZ)
{
	btRigidBody* body = GetRigidBody();
	if (body)
	{
		const btVector3& angvel= body->getAngularVelocity();
		angVelX = angvel.x();
		angVelY = angvel.y();
		angVelZ = angvel.z();
	} else
	{
		angVelX = 0.f;
		angVelY = 0.f;
		angVelZ = 0.f;
	}
}

void		CcdPhysicsController::GetVelocity(const float posX,const float posY,const float posZ,float& linvX,float& linvY,float& linvZ)
{
	btVector3 pos(posX,posY,posZ);
	btRigidBody* body = GetRigidBody();
	if (body)
	{
		btVector3 linvel = body->getVelocityInLocalPoint(pos);
		linvX = linvel.x();
		linvY = linvel.y();
		linvZ = linvel.z();
	} else
	{
		linvX = 0.f;
		linvY = 0.f;
		linvZ = 0.f;
	}
}
void		CcdPhysicsController::getReactionForce(float& forceX,float& forceY,float& forceZ)
{
}

		// dyna's that are rigidbody are free in orientation, dyna's with non-rigidbody are restricted 
void		CcdPhysicsController::setRigidBody(bool rigid)
{
	if (!rigid)
	{
		btRigidBody* body = GetRigidBody();
		if (body)
		{
			//fake it for now
			btVector3 inertia = body->getInvInertiaDiagLocal();
			inertia[1] = 0.f;
			body->setInvInertiaDiagLocal(inertia);
			body->updateInertiaTensor();
		}
	}
}

		// clientinfo for raycasts for example
void*		CcdPhysicsController::getNewClientInfo()
{
	return m_newClientInfo;
}
void		CcdPhysicsController::setNewClientInfo(void* clientinfo)
{
	m_newClientInfo = clientinfo;
}


void	CcdPhysicsController::UpdateDeactivation(float timeStep)
{
	btRigidBody* body = GetRigidBody();
	if (body)
	{
		body->updateDeactivation( timeStep);
	}
}

bool CcdPhysicsController::wantsSleeping()
{
	btRigidBody* body = GetRigidBody();
	if (body)
	{
		return body->wantsSleeping();
	}
	//check it out
	return true;
}

PHY_IPhysicsController*	CcdPhysicsController::GetReplica()
{
	// This is used only to replicate Near and Radar sensor controllers
	// The replication of object physics controller is done in KX_BulletPhysicsController::GetReplica()
	CcdConstructionInfo cinfo = m_cci;
	if (m_shapeInfo)
	{
		// This situation does not normally happen
		cinfo.m_collisionShape = m_shapeInfo->CreateBulletShape(0.01);
	} 
	else if (m_collisionShape)
	{
		switch (m_collisionShape->getShapeType())
		{
		case SPHERE_SHAPE_PROXYTYPE:
			{
				btSphereShape* orgShape = (btSphereShape*)m_collisionShape;
				cinfo.m_collisionShape = new btSphereShape(*orgShape);
				break;
			}

		case CONE_SHAPE_PROXYTYPE:
			{
				btConeShape* orgShape = (btConeShape*)m_collisionShape;
				cinfo.m_collisionShape = new btConeShape(*orgShape);
				break;
			}

		default:
			{
				return 0;
			}
		}
	}

	cinfo.m_MotionState = new DefaultMotionState();
	cinfo.m_shapeInfo = m_shapeInfo;

	CcdPhysicsController* replica = new CcdPhysicsController(cinfo);
	return replica;
}

///////////////////////////////////////////////////////////
///A small utility class, DefaultMotionState
///
///////////////////////////////////////////////////////////

DefaultMotionState::DefaultMotionState()
{
	m_worldTransform.setIdentity();
	m_localScaling.setValue(1.f,1.f,1.f);
}


DefaultMotionState::~DefaultMotionState()
{

}

void	DefaultMotionState::getWorldPosition(float& posX,float& posY,float& posZ)
{
	posX = m_worldTransform.getOrigin().x();
	posY = m_worldTransform.getOrigin().y();
	posZ = m_worldTransform.getOrigin().z();
}

void	DefaultMotionState::getWorldScaling(float& scaleX,float& scaleY,float& scaleZ)
{
	scaleX = m_localScaling.getX();
	scaleY = m_localScaling.getY();
	scaleZ = m_localScaling.getZ();
}

void	DefaultMotionState::getWorldOrientation(float& quatIma0,float& quatIma1,float& quatIma2,float& quatReal)
{
	btQuaternion quat = m_worldTransform.getRotation();
	quatIma0 = quat.x();
	quatIma1 = quat.y();
	quatIma2 = quat.z();
	quatReal = quat[3];
}
		
void	DefaultMotionState::getWorldOrientation(float* ori)
{
	m_worldTransform.getBasis().getOpenGLSubMatrix(ori);
}

void	DefaultMotionState::setWorldOrientation(const float* ori)
{
	m_worldTransform.getBasis().setFromOpenGLSubMatrix(ori);
}
void	DefaultMotionState::setWorldPosition(float posX,float posY,float posZ)
{
	btVector3 pos(posX,posY,posZ);
	m_worldTransform.setOrigin( pos );
}

void	DefaultMotionState::setWorldOrientation(float quatIma0,float quatIma1,float quatIma2,float quatReal)
{
	btQuaternion orn(quatIma0,quatIma1,quatIma2,quatReal);
	m_worldTransform.setRotation( orn );
}
		
void	DefaultMotionState::calculateWorldTransformations()
{

}

// Shape constructor
std::map<RAS_MeshObject*, CcdShapeConstructionInfo*> CcdShapeConstructionInfo::m_meshShapeMap;

CcdShapeConstructionInfo* CcdShapeConstructionInfo::FindMesh(RAS_MeshObject* mesh, struct DerivedMesh* dm, bool polytope, bool gimpact)
{
	if (polytope || dm || gimpact)
		// not yet supported
		return NULL;

	std::map<RAS_MeshObject*,CcdShapeConstructionInfo*>::const_iterator mit = m_meshShapeMap.find(mesh);
	if (mit != m_meshShapeMap.end())
		return mit->second;
	return NULL;
}

bool CcdShapeConstructionInfo::SetMesh(RAS_MeshObject* meshobj, DerivedMesh* dm, bool polytope,bool useGimpact)
{
	int numpolys, numverts;

	m_useGimpact = useGimpact;

	// assume no shape information
	// no support for dynamic change of shape yet
	assert(IsUnused());
	m_shapeType = PHY_SHAPE_NONE;
	m_meshObject = NULL;
	bool free_dm = false;

	// No mesh object or mesh has no polys
	if (!meshobj || meshobj->HasColliderPolygon()==false) {
		m_vertexArray.clear();
		m_polygonIndexArray.clear();
		m_triFaceArray.clear();
		return false;
	}

	if (!dm) {
		free_dm = true;
		dm = CDDM_from_mesh(meshobj->GetMesh(), NULL);
	}

	MVert *mvert = dm->getVertArray(dm);
	MFace *mface = dm->getFaceArray(dm);
	numpolys = dm->getNumFaces(dm);
	numverts = dm->getNumVerts(dm);
	int* index = (int*)dm->getFaceDataArray(dm, CD_ORIGINDEX);

	m_shapeType = (polytope) ? PHY_SHAPE_POLYTOPE : PHY_SHAPE_MESH;

	/* Convert blender geometry into bullet mesh, need these vars for mapping */
	vector<bool> vert_tag_array(numverts, false);
	unsigned int tot_bt_verts= 0;
	unsigned int orig_index;
	int i;

	if (polytope)
	{
		// Tag verts we're using
		for (int p2=0; p2<numpolys; p2++)
		{
			MFace* mf = &mface[p2];
			RAS_Polygon* poly = meshobj->GetPolygon(index[p2]);

			// only add polygons that have the collision flag set
			if (poly->IsCollider())
			{
				if (vert_tag_array[mf->v1]==false) {vert_tag_array[mf->v1]= true;tot_bt_verts++;}
				if (vert_tag_array[mf->v2]==false) {vert_tag_array[mf->v2]= true;tot_bt_verts++;}
				if (vert_tag_array[mf->v3]==false) {vert_tag_array[mf->v3]= true;tot_bt_verts++;}
				if (mf->v4 && vert_tag_array[mf->v4]==false) {vert_tag_array[mf->v4]= true;tot_bt_verts++;}
			}
		}

		m_vertexArray.resize(tot_bt_verts*3);

		btScalar *bt= &m_vertexArray[0];

		for (int p2=0; p2<numpolys; p2++)
		{
			MFace* mf = &mface[p2];
			RAS_Polygon* poly= meshobj->GetPolygon(index[p2]);

			// only add polygons that have the collisionflag set
			if (poly->IsCollider())
			{
				if (vert_tag_array[mf->v1]==true)
				{
					const float* vtx = mvert[mf->v1].co;
					vert_tag_array[mf->v1]= false;
					*bt++ = vtx[0];
					*bt++ = vtx[1];
					*bt++ = vtx[2];
				}
				if (vert_tag_array[mf->v2]==true)
				{
					const float* vtx = mvert[mf->v2].co;
					vert_tag_array[mf->v2]= false;
					*bt++ = vtx[0];
					*bt++ = vtx[1];
					*bt++ = vtx[2];
				}
				if (vert_tag_array[mf->v3]==true)
				{
					const float* vtx = mvert[mf->v3].co;
					vert_tag_array[mf->v3]= false;
					*bt++ = vtx[0];
					*bt++ = vtx[1];
					*bt++ = vtx[2];
				}
				if (mf->v4 && vert_tag_array[mf->v4]==true)
				{
					const float* vtx = mvert[mf->v4].co;
					vert_tag_array[mf->v4]= false;
					*bt++ = vtx[0];
					*bt++ = vtx[1];
					*bt++ = vtx[2];
				}
			}
		}
	}
	else {
		unsigned int tot_bt_tris= 0;
		vector<int> vert_remap_array(numverts, 0);
		
		// Tag verts we're using
		for (int p2=0; p2<numpolys; p2++)
		{
			MFace* mf = &mface[p2];
			RAS_Polygon* poly= meshobj->GetPolygon(index[p2]);

			// only add polygons that have the collision flag set
			if (poly->IsCollider())
			{
				if (vert_tag_array[mf->v1]==false)
					{vert_tag_array[mf->v1]= true;vert_remap_array[mf->v1]= tot_bt_verts;tot_bt_verts++;}
				if (vert_tag_array[mf->v2]==false)
					{vert_tag_array[mf->v2]= true;vert_remap_array[mf->v2]= tot_bt_verts;tot_bt_verts++;}
				if (vert_tag_array[mf->v3]==false)
					{vert_tag_array[mf->v3]= true;vert_remap_array[mf->v3]= tot_bt_verts;tot_bt_verts++;}
				if (mf->v4 && vert_tag_array[mf->v4]==false)
					{vert_tag_array[mf->v4]= true;vert_remap_array[mf->v4]= tot_bt_verts;tot_bt_verts++;}
				tot_bt_tris += (mf->v4 ? 2:1); /* a quad or a tri */
			}
		}

		m_vertexArray.resize(tot_bt_verts*3);
		m_polygonIndexArray.resize(tot_bt_tris);
		m_triFaceArray.resize(tot_bt_tris*3);

		btScalar *bt= &m_vertexArray[0];
		int *poly_index_pt= &m_polygonIndexArray[0];
		int *tri_pt= &m_triFaceArray[0];

		for (int p2=0; p2<numpolys; p2++)
		{
			MFace* mf = &mface[p2];
			RAS_Polygon* poly= meshobj->GetPolygon(index[p2]);

			// only add polygons that have the collisionflag set
			if (poly->IsCollider())
			{
				MVert *v1= &mvert[mf->v1];
				MVert *v2= &mvert[mf->v2];
				MVert *v3= &mvert[mf->v3];

				// the face indicies
				tri_pt[0]= vert_remap_array[mf->v1];
				tri_pt[1]= vert_remap_array[mf->v2];
				tri_pt[2]= vert_remap_array[mf->v3];
				tri_pt= tri_pt+3;

				// m_polygonIndexArray
				*poly_index_pt= index[p2];
				poly_index_pt++;

				// the vertex location
				if (vert_tag_array[mf->v1]==true) { /* *** v1 *** */
					vert_tag_array[mf->v1]= false;
					*bt++ = v1->co[0];
					*bt++ = v1->co[1];
					*bt++ = v1->co[2];
				}
				if (vert_tag_array[mf->v2]==true) { /* *** v2 *** */
					vert_tag_array[mf->v2]= false;
					*bt++ = v2->co[0];
					*bt++ = v2->co[1];
					*bt++ = v2->co[2];
				}
				if (vert_tag_array[mf->v3]==true) { /* *** v3 *** */
					vert_tag_array[mf->v3]= false;
					*bt++ = v3->co[0];	
					*bt++ = v3->co[1];
					*bt++ = v3->co[2];
				}

				if (mf->v4)
				{
					MVert *v4= &mvert[mf->v4];

					tri_pt[0]= vert_remap_array[mf->v1];
					tri_pt[1]= vert_remap_array[mf->v3];
					tri_pt[2]= vert_remap_array[mf->v4];
					tri_pt= tri_pt+3;

					// m_polygonIndexArray
					*poly_index_pt= index[p2];
					poly_index_pt++;

					// the vertex location
					if (vert_tag_array[mf->v4]==true) { /* *** v4 *** */
						vert_tag_array[mf->v4]= false;
						*bt++ = v4->co[0];
						*bt++ = v4->co[1];	
						*bt++ = v4->co[2];
					}
				}
			}
		}


		/* If this ever gets confusing, print out an OBJ file for debugging */
#if 0
		printf("# vert count %d\n", m_vertexArray.size());
		for(i=0; i<m_vertexArray.size(); i+=1) {
			printf("v %.6f %.6f %.6f\n", m_vertexArray[i].x(), m_vertexArray[i].y(), m_vertexArray[i].z());
		}

		printf("# face count %d\n", m_triFaceArray.size());
		for(i=0; i<m_triFaceArray.size(); i+=3) {
			printf("f %d %d %d\n", m_triFaceArray[i]+1, m_triFaceArray[i+1]+1, m_triFaceArray[i+2]+1);
		}
#endif

	}

#if 0
	if (validpolys==false)
	{
		// should not happen
		m_shapeType = PHY_SHAPE_NONE;
		return false;
	}
#endif
	
	m_meshObject = meshobj;
	if (free_dm) {
		dm->release(dm);
		dm = NULL;
	}

	// sharing only on static mesh at present, if you change that, you must also change in FindMesh
	if (!polytope && !dm && !useGimpact)
	{
		// triangle shape can be shared, store the mesh object in the map
		m_meshShapeMap.insert(std::pair<RAS_MeshObject*,CcdShapeConstructionInfo*>(meshobj,this));
	}
	return true;
}

bool CcdShapeConstructionInfo::SetProxy(CcdShapeConstructionInfo* shapeInfo)
{
	if (shapeInfo == NULL)
		return false;
	// no support for dynamic change
	assert(IsUnused());
	m_shapeType = PHY_SHAPE_PROXY;
	m_shapeProxy = shapeInfo;
	return true;
}

btCollisionShape* CcdShapeConstructionInfo::CreateBulletShape(btScalar margin)
{
	btCollisionShape* collisionShape = 0;
	btTriangleMeshShape* concaveShape = 0;
	btCompoundShape* compoundShape = 0;
	CcdShapeConstructionInfo* nextShapeInfo;

	if (m_shapeType == PHY_SHAPE_PROXY && m_shapeProxy != NULL)
		return m_shapeProxy->CreateBulletShape(margin);

	switch (m_shapeType) 
	{
	default:
		break;

	case PHY_SHAPE_BOX:
		collisionShape = new btBoxShape(m_halfExtend);
		collisionShape->setMargin(margin);
		break;

	case PHY_SHAPE_SPHERE:
		collisionShape = new btSphereShape(m_radius);
		collisionShape->setMargin(margin);
		break;

	case PHY_SHAPE_CYLINDER:
		collisionShape = new btCylinderShapeZ(m_halfExtend);
		collisionShape->setMargin(margin);
		break;

	case PHY_SHAPE_CONE:
		collisionShape = new btConeShapeZ(m_radius, m_height);
		collisionShape->setMargin(margin);
		break;

	case PHY_SHAPE_POLYTOPE:
		collisionShape = new btConvexHullShape(&m_vertexArray[0], m_vertexArray.size()/3, 3*sizeof(btScalar));
		collisionShape->setMargin(margin);
		break;

	case PHY_SHAPE_MESH:
		// Let's use the latest btScaledBvhTriangleMeshShape: it allows true sharing of 
		// triangle mesh information between duplicates => drastic performance increase when 
		// duplicating complex mesh objects. 
		// BUT it causes a small performance decrease when sharing is not required: 
		// 9 multiplications/additions and one function call for each triangle that passes the mid phase filtering
		// One possible optimization is to use directly the btBvhTriangleMeshShape when the scale is 1,1,1
		// and btScaledBvhTriangleMeshShape otherwise.
		if (m_useGimpact)
		{				
				btTriangleIndexVertexArray* indexVertexArrays = new btTriangleIndexVertexArray(
						m_polygonIndexArray.size(),
						&m_triFaceArray[0],
						3*sizeof(int),
						m_vertexArray.size()/3,
						&m_vertexArray[0],
						3*sizeof(btScalar)
				);
				
				btGImpactMeshShape* gimpactShape =  new btGImpactMeshShape(indexVertexArrays);
				gimpactShape->setMargin(margin);
				collisionShape = gimpactShape;
				gimpactShape->updateBound();

		} else
		{
			if (!m_unscaledShape)
			{
			
				btTriangleIndexVertexArray* indexVertexArrays = 0;

				///enable welding, only for the objects that need it (such as soft bodies)
				if (0.f != m_weldingThreshold1)
				{
					btTriangleMesh* collisionMeshData = new btTriangleMesh(true,false);
					collisionMeshData->m_weldingThreshold = m_weldingThreshold1;
					bool removeDuplicateVertices=true;
					// m_vertexArray not in multiple of 3 anymore, use m_triFaceArray
					for(int i=0; i<m_triFaceArray.size(); i+=3) {
						btScalar *bt = &m_vertexArray[3*m_triFaceArray[i]];
						btVector3 v1(bt[0], bt[1], bt[2]);
						bt = &m_vertexArray[3*m_triFaceArray[i+1]];
						btVector3 v2(bt[0], bt[1], bt[2]);
						bt = &m_vertexArray[3*m_triFaceArray[i+2]];
						btVector3 v3(bt[0], bt[1], bt[2]);
						collisionMeshData->addTriangle(v1, v2, v3, removeDuplicateVertices);
					}
					indexVertexArrays = collisionMeshData;

				} else
				{
					indexVertexArrays = new btTriangleIndexVertexArray(
							m_polygonIndexArray.size(),
							&m_triFaceArray[0],
							3*sizeof(int),
							m_vertexArray.size()/3,
							&m_vertexArray[0],
							3*sizeof(btScalar));
				}
				
				// this shape will be shared and not deleted until shapeInfo is deleted
				m_unscaledShape = new btBvhTriangleMeshShape( indexVertexArrays, true );
				m_unscaledShape->recalcLocalAabb();
			}
			collisionShape = new btScaledBvhTriangleMeshShape(m_unscaledShape, btVector3(1.0f,1.0f,1.0f));
			collisionShape->setMargin(margin);
		}
		break;

	case PHY_SHAPE_COMPOUND:
		if (m_shapeArray.size() > 0)
		{
			compoundShape = new btCompoundShape();
			for (std::vector<CcdShapeConstructionInfo*>::iterator sit = m_shapeArray.begin();
				 sit != m_shapeArray.end();
				 sit++)
			{
				collisionShape = (*sit)->CreateBulletShape(margin);
				if (collisionShape)
				{
					collisionShape->setLocalScaling((*sit)->m_childScale);
					compoundShape->addChildShape((*sit)->m_childTrans, collisionShape);
				}
			}
			collisionShape = compoundShape;
		}
	}
	return collisionShape;
}

void CcdShapeConstructionInfo::AddShape(CcdShapeConstructionInfo* shapeInfo)
{
	m_shapeArray.push_back(shapeInfo);
}

CcdShapeConstructionInfo::~CcdShapeConstructionInfo()
{
	for (std::vector<CcdShapeConstructionInfo*>::iterator sit = m_shapeArray.begin();
		 sit != m_shapeArray.end();
		 sit++)
	{
		(*sit)->Release();
	}
	m_shapeArray.clear();
	if (m_unscaledShape)
	{
		DeleteBulletShape(m_unscaledShape);
	}
	m_vertexArray.clear();
	if (m_shapeType == PHY_SHAPE_MESH && m_meshObject != NULL) 
	{
		std::map<RAS_MeshObject*,CcdShapeConstructionInfo*>::iterator mit = m_meshShapeMap.find(m_meshObject);
		if (mit != m_meshShapeMap.end() && mit->second == this)
		{
			m_meshShapeMap.erase(mit);
		}
	}
	if (m_shapeType == PHY_SHAPE_PROXY && m_shapeProxy != NULL)
	{
		m_shapeProxy->Release();
	}
}


