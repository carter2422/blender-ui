#include "CcdPhysicsEnvironment.h"
#include "CcdPhysicsController.h"

#include <algorithm>
#include "SimdTransform.h"
#include "Dynamics/RigidBody.h"
#include "BroadphaseCollision/BroadPhaseInterface.h"
#include "BroadphaseCollision/SimpleBroadphase.h"

#include "CollisionShapes/ConvexShape.h"
#include "BroadphaseCollision/CollisionDispatcher.h"
#include "NarrowPhaseCollision/PersistentManifold.h"
#include "CollisionShapes/TriangleMeshShape.h"
#include "ConstraintSolver/OdeConstraintSolver.h"
#include "ConstraintSolver/SimpleConstraintSolver.h"

#include "IDebugDraw.h"


#include "CollisionDispatch/ToiContactDispatcher.h"


#include "CollisionDispatch/EmptyCollisionAlgorithm.h"
#include "CollisionDispatch/UnionFind.h"

#include "NarrowPhaseCollision/RaycastCallback.h"

bool useIslands = true;

#include "ConstraintSolver/ConstraintSolver.h"
#include "ConstraintSolver/Point2PointConstraint.h"
//#include "BroadphaseCollision/QueryDispatcher.h"
//#include "BroadphaseCollision/QueryBox.h"
//todo: change this to allow dynamic registration of types!

unsigned long gNumIterations = 10;

#ifdef WIN32
void DrawRasterizerLine(const float* from,const float* to,int color);
#endif


#include "ConstraintSolver/ContactConstraint.h"



#include <stdio.h>




static void DrawAabb(IDebugDraw* debugDrawer,const SimdVector3& from,const SimdVector3& to,const SimdVector3& color)
{
	SimdVector3 halfExtents = (to-from)* 0.5f;
	SimdVector3 center = (to+from) *0.5f;
	int i,j;

	SimdVector3 edgecoord(1.f,1.f,1.f),pa,pb;
	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++)
		{
			pa = SimdVector3(edgecoord[0]*halfExtents[0], edgecoord[1]*halfExtents[1],		
				edgecoord[2]*halfExtents[2]);
			pa+=center;
			
			int othercoord = j%3;
			edgecoord[othercoord]*=-1.f;
			pb = SimdVector3(edgecoord[0]*halfExtents[0], edgecoord[1]*halfExtents[1],	
				edgecoord[2]*halfExtents[2]);
			pb+=center;
			
			debugDrawer->DrawLine(pa,pb,color);
		}
		edgecoord = SimdVector3(-1.f,-1.f,-1.f);
		if (i<3)
			edgecoord[i]*=-1.f;
	}


}






CcdPhysicsEnvironment::CcdPhysicsEnvironment(ToiContactDispatcher* dispatcher,BroadphaseInterface* bp)
:m_dispatcher(dispatcher),
m_broadphase(bp),
m_scalingPropagated(false)
{
	if (!m_dispatcher)
	{
		OdeConstraintSolver* solver = new OdeConstraintSolver();
		//SimpleConstraintSolver* solver= new SimpleConstraintSolver();
		m_dispatcher = new ToiContactDispatcher(solver);
	}
	if (!m_broadphase)
	{
		m_broadphase = new SimpleBroadphase();
	}
	
	m_debugDrawer = 0;
	m_gravity = SimdVector3(0.f,-10.f,0.f);
	

}

void	CcdPhysicsEnvironment::addCcdPhysicsController(CcdPhysicsController* ctrl)
{
	ctrl->GetRigidBody()->setGravity( m_gravity );
	m_controllers.push_back(ctrl);
	
	BroadphaseInterface* scene =  m_broadphase;
	
	CollisionShape* shapeinterface = ctrl->GetCollisionShape();
	
	assert(shapeinterface);
	
	const SimdTransform& t = ctrl->GetRigidBody()->getCenterOfMassTransform();
	
	RigidBody* body = ctrl->GetRigidBody();
	
	SimdPoint3 minAabb,maxAabb;
	
	shapeinterface->GetAabb(t,minAabb,maxAabb);
	
	float timeStep = 0.02f;
	
	
	//extent it with the motion
	
	SimdVector3 linMotion = body->getLinearVelocity()*timeStep;
	
	float maxAabbx = maxAabb.getX();
	float maxAabby = maxAabb.getY();
	float maxAabbz = maxAabb.getZ();
	float minAabbx = minAabb.getX();
	float minAabby = minAabb.getY();
	float minAabbz = minAabb.getZ();

	if (linMotion.x() > 0.f)
		maxAabbx += linMotion.x(); 
	else
		minAabbx += linMotion.x();
	if (linMotion.y() > 0.f)
		maxAabby += linMotion.y(); 
	else
		minAabby += linMotion.y();
	if (linMotion.z() > 0.f)
		maxAabbz += linMotion.z(); 
	else
		minAabbz += linMotion.z();
	

	minAabb = SimdVector3(minAabbx,minAabby,minAabbz);
	maxAabb = SimdVector3(maxAabbx,maxAabby,maxAabbz);
	
	if (!ctrl->m_broadphaseHandle)
	{
		int type = shapeinterface->GetShapeType();
		ctrl->m_broadphaseHandle = scene->CreateProxy(
			ctrl->GetRigidBody(),
			type,
			minAabb, 
			maxAabb);
	}
	
	body->SetCollisionShape( shapeinterface );
	
	
	
}

void	CcdPhysicsEnvironment::removeCcdPhysicsController(CcdPhysicsController* ctrl)
{
	
	//also remove constraint
	
	{
		std::vector<Point2PointConstraint*>::iterator i;
		
		for (i=m_p2pConstraints.begin();
		!(i==m_p2pConstraints.end()); i++)
		{
			Point2PointConstraint* p2p = (*i);
			if  ((&p2p->GetRigidBodyA() == ctrl->GetRigidBody() ||
				(&p2p->GetRigidBodyB() == ctrl->GetRigidBody())))
			{
				removeConstraint(int(p2p));
				//only 1 constraint per constroller
				break;
			}
		}
	}
	
	{
		std::vector<Point2PointConstraint*>::iterator i;
		
		for (i=m_p2pConstraints.begin();
		!(i==m_p2pConstraints.end()); i++)
		{
			Point2PointConstraint* p2p = (*i);
			if  ((&p2p->GetRigidBodyA() == ctrl->GetRigidBody() ||
				(&p2p->GetRigidBodyB() == ctrl->GetRigidBody())))
			{
				removeConstraint(int(p2p));
				//only 1 constraint per constroller
				break;
			}
		}
	}
	
	
	
	bool removeFromBroadphase = false;
	
	{
		BroadphaseInterface* scene = m_broadphase;
		BroadphaseProxy* bp = (BroadphaseProxy*)ctrl->m_broadphaseHandle;
		
		if (removeFromBroadphase)
		{
		}
		//
		// only clear the cached algorithms
		//
		scene->CleanProxyFromPairs(bp);
	}
	{
		std::vector<CcdPhysicsController*>::iterator i =
			std::find(m_controllers.begin(), m_controllers.end(), ctrl);
		if (!(i == m_controllers.end()))
		{
			std::swap(*i, m_controllers.back());
			m_controllers.pop_back();
		}
	}
}

void	CcdPhysicsEnvironment::UpdateActivationState()
{
	m_dispatcher->InitUnionFind();
	
	// put the index into m_controllers into m_tag	
	{
		std::vector<CcdPhysicsController*>::iterator i;
		
		int index = 0;
		for (i=m_controllers.begin();
		!(i==m_controllers.end()); i++)
		{
			CcdPhysicsController* ctrl = (*i);
			RigidBody* body = ctrl->GetRigidBody();
			body->m_islandTag1 = index;
			body->m_hitFraction = 1.f;
			index++;
			
		}
	}
	// do the union find
	
	m_dispatcher->FindUnions();
	
	// put the islandId ('find' value) into m_tag	
	{
		UnionFind& unionFind = m_dispatcher->GetUnionFind();
		
		std::vector<CcdPhysicsController*>::iterator i;
		
		int index = 0;
		for (i=m_controllers.begin();
		!(i==m_controllers.end()); i++)
		{
			CcdPhysicsController* ctrl = (*i);
			RigidBody* body = ctrl->GetRigidBody();
			
			
			if (body->mergesSimulationIslands())
			{
				body->m_islandTag1 = unionFind.find(index);
			} else
			{
				body->m_islandTag1 = -1;
			}
			index++;
		}
	}
	
}

bool gPredictCollision = false;//true;//false;


/// Perform an integration step of duration 'timeStep'.
bool	CcdPhysicsEnvironment::proceedDeltaTime(double curTime,float timeStep)
{
	
	
//	printf("CcdPhysicsEnvironment::proceedDeltaTime\n");
	
	if (timeStep == 0.f)
		return true;

	//clamp hardcoded for now
	if (timeStep > 0.02)
		timeStep = 0.02;
	
	//this is needed because scaling is not known in advance, and scaling has to propagate to the shape
	if (!m_scalingPropagated)
	{
		//SyncMotionStates(timeStep);
		//m_scalingPropagated = true;
	}



	{
//		std::vector<CcdPhysicsController*>::iterator i;
		
		
		
		int k;
		for (k=0;k<GetNumControllers();k++)
		{
			CcdPhysicsController* ctrl = m_controllers[k];
			//		SimdTransform predictedTrans;
			RigidBody* body = ctrl->GetRigidBody();
			if (body->GetActivationState() != ISLAND_SLEEPING)
			{
				body->applyForces( timeStep);
				body->integrateVelocities( timeStep);
			}
			
		}
	}
	BroadphaseInterface*	scene = m_broadphase;
	
	
	//
	// collision detection (?)
	//
	
	
	
	
	
	int numsubstep = gNumIterations;
	
	
	DispatcherInfo dispatchInfo;
	dispatchInfo.m_timeStep = timeStep;
	dispatchInfo.m_stepCount = 0;

	scene->DispatchAllCollisionPairs(*m_dispatcher,dispatchInfo);///numsubstep,g);



	
		
	
	int numRigidBodies = m_controllers.size();
	
	UpdateActivationState();

	//contacts

	
	m_dispatcher->SolveConstraints(timeStep, gNumIterations ,numRigidBodies,m_debugDrawer);

	for (int g=0;g<numsubstep;g++)
	{
		//
		// constraint solving
		//
		
		
		int i;
		int numPoint2Point = m_p2pConstraints.size();
		
		//point to point constraints
		for (i=0;i< numPoint2Point ; i++ )
		{
			Point2PointConstraint* p2p = m_p2pConstraints[i];
			
			p2p->BuildJacobian();
			p2p->SolveConstraint( timeStep );
			
		}
		/*
		//vehicles
		int numVehicles = m_vehicles.size();
		for (i=0;i<numVehicles;i++)
		{
			Vehicle* vehicle = m_vehicles[i];
			vehicle->UpdateVehicle( timeStep );
		}
		*/
		
		
		
	}
	
	{
		
		
		
		{
			
			std::vector<CcdPhysicsController*>::iterator i;
			
			//
			// update aabbs, only for moving objects (!)
			//
			for (i=m_controllers.begin();
			!(i==m_controllers.end()); i++)
			{
				CcdPhysicsController* ctrl = (*i);
				RigidBody* body = ctrl->GetRigidBody();
				
				
				SimdPoint3 minAabb,maxAabb;
				CollisionShape* shapeinterface = ctrl->GetCollisionShape();



				shapeinterface->CalculateTemporalAabb(body->getCenterOfMassTransform(),
					body->getLinearVelocity(),body->getAngularVelocity(),
					timeStep,minAabb,maxAabb);

				shapeinterface->GetAabb(body->getCenterOfMassTransform(),
					minAabb,maxAabb);

				
				BroadphaseProxy* bp = (BroadphaseProxy*) ctrl->m_broadphaseHandle;
				if (bp)
				{
					
#ifdef WIN32
					SimdVector3 color (1,0,0);
					if (m_debugDrawer)
					{	
						//draw aabb

						DrawAabb(m_debugDrawer,minAabb,maxAabb,color);
					}
#endif
					scene->SetAabb(bp,minAabb,maxAabb);
				}
			}
			
			float toi = 1.f;


			
			if (gPredictCollision)
			{
				DispatcherInfo dispatchInfo;
				dispatchInfo.m_timeStep = timeStep;
				dispatchInfo.m_stepCount = 0;
				dispatchInfo.m_dispatchFunc = DispatcherInfo::DISPATCH_CONTINUOUS;
				
				scene->DispatchAllCollisionPairs( *m_dispatcher,dispatchInfo);///numsubstep,g);
				toi = dispatchInfo.m_timeOfImpact;
			}
			
			//
			// integrating solution
			//
			
			{
				std::vector<CcdPhysicsController*>::iterator i;
				
				for (i=m_controllers.begin();
				!(i==m_controllers.end()); i++)
				{
					
					CcdPhysicsController* ctrl = *i;
					
					SimdTransform predictedTrans;
					RigidBody* body = ctrl->GetRigidBody();
					if (body->GetActivationState() != ISLAND_SLEEPING)
					{
						body->predictIntegratedTransform(timeStep*	toi, predictedTrans);
						body->proceedToTransform( predictedTrans);

					}
				}
				
			}
			
			
			
			
			
			//
			// disable sleeping physics objects
			//
			
			std::vector<CcdPhysicsController*> m_sleepingControllers;
			
			for (i=m_controllers.begin();
			!(i==m_controllers.end()); i++)
			{
				CcdPhysicsController* ctrl = (*i);
				RigidBody* body = ctrl->GetRigidBody();
				
				if (ctrl->wantsSleeping())
				{
					if (body->GetActivationState() == ACTIVE_TAG)
						body->SetActivationState( WANTS_DEACTIVATION );
				} else
				{
					body->SetActivationState( ACTIVE_TAG );
				}

				if (useIslands)
				{
					if (body->GetActivationState() == ISLAND_SLEEPING)
					{
						m_sleepingControllers.push_back(ctrl);
					}
				} else
				{
					if (ctrl->wantsSleeping())
					{
						m_sleepingControllers.push_back(ctrl);
					}
				}
			}
			
	
			
			
	}
	
	SyncMotionStates(timeStep);

	}
	return true;
}

void		CcdPhysicsEnvironment::setDebugMode(int debugMode)
{
	if (debugMode > 10)
	{
		if (m_dispatcher)
			delete m_dispatcher;

		if (debugMode == 11)
		{
			SimpleConstraintSolver* solver= new SimpleConstraintSolver();
			m_dispatcher = new ToiContactDispatcher(solver);
		} else
		{
			OdeConstraintSolver* solver = new OdeConstraintSolver();
			m_dispatcher = new ToiContactDispatcher(solver);
		}
	}
	if (m_debugDrawer){
		m_debugDrawer->SetDebugMode(debugMode);
	}
}


void	CcdPhysicsEnvironment::SyncMotionStates(float timeStep)
{
	std::vector<CcdPhysicsController*>::iterator i;

	//
	// synchronize the physics and graphics transformations
	//
	for (i=m_controllers.begin();
	!(i==m_controllers.end()); i++)
	{
		CcdPhysicsController* ctrl = (*i);
		ctrl->SynchronizeMotionStates(timeStep);
		
	}

}
void		CcdPhysicsEnvironment::setGravity(float x,float y,float z)
{
	m_gravity = SimdVector3(x,y,z);

	std::vector<CcdPhysicsController*>::iterator i;

	//todo: review this gravity stuff
	for (i=m_controllers.begin();
	!(i==m_controllers.end()); i++)
	{

		CcdPhysicsController* ctrl = (*i);
		ctrl->GetRigidBody()->setGravity(m_gravity);

	}
}

#ifdef DASHDASJKHASDJK
class RaycastingQueryBox : public QueryBox
{
	
	SimdVector3 m_aabbMin;
	
	SimdVector3 m_aabbMax;
	
	
	
public:
	
	RaycastCallback	m_raycastCallback;
	
	
	RaycastingQueryBox(QueryBoxConstructionInfo& ci,const SimdVector3& from,const SimdVector3& to)
		: QueryBox(ci),
		m_raycastCallback(from,to)
	{
		for (int i=0;i<3;i++)
		{
			float fromI = from[i];
			float toI = to[i];
			if (fromI < toI)
			{
				m_aabbMin[i] = fromI;
				m_aabbMax[i] = toI;
			} else
			{
				m_aabbMin[i] = toI;
				m_aabbMax[i] = fromI;
			}
		}
		
	}
	virtual void AddCollider( BroadphaseProxy* proxy)
	{
		//perform raycast if wanted, and update the m_hitFraction
		
		if (proxy->GetClientObjectType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
		{
			//do it
			RigidBody* body = (RigidBody*)proxy->m_clientObject;
			TriangleMeshInterface* meshInterface = (TriangleMeshInterface*)
				body->m_minkowski1;
			
			//if the hit is closer, record the proxy!
			float curFraction = m_raycastCallback.m_hitFraction;
			
			meshInterface->ProcessAllTriangles(&m_raycastCallback,m_aabbMin,m_aabbMax);
			
			if (m_raycastCallback.m_hitFraction < curFraction)
			{
				m_raycastCallback.m_hitProxy = proxy;
			}
			
		}
		
	}
};

struct InternalVehicleRaycaster : public VehicleRaycaster
{
	
	CcdPhysicsEnvironment* m_env;
	
public:
	
	InternalVehicleRaycaster(CcdPhysicsEnvironment* env)
		:	m_env(env)
	{
		
	}
	
	virtual void* CastRay(const SimdVector3& from,const SimdVector3& to, VehicleRaycasterResult& result)
	{

		return 0;
	}
	
};

#endif 
int			CcdPhysicsEnvironment::createConstraint(class PHY_IPhysicsController* ctrl0,class PHY_IPhysicsController* ctrl1,PHY_ConstraintType type,
														float pivotX,float pivotY,float pivotZ,
														float axisX,float axisY,float axisZ)
{
	
	
	CcdPhysicsController* c0 = (CcdPhysicsController*)ctrl0;
	CcdPhysicsController* c1 = (CcdPhysicsController*)ctrl1;
	
	RigidBody* rb0 = c0 ? c0->GetRigidBody() : 0;
	RigidBody* rb1 = c1 ? c1->GetRigidBody() : 0;
	
	ASSERT(rb0);
	
	SimdVector3 pivotInA(pivotX,pivotY,pivotZ);
	SimdVector3 pivotInB = rb1 ? rb1->getCenterOfMassTransform().inverse()(rb0->getCenterOfMassTransform()(pivotInA)) : pivotInA;
	
	switch (type)
	{
	case PHY_POINT2POINT_CONSTRAINT:
		{
			
			Point2PointConstraint* p2p = 0;
			
			if (rb1)
			{
				p2p = new Point2PointConstraint(*rb0,
					*rb1,pivotInA,pivotInB);
			} else
			{
				p2p = new Point2PointConstraint(*rb0,
					pivotInA);
			}
			
			m_p2pConstraints.push_back(p2p);
			return 0;
			
			break;
		}
	default:
		{
		}
	};
	
	//RigidBody& rbA,RigidBody& rbB, const SimdVector3& pivotInA,const SimdVector3& pivotInB
	
	return 0;
	
}

void		CcdPhysicsEnvironment::removeConstraint(int constraintid)
{
	
	Point2PointConstraint* p2p = (Point2PointConstraint*) constraintid;
	
	std::vector<Point2PointConstraint*>::iterator i =
		std::find(m_p2pConstraints.begin(), m_p2pConstraints.end(), p2p);
	
	if (!(i == m_p2pConstraints.end()) )
	{
		std::swap(*i, m_p2pConstraints.back());
		m_p2pConstraints.pop_back();
	}
	
}
PHY_IPhysicsController* CcdPhysicsEnvironment::rayTest(PHY_IPhysicsController* ignoreClient, float fromX,float fromY,float fromZ, float toX,float toY,float toZ, 
								float& hitX,float& hitY,float& hitZ,float& normalX,float& normalY,float& normalZ)
{


//	m_broadphase->cast(
	return 0;
}



int	CcdPhysicsEnvironment::getNumContactPoints()
{
	return 0;
}

void CcdPhysicsEnvironment::getContactPoint(int i,float& hitX,float& hitY,float& hitZ,float& normalX,float& normalY,float& normalZ)
{
	
}





Dispatcher* CcdPhysicsEnvironment::GetDispatcher()
{
	return m_dispatcher;
}

CcdPhysicsEnvironment::~CcdPhysicsEnvironment()
{
	
	
	m_vehicles.clear();
	
	//m_broadphase->DestroyScene();
	//delete broadphase ? release reference on broadphase ?
	
	//first delete scene, then dispatcher, because pairs have to release manifolds on the dispatcher
	delete m_dispatcher;
	
}


int	CcdPhysicsEnvironment::GetNumControllers()
{
	return m_controllers.size();
}


CcdPhysicsController* CcdPhysicsEnvironment::GetPhysicsController( int index)
{
	return m_controllers[index];
}


int	CcdPhysicsEnvironment::GetNumManifolds() const
{
	return m_dispatcher->GetNumManifolds();
}

const PersistentManifold*	CcdPhysicsEnvironment::GetManifold(int index) const
{
	return m_dispatcher->GetManifoldByIndexInternal(index);
}
