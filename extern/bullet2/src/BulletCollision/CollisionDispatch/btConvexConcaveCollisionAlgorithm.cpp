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


#include "btConvexConcaveCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "btConvexConvexAlgorithm.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"
#include "BulletCollision/CollisionShapes/btConcaveShape.h"
#include "BulletCollision/CollisionDispatch/btManifoldResult.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.h"

btConvexConcaveCollisionAlgorithm::btConvexConcaveCollisionAlgorithm( const btCollisionAlgorithmConstructionInfo& ci, btCollisionObject* body0,btCollisionObject* body1,bool isSwapped)
: btCollisionAlgorithm(ci),
m_isSwapped(isSwapped),
m_btConvexTriangleCallback(ci.m_dispatcher,body0,body1,isSwapped)
{
}

btConvexConcaveCollisionAlgorithm::~btConvexConcaveCollisionAlgorithm()
{
}



btConvexTriangleCallback::btConvexTriangleCallback(btDispatcher*  dispatcher,btCollisionObject* body0,btCollisionObject* body1,bool isSwapped):
	  m_dispatcher(dispatcher),
	m_dispatchInfoPtr(0)
{
	m_convexBody = isSwapped? body1:body0;
	m_triBody = isSwapped? body0:body1;
	
	  //
	  // create the manifold from the dispatcher 'manifold pool'
	  //
	  m_manifoldPtr = m_dispatcher->getNewManifold(m_convexBody,m_triBody);

  	  clearCache();
}

btConvexTriangleCallback::~btConvexTriangleCallback()
{
	clearCache();
	m_dispatcher->releaseManifold( m_manifoldPtr );
  
}
  

void	btConvexTriangleCallback::clearCache()
{
	m_dispatcher->clearManifold(m_manifoldPtr);
};



void btConvexTriangleCallback::processTriangle(btVector3* triangle,int partId, int triangleIndex)
{
 
	//just for debugging purposes
	//printf("triangle %d",m_triangleCount++);


	//aabb filter is already applied!	

	btCollisionAlgorithmConstructionInfo ci;
	ci.m_dispatcher = m_dispatcher;

	btCollisionObject* ob = static_cast<btCollisionObject*>(m_triBody);


	
	///debug drawing of the overlapping triangles
	if (m_dispatchInfoPtr && m_dispatchInfoPtr->m_debugDraw && m_dispatchInfoPtr->m_debugDraw->getDebugMode() > 0)
	{
		btVector3 color(255,255,0);
		btTransform& tr = ob->m_worldTransform;
		m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[0]),tr(triangle[1]),color);
		m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[1]),tr(triangle[2]),color);
		m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[2]),tr(triangle[0]),color);

		//btVector3 center = triangle[0] + triangle[1]+triangle[2];
		//center *= 0.333333f;
		//m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[0]),tr(center),color);
		//m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[1]),tr(center),color);
		//m_dispatchInfoPtr->m_debugDraw->drawLine(tr(triangle[2]),tr(center),color);

	}


	//btCollisionObject* colObj = static_cast<btCollisionObject*>(m_convexProxy->m_clientObject);
	
	if (m_convexBody->m_collisionShape->isConvex())
	{
		btTriangleShape tm(triangle[0],triangle[1],triangle[2]);	
		tm.setMargin(m_collisionMarginTriangle);
	
		
		btCollisionShape* tmpShape = ob->m_collisionShape;
		ob->m_collisionShape = &tm;
		
		///this should use the btDispatcher, so the actual registered algorithm is used
		btConvexConvexAlgorithm cvxcvxalgo(m_manifoldPtr,ci,m_convexBody,m_triBody);
		cvxcvxalgo.setShapeIdentifiers(-1,-1,partId,triangleIndex);
		cvxcvxalgo.processCollision(m_convexBody,m_triBody,*m_dispatchInfoPtr,m_resultOut);
		ob->m_collisionShape = tmpShape;

	}

	

}



void	btConvexTriangleCallback::setTimeStepAndCounters(float collisionMarginTriangle,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{
	m_dispatchInfoPtr = &dispatchInfo;
	m_collisionMarginTriangle = collisionMarginTriangle;
	m_resultOut = resultOut;

	//recalc aabbs
	btTransform convexInTriangleSpace;
	convexInTriangleSpace = m_triBody->m_worldTransform.inverse() * m_convexBody->m_worldTransform;
	btCollisionShape* convexShape = static_cast<btCollisionShape*>(m_convexBody->m_collisionShape);
	//CollisionShape* triangleShape = static_cast<btCollisionShape*>(triBody->m_collisionShape);
	convexShape->getAabb(convexInTriangleSpace,m_aabbMin,m_aabbMax);
	float extraMargin = collisionMarginTriangle;
	btVector3 extra(extraMargin,extraMargin,extraMargin);

	m_aabbMax += extra;
	m_aabbMin -= extra;
	
}

void btConvexConcaveCollisionAlgorithm::clearCache()
{
	m_btConvexTriangleCallback.clearCache();

}

void btConvexConcaveCollisionAlgorithm::processCollision (btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{
	
	
	btCollisionObject* convexBody = m_isSwapped ? body1 : body0;
	btCollisionObject* triBody = m_isSwapped ? body0 : body1;

	if (triBody->m_collisionShape->isConcave())
	{


		btCollisionObject*	triOb = triBody;
		ConcaveShape* concaveShape = static_cast<ConcaveShape*>( triOb->m_collisionShape);
		
		if (convexBody->m_collisionShape->isConvex())
		{
			float collisionMarginTriangle = concaveShape->getMargin();
					
			resultOut->setPersistentManifold(m_btConvexTriangleCallback.m_manifoldPtr);
			m_btConvexTriangleCallback.setTimeStepAndCounters(collisionMarginTriangle,dispatchInfo,resultOut);

			//Disable persistency. previously, some older algorithm calculated all contacts in one go, so you can clear it here.
			//m_dispatcher->clearManifold(m_btConvexTriangleCallback.m_manifoldPtr);

			m_btConvexTriangleCallback.m_manifoldPtr->setBodies(convexBody,triBody);

			concaveShape->processAllTriangles( &m_btConvexTriangleCallback,m_btConvexTriangleCallback.getAabbMin(),m_btConvexTriangleCallback.getAabbMax());
			
	
		}

	}

}


float btConvexConcaveCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{

	btCollisionObject* convexbody = m_isSwapped ? body1 : body0;
	btCollisionObject* triBody = m_isSwapped ? body0 : body1;


	//quick approximation using raycast, todo: hook up to the continuous collision detection (one of the btConvexCast)

	//only perform CCD above a certain treshold, this prevents blocking on the long run
	//because object in a blocked ccd state (hitfraction<1) get their linear velocity halved each frame...
	float squareMot0 = (convexbody->m_interpolationWorldTransform.getOrigin() - convexbody->m_worldTransform.getOrigin()).length2();
	if (squareMot0 < convexbody->m_ccdSquareMotionTreshold)
	{
		return 1.f;
	}

	//const btVector3& from = convexbody->m_worldTransform.getOrigin();
	//btVector3 to = convexbody->m_interpolationWorldTransform.getOrigin();
	//todo: only do if the motion exceeds the 'radius'

	btTransform triInv = triBody->m_worldTransform.inverse();
	btTransform convexFromLocal = triInv * convexbody->m_worldTransform;
	btTransform convexToLocal = triInv * convexbody->m_interpolationWorldTransform;

	struct LocalTriangleSphereCastCallback	: public btTriangleCallback
	{
		btTransform m_ccdSphereFromTrans;
		btTransform m_ccdSphereToTrans;
		btTransform	m_meshTransform;

		float	m_ccdSphereRadius;
		float	m_hitFraction;
	

		LocalTriangleSphereCastCallback(const btTransform& from,const btTransform& to,float ccdSphereRadius,float hitFraction)
			:m_ccdSphereFromTrans(from),
			m_ccdSphereToTrans(to),
			m_ccdSphereRadius(ccdSphereRadius),
			m_hitFraction(hitFraction)
		{			
		}
		
		
		virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex)
		{
			//do a swept sphere for now
			btTransform ident;
			ident.setIdentity();
			btConvexCast::CastResult castResult;
			castResult.m_fraction = m_hitFraction;
			btSphereShape	pointShape(m_ccdSphereRadius);
			btTriangleShape	triShape(triangle[0],triangle[1],triangle[2]);
			btVoronoiSimplexSolver	simplexSolver;
			btSubsimplexConvexCast convexCaster(&pointShape,&triShape,&simplexSolver);
			//GjkConvexCast	convexCaster(&pointShape,convexShape,&simplexSolver);
			//ContinuousConvexCollision convexCaster(&pointShape,convexShape,&simplexSolver,0);
			//local space?

			if (convexCaster.calcTimeOfImpact(m_ccdSphereFromTrans,m_ccdSphereToTrans,
				ident,ident,castResult))
			{
				if (m_hitFraction > castResult.m_fraction)
					m_hitFraction = castResult.m_fraction;
			}

		}

	};


	

	
	if (triBody->m_collisionShape->isConcave())
	{
		btVector3 rayAabbMin = convexFromLocal.getOrigin();
		rayAabbMin.setMin(convexToLocal.getOrigin());
		btVector3 rayAabbMax = convexFromLocal.getOrigin();
		rayAabbMax.setMax(convexToLocal.getOrigin());
		rayAabbMin -= btVector3(convexbody->m_ccdSweptSphereRadius,convexbody->m_ccdSweptSphereRadius,convexbody->m_ccdSweptSphereRadius);
		rayAabbMax += btVector3(convexbody->m_ccdSweptSphereRadius,convexbody->m_ccdSweptSphereRadius,convexbody->m_ccdSweptSphereRadius);

		float curHitFraction = 1.f; //is this available?
		LocalTriangleSphereCastCallback raycastCallback(convexFromLocal,convexToLocal,
		convexbody->m_ccdSweptSphereRadius,curHitFraction);

		raycastCallback.m_hitFraction = convexbody->m_hitFraction;

		btCollisionObject* concavebody = triBody;

		ConcaveShape* triangleMesh = (ConcaveShape*) concavebody->m_collisionShape;
		
		if (triangleMesh)
		{
			triangleMesh->processAllTriangles(&raycastCallback,rayAabbMin,rayAabbMax);
		}
	


		if (raycastCallback.m_hitFraction < convexbody->m_hitFraction)
		{
			convexbody->m_hitFraction = raycastCallback.m_hitFraction;
			return raycastCallback.m_hitFraction;
		}
	}

	return 1.f;

}
