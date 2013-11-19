/*
* Copyright (c) 2006-2009 Erin Catto http://www.gphysics.com
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies.
* Erin Catto makes no representations about the suitability
* of this software for any purpose.
* It is provided "as is" without express or implied warranty.
*/

#include "BoxLite.h"

namespace Box2D
{

	int Collide( Contact* contacts, Body* body1, Body* body2 );

	Arbiter::Arbiter( Body* b1, Body* b2 )
	{
		body1 = ( b1 < b2 ) ? b1 : b2;
		body2 = ( b1 < b2 ) ? b2 : b1;

		numContacts = Collide( contacts, body1, body2 );

		friction = sqrtf( body1->friction * body2->friction );
	}

	void Arbiter::Update( Contact* newContacts, int numNewContacts )
	{
		Contact mergedContacts[2];

		for ( int i = 0; i < numNewContacts; ++i )
		{
			Contact* cNew = newContacts + i;
			int k = -1;

			for ( int j = 0; j < numContacts; ++j )
			{
				Contact* cOld = contacts + j;

				if ( cNew->feature.value == cOld->feature.value )
				{
					k = j;
					break;
				}
			}

			if ( k > -1 )
			{
				Contact* c = mergedContacts + i;
				Contact* cOld = contacts + k;
				*c = *cNew;

				if ( World::warmStarting )
				{
					c->Pn = cOld->Pn;
					c->Pt = cOld->Pt;
					c->Pnb = cOld->Pnb;
				}
				else
				{
					c->Pn = 0.0f;
					c->Pt = 0.0f;
					c->Pnb = 0.0f;
				}
			}
			else
			{
				mergedContacts[i] = newContacts[i];
			}
		}

		for ( int i = 0; i < numNewContacts; ++i ) { contacts[i] = mergedContacts[i]; }

		numContacts = numNewContacts;
	}

	void Arbiter::PreStep( float inv_dt )
	{
		const float k_allowedPenetration = 0.01f;
		float k_biasFactor = World::positionCorrection ? 0.2f : 0.0f;

		for ( int i = 0; i < numContacts; ++i )
		{
			Contact* c = contacts + i;

			Vec2 r1 = c->position - body1->position;
			Vec2 r2 = c->position - body2->position;

			// Precompute normal mass, tangent mass, and bias.
			float rn1 = Dot( r1, c->normal ), rn2 = Dot( r2, c->normal );
			float kNormal = body1->invMass + body2->invMass;
			kNormal += body1->invI * ( Dot( r1, r1 ) - rn1 * rn1 ) + body2->invI * ( Dot( r2, r2 ) - rn2 * rn2 );
			c->massNormal = 1.0f / kNormal;

			Vec2 tangent = Cross( c->normal, 1.0f );
			float rt1 = Dot( r1, tangent ), rt2 = Dot( r2, tangent );
			float kTangent = body1->invMass + body2->invMass;
			kTangent += body1->invI * ( Dot( r1, r1 ) - rt1 * rt1 ) + body2->invI * ( Dot( r2, r2 ) - rt2 * rt2 );
			c->massTangent = 1.0f /  kTangent;

			c->bias = -k_biasFactor * inv_dt * Min( 0.0f, c->separation + k_allowedPenetration );

			if ( World::accumulateImpulses )
			{
				// Apply normal + friction impulse
				Vec2 P = c->Pn * c->normal + c->Pt * tangent;

				body1->velocity -= body1->invMass * P;
				body1->angularVelocity -= body1->invI * Cross( r1, P );

				body2->velocity += body2->invMass * P;
				body2->angularVelocity += body2->invI * Cross( r2, P );
			}
		}
	}

	void Arbiter::ApplyImpulse()
	{
		Body* b1 = body1;
		Body* b2 = body2;

		for ( int i = 0; i < numContacts; ++i )
		{
			Contact* c = contacts + i;
			c->r1 = c->position - b1->position;
			c->r2 = c->position - b2->position;

			// Relative velocity at contact
			Vec2 dv = b2->velocity + Cross( b2->angularVelocity, c->r2 ) - b1->velocity - Cross( b1->angularVelocity, c->r1 );

			// Compute normal impulse
			float vn = Dot( dv, c->normal );
			float dPn = c->massNormal * ( -vn + c->bias );

			if ( World::accumulateImpulses )
			{
				// Clamp the accumulated impulse
				float Pn0 = c->Pn;
				c->Pn = Max( Pn0 + dPn, 0.0f );
				dPn = c->Pn - Pn0;
			}
			else
			{
				dPn = Max( dPn, 0.0f );
			}

			// Apply contact impulse
			Vec2 Pn = dPn * c->normal;

			b1->velocity -= b1->invMass * Pn;
			b1->angularVelocity -= b1->invI * Cross( c->r1, Pn );

			b2->velocity += b2->invMass * Pn;
			b2->angularVelocity += b2->invI * Cross( c->r2, Pn );

			// Relative velocity at contact
			dv = b2->velocity + Cross( b2->angularVelocity, c->r2 ) - b1->velocity - Cross( b1->angularVelocity, c->r1 );

			Vec2 tangent = Cross( c->normal, 1.0f );
			float vt = Dot( dv, tangent );
			float dPt = c->massTangent * ( -vt );

			if ( World::accumulateImpulses )
			{
				// Compute friction impulse
				float maxPt = friction * c->Pn;

				// Clamp friction
				float oldTangentImpulse = c->Pt;
				c->Pt = Clamp( oldTangentImpulse + dPt, -maxPt, maxPt );
				dPt = c->Pt - oldTangentImpulse;
			}
			else
			{
				float maxPt = friction * dPn;
				dPt = Clamp( dPt, -maxPt, maxPt );
			}

			// Apply contact impulse
			Vec2 Pt = dPt * tangent;

			b1->velocity -= b1->invMass * Pt;
			b1->angularVelocity -= b1->invI * Cross( c->r1, Pt );

			b2->velocity += b2->invMass * Pt;
			b2->angularVelocity += b2->invI * Cross( c->r2, Pt );
		}
	}

	void Body::Set( const Vec2& w, float m )
	{
		width = w;
		mass = m;

		invMass = ( mass < FLT_MAX ) ? 1.0f / mass : 0.0f;
		I = ( mass < FLT_MAX ) ? mass * ( width.x * width.x + width.y * width.y ) / 12.0f : FLT_MAX;
		invI = ( mass < FLT_MAX ) ? 1.0f / I : 0.0f;
	}

// Box vertex and edge numbering:
//
//       ^ y
//       |
//       e1
//   v2 ---- v1
// e2 |       | e4 --> x
//   v3 ---- v4
//       e3

	enum Axis { FACE_A_X, FACE_A_Y, FACE_B_X, FACE_B_Y };

	enum EdgeNumbers { NO_EDGE = 0, EDGE1, EDGE2, EDGE3, EDGE4 };

	struct ClipVertex
	{
		ClipVertex() { fp.value = 0; }
		Vec2 v;
		FeaturePair fp;
	};

	inline void Box_Swap( char& a, char& b ) { char tmp = a; a = b; b = tmp; }

	inline void Flip( FeaturePair& fp ) { Box_Swap( fp.e.inEdge1, fp.e.inEdge2 ); Box_Swap( fp.e.outEdge1, fp.e.outEdge2 ); }

	int ClipSegmentToLine( ClipVertex vOut[2], ClipVertex vIn[2], const Vec2& normal, float offset, char clipEdge )
	{
		// Start with no output points
		int numOut = 0;

		// Calculate the distance of end points to the line
		float distance0 = Dot( normal, vIn[0].v ) - offset;
		float distance1 = Dot( normal, vIn[1].v ) - offset;

		// If the points are behind the plane
		if ( distance0 <= 0.0f ) { vOut[numOut++] = vIn[0]; }

		if ( distance1 <= 0.0f ) { vOut[numOut++] = vIn[1]; }

		// If the points are on different sides of the plane
		if ( distance0 * distance1 < 0.0f )
		{
			// Find intersection point of edge and plane
			float interp = distance0 / ( distance0 - distance1 );
			vOut[numOut].v = vIn[0].v + interp * ( vIn[1].v - vIn[0].v );

			if ( distance0 > 0.0f )
			{
				vOut[numOut].fp = vIn[0].fp;
				vOut[numOut].fp.e.inEdge1 = clipEdge;
				vOut[numOut].fp.e.inEdge2 = NO_EDGE;
			}
			else
			{
				vOut[numOut].fp = vIn[1].fp;
				vOut[numOut].fp.e.outEdge1 = clipEdge;
				vOut[numOut].fp.e.outEdge2 = NO_EDGE;
			}

			++numOut;
		}

		return numOut;
	}

	static void ComputeIncidentEdge( ClipVertex c[2], const Vec2& h, const Vec2& pos, const Mat22& Rot, const Vec2& normal )
	{
		// The normal is from the reference box. Convert it
		// to the incident box's frame and flip sign.
		Mat22 RotT = Rot.Transpose();
		Vec2 n = -( RotT * normal );
		Vec2 nAbs = Abs( n );

		if ( nAbs.x > nAbs.y )
		{
			if ( n.x >= 0.0f )
			{
				c[0].v.Set( h.x, -h.y );
				c[0].fp.e.inEdge2 = EDGE3;
				c[0].fp.e.outEdge2 = EDGE4;

				c[1].v.Set( h.x, h.y );
				c[1].fp.e.inEdge2 = EDGE4;
				c[1].fp.e.outEdge2 = EDGE1;
			}
			else
			{
				c[0].v.Set( -h.x, h.y );
				c[0].fp.e.inEdge2 = EDGE1;
				c[0].fp.e.outEdge2 = EDGE2;

				c[1].v.Set( -h.x, -h.y );
				c[1].fp.e.inEdge2 = EDGE2;
				c[1].fp.e.outEdge2 = EDGE3;
			}
		}
		else
		{
			if ( n.y >= 0.0f )
			{
				c[0].v.Set( h.x, h.y );
				c[0].fp.e.inEdge2 = EDGE4;
				c[0].fp.e.outEdge2 = EDGE1;

				c[1].v.Set( -h.x, h.y );
				c[1].fp.e.inEdge2 = EDGE1;
				c[1].fp.e.outEdge2 = EDGE2;
			}
			else
			{
				c[0].v.Set( -h.x, -h.y );
				c[0].fp.e.inEdge2 = EDGE2;
				c[0].fp.e.outEdge2 = EDGE3;

				c[1].v.Set( h.x, -h.y );
				c[1].fp.e.inEdge2 = EDGE3;
				c[1].fp.e.outEdge2 = EDGE4;
			}
		}

		c[0].v = pos + Rot * c[0].v;
		c[1].v = pos + Rot * c[1].v;
	}

	// The normal points from A to B
	int Collide( Contact* contacts, Body* bodyA, Body* bodyB )
	{
		// Setup
		Vec2 hA = 0.5f * bodyA->width, hB = 0.5f * bodyB->width;
		Vec2 posA = bodyA->position, posB = bodyB->position;

		Mat22 RotA( bodyA->rotation ), RotB( bodyB->rotation );

		Mat22 RotAT = RotA.Transpose();
		Mat22 RotBT = RotB.Transpose();

		Vec2 dp = posB - posA;
		Vec2 dA = RotAT * dp, dB = RotBT * dp;

		Mat22 C = RotAT * RotB;
		Mat22 absC = Abs( C );

		// Box A faces
		Vec2 faceA = Abs( dA ) - hA - absC * hB;

		if ( faceA.x > 0.0f || faceA.y > 0.0f ) { return 0; }

		// Box B faces
		Vec2 faceB = Abs( dB ) - absC.Transpose() * hA - hB;

		if ( faceB.x > 0.0f || faceB.y > 0.0f ) { return 0; }

		// Find best axis
		const float relativeTol = 0.95f;
		const float absoluteTol = 0.01f;

		Axis axis = FACE_A_X;
		float separation = faceA.x;
		Vec2 normal = dA.x > 0.0f ? RotA.col1 : -RotA.col1;

		// Box A faces
		if ( faceA.y > relativeTol * separation + absoluteTol * hA.y )
		{
			axis = FACE_A_Y;
			separation = faceA.y;
			normal = dA.y > 0.0f ? RotA.col2 : -RotA.col2;
		}

		// Box B faces
		if ( faceB.x > relativeTol * separation + absoluteTol * hB.x )
		{
			axis = FACE_B_X;
			separation = faceB.x;
			normal = dB.x > 0.0f ? RotB.col1 : -RotB.col1;
		}

		if ( faceB.y > relativeTol * separation + absoluteTol * hB.y )
		{
			axis = FACE_B_Y;
			separation = faceB.y;
			normal = dB.y > 0.0f ? RotB.col2 : -RotB.col2;
		}

		// Setup clipping plane data based on the separating axis
		Vec2 frontNormal, sideNormal;
		ClipVertex incidentEdge[2];
		float front = 0.0f;
		float negSide = 0.0f;
		float posSide = 0.0f;
		char negEdge = 0, posEdge = 0;
		float side;

		// Compute the clipping lines and the line segment to be clipped.
		switch ( axis )
		{
			case FACE_A_X:
				frontNormal = normal;
				front = Dot( posA, frontNormal ) + hA.x;
				sideNormal = RotA.col2;
				side = Dot( posA, sideNormal );
				negSide = -side + hA.y;
				posSide =  side + hA.y;
				negEdge = EDGE3;
				posEdge = EDGE1;
				ComputeIncidentEdge( incidentEdge, hB, posB, RotB, frontNormal );
				break;

			case FACE_A_Y:
				frontNormal = normal;
				front = Dot( posA, frontNormal ) + hA.y;
				sideNormal = RotA.col1;
				side = Dot( posA, sideNormal );
				negSide = -side + hA.x;
				posSide =  side + hA.x;
				negEdge = EDGE2;
				posEdge = EDGE4;
				ComputeIncidentEdge( incidentEdge, hB, posB, RotB, frontNormal );
				break;

			case FACE_B_X:
				frontNormal = -normal;
				front = Dot( posB, frontNormal ) + hB.x;
				sideNormal = RotB.col2;
				side = Dot( posB, sideNormal );
				negSide = -side + hB.y;
				posSide =  side + hB.y;
				negEdge = EDGE3;
				posEdge = EDGE1;
				ComputeIncidentEdge( incidentEdge, hA, posA, RotA, frontNormal );
				break;

			case FACE_B_Y:
				frontNormal = -normal;
				front = Dot( posB, frontNormal ) + hB.y;
				sideNormal = RotB.col1;
				side = Dot( posB, sideNormal );
				negSide = -side + hB.x;
				posSide =  side + hB.x;
				negEdge = EDGE2;
				posEdge = EDGE4;
				ComputeIncidentEdge( incidentEdge, hA, posA, RotA, frontNormal );
				break;
		}

		// clip other face with 5 box planes (1 face plane, 4 edge planes)
		ClipVertex clipPoints1[2], clipPoints2[2];
		int np;

		// Clip to box side 1
		np = ClipSegmentToLine( clipPoints1, incidentEdge, -sideNormal, negSide, negEdge );

		if ( np < 2 ) { return 0; }

		// Clip to negative box side 1
		np = ClipSegmentToLine( clipPoints2, clipPoints1,  sideNormal, posSide, posEdge );

		if ( np < 2 ) { return 0; }

		// Now clipPoints2 contains the clipping points.
		// Due to roundoff, it is possible that clipping removes all points.

		int numContacts = 0;

		for ( int i = 0; i < 2; ++i )
		{
			float separation = Dot( frontNormal, clipPoints2[i].v ) - front;

			if ( separation > 0 ) { continue; }

			contacts[numContacts].separation = separation;
			contacts[numContacts].normal = normal;

			// Slide contact point onto reference face (easy to cull)
			contacts[numContacts].position = clipPoints2[i].v - separation * frontNormal;
			contacts[numContacts].feature = clipPoints2[i].fp;

			if ( axis == FACE_B_X || axis == FACE_B_Y ) { Flip( contacts[numContacts].feature ); }

			++numContacts;
		}

		return numContacts;
	}

	void Joint::Set( Body* b1, Body* b2, const Vec2& anchor )
	{
		body1 = b1;
		body2 = b2;

		P.Set( 0.0f, 0.0f );

		localAnchor1 = Mat22( body1->rotation ).Transpose() * ( anchor - body1->position );
		localAnchor2 = Mat22( body2->rotation ).Transpose() * ( anchor - body2->position );
	}

	void Joint::PreStep( float inv_dt )
	{
		// Pre-compute anchors, mass matrix, and bias.
		r1 = Mat22( body1->rotation ) * localAnchor1;
		r2 = Mat22( body2->rotation ) * localAnchor2;

		// deltaV = deltaV0 + K * impulse
		// invM = [(1/m1 + 1/m2) * eye(2) - skew(r1) * invI1 * skew(r1) - skew(r2) * invI2 * skew(r2)]
		//      = [1/m1+1/m2     0    ] + invI1 * [r1.y*r1.y -r1.x*r1.y] + invI2 * [r1.y*r1.y -r1.x*r1.y]
		//        [    0     1/m1+1/m2]           [-r1.x*r1.y r1.x*r1.x]           [-r1.x*r1.y r1.x*r1.x]
		Mat22 K1;
		K1.col1.x = body1->invMass + body2->invMass;
		K1.col2.x = 0.0f;
		K1.col1.y = 0.0f;
		K1.col2.y = body1->invMass + body2->invMass;

		Mat22 K2;
		K2.col1.x =  body1->invI * r1.y * r1.y;
		K2.col2.x = -body1->invI * r1.x * r1.y;
		K2.col1.y = -body1->invI * r1.x * r1.y;
		K2.col2.y =  body1->invI * r1.x * r1.x;

		Mat22 K3;
		K3.col1.x =  body2->invI * r2.y * r2.y;
		K3.col2.x = -body2->invI * r2.x * r2.y;
		K3.col1.y = -body2->invI * r2.x * r2.y;
		K3.col2.y =  body2->invI * r2.x * r2.x;

		Mat22 K = K1 + K2 + K3;
		K.col1.x += softness;
		K.col2.y += softness;

		M = K.Invert();

		Vec2 p1 = body1->position + r1;
		Vec2 p2 = body2->position + r2;
		Vec2 dp = p2 - p1;

		bias = ( World::positionCorrection ) ? -biasFactor * inv_dt * dp : Vec2( 0, 0 );

		if ( !World::warmStarting ) { P.Set( 0.0f, 0.0f ); return; }

		// Apply accumulated impulse.
		body1->velocity -= body1->invMass * P;
		body1->angularVelocity -= body1->invI * Cross( r1, P );

		body2->velocity += body2->invMass * P;
		body2->angularVelocity += body2->invI * Cross( r2, P );
	}

	void Joint::ApplyImpulse()
	{
		Vec2 dv = body2->velocity + Cross( body2->angularVelocity, r2 ) - body1->velocity - Cross( body1->angularVelocity, r1 );

		Vec2 impulse = M * ( bias - dv - softness * P );

		body1->velocity -= body1->invMass * impulse;
		body1->angularVelocity -= body1->invI * Cross( r1, impulse );

		body2->velocity += body2->invMass * impulse;
		body2->angularVelocity += body2->invI * Cross( r2, impulse );

		P += impulse;
	}

	typedef std::map<ArbiterKey, Arbiter>::iterator ArbIter;

	bool World::accumulateImpulses = true;
	bool World::warmStarting = true;
	bool World::positionCorrection = true;

	void World::BroadPhase()
	{
		double InsertStart = 0.0;
		double EraseStart = 0.0;
		double IntersectStart = 0.0;

		InsertArbiterTime = 0;
		EraseArbiterTime = 0;
		IntersectTime = 0;

		int i, j, sz = ( int )bodies.size();

		// O(n^2) broad-phase
		for ( i = 0; i < sz; ++i )
		{
			Body* bi = bodies[i];

			for ( j = i + 1; j < sz; ++j )
			{
				Body* bj = bodies[j];

				if ( bi->invMass == 0.0f && bj->invMass == 0.0f ) { continue; }

				if ( TimingFunction ) { IntersectStart = TimingFunction(); }

				Arbiter newArb( bi, bj );
				ArbiterKey key( bi, bj );

				if ( TimingFunction ) { IntersectStart = TimingFunction() - IntersectStart; IntersectTime += IntersectStart; }

				if ( newArb.numContacts > 0 )
				{
					if ( TimingFunction ) { InsertStart = TimingFunction(); }

					ArbIter iter = arbiters.find( key );

					if ( iter == arbiters.end() )
					{
						arbiters.insert( std::pair<ArbiterKey, Arbiter>( key, newArb ) );

						if ( NewCollisionCallback ) { NewCollisionCallback( CollisionUserData, &newArb ); }
					}
					else
					{
						iter->second.Update( newArb.contacts, newArb.numContacts );

						if ( CollisionCallback ) { CollisionCallback( CollisionUserData, &newArb ); }
					}

					if ( TimingFunction ) { InsertStart = TimingFunction() - InsertStart; InsertArbiterTime += InsertStart; }
				}
				else
				{
					if ( TimingFunction ) { EraseStart = TimingFunction(); }

					if ( arbiters.erase( key ) > 0 )

						// contact ended
						if ( EndCollisionCallback ) { EndCollisionCallback( CollisionUserData, &newArb ); }

					if ( TimingFunction ) { EraseStart = TimingFunction() - EraseStart; EraseArbiterTime += EraseStart; }
				}
			}
		}
	}

	void World::Step( float dt )
	{
		float inv_dt = dt > 0.0f ? 1.0f / dt : 0.0f;

		if ( TimingFunction ) { BroadPhaseTime = TimingFunction(); }

		BroadPhase();

		if ( TimingFunction ) { BroadPhaseTime = TimingFunction() - BroadPhaseTime; }

		for ( int i = 0; i < ( int )bodies.size(); ++i ) // forces
		{
			Body* b = bodies[i];

			if ( b->invMass == 0.0f ) { continue; }

			b->velocity += dt * ( gravity + b->invMass * b->force );
			b->angularVelocity += dt * b->invI * b->torque;
		}

		if ( TimingFunction ) { PreStepTime = TimingFunction(); }

		// pre-steps.
		for ( ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb ) { arb->second.PreStep( inv_dt ); }

		for ( int i = 0; i < ( int )joints.size(); ++i ) { joints[i]->PreStep( inv_dt ); }

		if ( TimingFunction ) { PreStepTime = TimingFunction() - PreStepTime; }

		if ( TimingFunction ) { CollisionResolveTime = TimingFunction(); }

		for ( int i = 0; i < iterations; ++i ) // iterations
		{
			for ( ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb ) { arb->second.ApplyImpulse(); }

			for ( int j = 0; j < ( int )joints.size(); ++j ) { joints[j]->ApplyImpulse(); }
		}

		if ( TimingFunction ) { CollisionResolveTime = TimingFunction() - CollisionResolveTime; }

		if ( TimingFunction ) { VelocityStepTime = TimingFunction(); }

		for ( int i = 0; i < ( int )bodies.size(); ++i ) // velocity step
		{
			Body* b = bodies[i];

			b->position += dt * b->velocity;
			b->rotation += dt * b->angularVelocity;

			b->force.Set( 0.0f, 0.0f );
			b->torque = 0.0f;
		}

		if ( TimingFunction ) { VelocityStepTime = TimingFunction() - VelocityStepTime; }
	}

} // namespace Box2D
