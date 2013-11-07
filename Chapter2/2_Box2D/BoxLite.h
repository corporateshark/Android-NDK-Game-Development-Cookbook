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

#ifndef __BoxLite__h__included__
#define __BoxLite__h__included__

#include <math.h>
#include <assert.h>
#include <float.h>

#include <vector>
#include <map>

namespace Box2D
{

	struct Vec2
	{
		Vec2() {}
		Vec2( float x, float y ) : x( x ), y( y ) {}

		void Set( float x_, float y_ ) { x = x_; y = y_; }

		Vec2 operator -() { return Vec2( -x, -y ); }

		void operator += ( const Vec2& v ) { x += v.x; y += v.y; }
		void operator -= ( const Vec2& v ) { x -= v.x; y -= v.y; }

		float x, y;
	};

	struct Mat22
	{
		Mat22() {}
		Mat22( float angle )
		{
			float c = cosf( angle ), s = sinf( angle );
			col1.x = c;
			col2.x = -s;
			col1.y = s;
			col2.y = c;
		}

		Mat22( const Vec2& col1, const Vec2& col2 ) : col1( col1 ), col2( col2 ) {}

		Mat22 Transpose() const { return Mat22( Vec2( col1.x, col2.x ), Vec2( col1.y, col2.y ) ); }

		Mat22 Invert() const
		{
			float a = col1.x, b = col2.x, c = col1.y, d = col2.y, det = a * d - b * c;
			assert( det != 0.0f );
			det = 1.0f / det;
			return Mat22( Vec2( det * d, -det * c ), Vec2( -det * b, det * a ) );
		}

		Vec2 col1, col2;
	};

	inline float Dot  ( const Vec2& a, const Vec2& b ) { return a.x * b.x + a.y * b.y; }
	inline float Cross( const Vec2& a, const Vec2& b ) { return a.x * b.y - a.y * b.x; }

	inline Vec2 Cross( const Vec2& a, float s ) { return Vec2( s * a.y, -s * a.x ); }
	inline Vec2 Cross( float s, const Vec2& a ) { return Vec2( -s * a.y, s * a.x ); }

	inline Vec2 operator * ( float s, const Vec2& v ) { return Vec2( s * v.x, s * v.y ); }
	inline Vec2 operator * ( const Mat22& A, const Vec2& v ) { return Vec2( A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y ); }

	inline Vec2 operator + ( const Vec2& a, const Vec2& b ) { return Vec2( a.x + b.x, a.y + b.y ); }
	inline Vec2 operator - ( const Vec2& a, const Vec2& b ) { return Vec2( a.x - b.x, a.y - b.y ); }

	inline Mat22 operator + ( const Mat22& A, const Mat22& B ) { return Mat22( A.col1 + B.col1, A.col2 + B.col2 ); }
	inline Mat22 operator * ( const Mat22& A, const Mat22& B ) { return Mat22( A * B.col1, A * B.col2 ); }

	inline Vec2  Abs( const Vec2& a )  { return Vec2( fabsf( a.x ), fabsf( a.y ) ); }
	inline Mat22 Abs( const Mat22& A ) { return Mat22( Abs( A.col1 ), Abs( A.col2 ) ); }

	inline float Min( float a, float b ) { return a < b ? a : b; }
	inline float Max( float a, float b ) { return a > b ? a : b; }

	inline float Clamp( float a, float low, float high ) { return Max( low, Min( a, high ) ); }

	struct Body
	{
		Body(): tag( NULL ), position( 0, 0 ), rotation( 0 ), velocity( 0, 0 ), angularVelocity( 0 ), force( 0, 0 ), torque( 0 ) { Set( Vec2( 1.0f, 1.0f ), FLT_MAX ); }
		void Set( const Vec2& w, float m );

		void SetNull()
		{
			position.Set( 0.0f, 0.0f );
			rotation = 0.0f;
			velocity.Set( 0.0f, 0.0f );
			angularVelocity = 0.0f;
			force.Set( 0.0f, 0.0f );
			torque = 0.0f;
			friction = 0.2f;
		}

		void AddForce( const Vec2& f ) { force += f; }

		Vec2 width, position, velocity, force;
		float rotation, angularVelocity, torque;

		float friction;
		float mass, invMass, I, invI;

		void* tag;
	};

	struct Joint
	{
		Joint(): tag( NULL ), body1( 0 ), body2( 0 ), P( 0.0f, 0.0f ), biasFactor( 0.2f ), softness( 0.0f ) {}

		void Set( Body* body1, Body* body2, const Vec2& anchor );

		void SetNull()
		{
			softness = 0.0f;
			biasFactor = 0.2f;
		}

		void PreStep( float inv_dt );
		void ApplyImpulse();

		Mat22 M;
		Vec2 localAnchor1, localAnchor2;
		Vec2 r1, r2;
		Vec2 bias;
		Vec2 P;     // accumulated impulse
		Body* body1;
		Body* body2;
		float biasFactor;
		float softness;

		void* tag;
	};

	union FeaturePair
	{
		struct Edges
		{
			char inEdge1, outEdge1;
			char inEdge2, outEdge2;
		} e;
		int value;
	};

	struct Contact
	{
		Contact() : Pn( 0.0f ), Pt( 0.0f ), Pnb( 0.0f ) {}

		Vec2 position, normal;
		Vec2 r1, r2;
		float separation;
		// accumulated normal impulse, tangent impulse, normal impulse for position bias
		float Pn, Pt, Pnb;
		float massNormal, massTangent, bias;
		FeaturePair feature;
	};

	struct ArbiterKey
	{
		ArbiterKey( Body* b1, Body* b2 ) { body1 = ( b1 < b2 ) ? b1 : b2; body2 = ( b1 < b2 ) ? b2 : b1; }

		Body* body1;
		Body* body2;
	};

	struct Arbiter
	{
		enum {MAX_POINTS = 2};

		Arbiter( Body* b1, Body* b2 );

		void Update( Contact* contacts, int numContacts );

		void PreStep( float inv_dt );
		void ApplyImpulse();

		Contact contacts[MAX_POINTS];
		int numContacts;

		Body* body1;
		Body* body2;

		// Combined friction
		float friction;
	};

// This is used by std::set
	inline bool operator < ( const ArbiterKey& a1, const ArbiterKey& a2 )
	{
		return ( ( a1.body1 < a2.body1 ) || ( a1.body1 == a2.body1 && a1.body2 < a2.body2 ) );
	}

// If false is returned in NewCollision then no collision is created
	typedef bool ( *user_collision_callback_t )( void* UserData, Arbiter* Cnt );

/// Returns high-precision timestamps
	typedef double ( *user_timing_function_t )();

	struct World
	{
		void* CollisionUserData;

		// Called at the first strike
		user_collision_callback_t NewCollisionCallback;

		// Called for the persistent contact
		user_collision_callback_t CollisionCallback;

		// Called right after the contact disappearance
		user_collision_callback_t EndCollisionCallback;

		World( Vec2 _gravity, int _iterations ): CollisionCallback( NULL ), NewCollisionCallback( NULL ), EndCollisionCallback( NULL ),
			CollisionUserData( NULL ),
			TimingFunction( NULL ),
			gravity( _gravity ), iterations( _iterations ) {}

		inline void Add( Body* body ) { bodies.push_back( body ); }
		inline void Add( Joint* joint ) { joints.push_back( joint ); }
		inline void Clear() { bodies.clear(); joints.clear(); arbiters.clear(); }

		void BroadPhase();
		void Step( float dt );

		std::vector<Body*> bodies;
		std::vector<Joint*> joints;
		std::map<ArbiterKey, Arbiter> arbiters;
		Vec2 gravity;
		int iterations;
		static bool accumulateImpulses;
		static bool warmStarting;
		static bool positionCorrection;

		/// Used in timing
		user_timing_function_t TimingFunction;

		/// Timings for Step() procedure internals
		double BroadPhaseTime;
		double PreStepTime;
		double CollisionResolveTime;
		double VelocityStepTime;

		double InsertArbiterTime;
		double EraseArbiterTime;
		double IntersectTime;
	};

}

#endif // BoxLite
