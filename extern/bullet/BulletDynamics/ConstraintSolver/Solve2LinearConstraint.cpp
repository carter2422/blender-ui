/*
 * Copyright (c) 2005 Erwin Coumans http://www.erwincoumans.com
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erwin Coumans makes no representations about the suitability 
 * of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
*/

#include "Solve2LinearConstraint.h"

#include "Dynamics/RigidBody.h"
#include "SimdVector3.h"
#include "JacobianEntry.h"


void Solve2LinearConstraint::resolveUnilateralPairConstraint(
												   RigidBody* body1,
		RigidBody* body2,

						const SimdMatrix3x3& world2A,
						const SimdMatrix3x3& world2B,
						
						const SimdVector3& invInertiaADiag,
						const SimdScalar invMassA,
						const SimdVector3& linvelA,const SimdVector3& angvelA,
						const SimdVector3& rel_posA1,
						const SimdVector3& invInertiaBDiag,
						const SimdScalar invMassB,
						const SimdVector3& linvelB,const SimdVector3& angvelB,
						const SimdVector3& rel_posA2,

					  SimdScalar depthA, const SimdVector3& normalA, 
					  const SimdVector3& rel_posB1,const SimdVector3& rel_posB2,
					  SimdScalar depthB, const SimdVector3& normalB, 
					  SimdScalar& imp0,SimdScalar& imp1)
{

	imp0 = 0.f;
	imp1 = 0.f;

	SimdScalar len = fabs(normalA.length())-1.f;
	if (fabs(len) >= SIMD_EPSILON)
		return;

	ASSERT(len < SIMD_EPSILON);


	//this jacobian entry could be re-used for all iterations
	JacobianEntry jacA(world2A,world2B,rel_posA1,rel_posA2,normalA,invInertiaADiag,invMassA,
		invInertiaBDiag,invMassB);
	JacobianEntry jacB(world2A,world2B,rel_posB1,rel_posB2,normalB,invInertiaADiag,invMassA,
		invInertiaBDiag,invMassB);
	
	//const SimdScalar vel0 = jacA.getRelativeVelocity(linvelA,angvelA,linvelB,angvelB);
	//const SimdScalar vel1 = jacB.getRelativeVelocity(linvelA,angvelA,linvelB,angvelB);

	const SimdScalar vel0 = normalA.dot(body1->getVelocityInLocalPoint(rel_posA1)-body2->getVelocityInLocalPoint(rel_posA1));
	const SimdScalar vel1 = normalB.dot(body1->getVelocityInLocalPoint(rel_posB1)-body2->getVelocityInLocalPoint(rel_posB1));

//	SimdScalar penetrationImpulse = (depth*contactTau*timeCorrection)  * massTerm;//jacDiagABInv
	SimdScalar massTerm = 1.f / (invMassA + invMassB);


	// calculate rhs (or error) terms
	const SimdScalar dv0 = depthA  * m_tau * massTerm - vel0 * m_damping;
	const SimdScalar dv1 = depthB  * m_tau * massTerm - vel1 * m_damping;


	// dC/dv * dv = -C
	
	// jacobian * impulse = -error
	//

	//impulse = jacobianInverse * -error

	// inverting 2x2 symmetric system (offdiagonal are equal!)
	// 


	SimdScalar nonDiag = jacA.getNonDiagonal(jacB,invMassA,invMassB);
	SimdScalar	invDet = 1.0f / (jacA.getDiagonal() * jacB.getDiagonal() - nonDiag * nonDiag );
	
	//imp0 = dv0 * jacA.getDiagonal() * invDet + dv1 * -nonDiag * invDet;
	//imp1 = dv1 * jacB.getDiagonal() * invDet + dv0 * - nonDiag * invDet;

	imp0 = dv0 * jacA.getDiagonal() * invDet + dv1 * -nonDiag * invDet;
	imp1 = dv1 * jacB.getDiagonal() * invDet + dv0 * - nonDiag * invDet;

	//[a b]								  [d -c]
	//[c d] inverse = (1 / determinant) * [-b a] where determinant is (ad - bc)

	//[jA nD] * [imp0] = [dv0]
	//[nD jB]   [imp1]   [dv1]

}



void Solve2LinearConstraint::resolveBilateralPairConstraint(
						RigidBody* body1,
						RigidBody* body2,
						const SimdMatrix3x3& world2A,
						const SimdMatrix3x3& world2B,
						
						const SimdVector3& invInertiaADiag,
						const SimdScalar invMassA,
						const SimdVector3& linvelA,const SimdVector3& angvelA,
						const SimdVector3& rel_posA1,
						const SimdVector3& invInertiaBDiag,
						const SimdScalar invMassB,
						const SimdVector3& linvelB,const SimdVector3& angvelB,
						const SimdVector3& rel_posA2,

					  SimdScalar depthA, const SimdVector3& normalA, 
					  const SimdVector3& rel_posB1,const SimdVector3& rel_posB2,
					  SimdScalar depthB, const SimdVector3& normalB, 
					  SimdScalar& imp0,SimdScalar& imp1)
{

	imp0 = 0.f;
	imp1 = 0.f;

	SimdScalar len = fabs(normalA.length())-1.f;
	if (fabs(len) >= SIMD_EPSILON)
		return;

	ASSERT(len < SIMD_EPSILON);


	//this jacobian entry could be re-used for all iterations
	JacobianEntry jacA(world2A,world2B,rel_posA1,rel_posA2,normalA,invInertiaADiag,invMassA,
		invInertiaBDiag,invMassB);
	JacobianEntry jacB(world2A,world2B,rel_posB1,rel_posB2,normalB,invInertiaADiag,invMassA,
		invInertiaBDiag,invMassB);
	
	//const SimdScalar vel0 = jacA.getRelativeVelocity(linvelA,angvelA,linvelB,angvelB);
	//const SimdScalar vel1 = jacB.getRelativeVelocity(linvelA,angvelA,linvelB,angvelB);

	const SimdScalar vel0 = normalA.dot(body1->getVelocityInLocalPoint(rel_posA1)-body2->getVelocityInLocalPoint(rel_posA1));
	const SimdScalar vel1 = normalB.dot(body1->getVelocityInLocalPoint(rel_posB1)-body2->getVelocityInLocalPoint(rel_posB1));

	// calculate rhs (or error) terms
	const SimdScalar dv0 = depthA  * m_tau - vel0 * m_damping;
	const SimdScalar dv1 = depthB  * m_tau - vel1 * m_damping;

	// dC/dv * dv = -C
	
	// jacobian * impulse = -error
	//

	//impulse = jacobianInverse * -error

	// inverting 2x2 symmetric system (offdiagonal are equal!)
	// 


	SimdScalar nonDiag = jacA.getNonDiagonal(jacB,invMassA,invMassB);
	SimdScalar	invDet = 1.0f / (jacA.getDiagonal() * jacB.getDiagonal() - nonDiag * nonDiag );
	
	//imp0 = dv0 * jacA.getDiagonal() * invDet + dv1 * -nonDiag * invDet;
	//imp1 = dv1 * jacB.getDiagonal() * invDet + dv0 * - nonDiag * invDet;

	imp0 = dv0 * jacA.getDiagonal() * invDet + dv1 * -nonDiag * invDet;
	imp1 = dv1 * jacB.getDiagonal() * invDet + dv0 * - nonDiag * invDet;

	//[a b]								  [d -c]
	//[c d] inverse = (1 / determinant) * [-b a] where determinant is (ad - bc)

	//[jA nD] * [imp0] = [dv0]
	//[nD jB]   [imp1]   [dv1]

	if ( imp0 > 0.0f)
	{
		if ( imp1 > 0.0f )
		{
			//both positive
		}
		else
		{
			imp1 = 0.f;

			// now imp0>0 imp1<0
			imp0 = dv0 / jacA.getDiagonal();
			if ( imp0 > 0.0f )
			{
			} else
			{
				imp0 = 0.f;
			}
		}
	}
	else
	{
		imp0 = 0.f;

		imp1 = dv1 / jacB.getDiagonal();
		if ( imp1 <= 0.0f )
		{
			imp1 = 0.f;
			// now imp0>0 imp1<0
			imp0 = dv0 / jacA.getDiagonal();
			if ( imp0 > 0.0f )
			{
			} else
			{
				imp0 = 0.f;
			}
		} else
		{
		}
	}
}



void Solve2LinearConstraint::resolveAngularConstraint(	const SimdMatrix3x3& invInertiaAWS,
											const SimdScalar invMassA,
											const SimdVector3& linvelA,const SimdVector3& angvelA,
											const SimdVector3& rel_posA1,
											const SimdMatrix3x3& invInertiaBWS,
											const SimdScalar invMassB,
											const SimdVector3& linvelB,const SimdVector3& angvelB,
											const SimdVector3& rel_posA2,

											SimdScalar depthA, const SimdVector3& normalA, 
											const SimdVector3& rel_posB1,const SimdVector3& rel_posB2,
											SimdScalar depthB, const SimdVector3& normalB, 
											SimdScalar& imp0,SimdScalar& imp1)
{

}