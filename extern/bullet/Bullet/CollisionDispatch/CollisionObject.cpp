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

#include "CollisionObject.h"

CollisionObject::CollisionObject()
	:	m_collisionFlags(0),
		m_activationState1(1),
		m_deactivationTime(0.f),
		m_broadphaseHandle(0),
		m_collisionShape(0),
		m_hitFraction(1.f),
		m_ccdSweptShereRadius(0.f),
		m_ccdSquareMotionTreshold(0.f)
{
	m_cachedInvertedWorldTransform.setIdentity();

}


void CollisionObject::SetActivationState(int newState) 
{ 
	if ( (m_activationState1 != DISABLE_DEACTIVATION) && (m_activationState1 != DISABLE_SIMULATION))
		m_activationState1 = newState;
}

void CollisionObject::ForceActivationState(int newState)
{
	m_activationState1 = newState;
}

void CollisionObject::activate()
{
	if (!(m_collisionFlags & isStatic))
	{
		SetActivationState(1);
		m_deactivationTime = 0.f;
	}
}

bool CollisionObject::mergesSimulationIslands() const
{
	return ( !(m_collisionFlags & isStatic));
}
