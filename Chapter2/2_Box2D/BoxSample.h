/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

Body* CreateBody( const Vec2& Size, float Mass, float Friction )
{
	Body* B = new Body();
	B->Set( Size, Mass );
	B->friction = Friction;

	return B;
}

Body* CreateBodyPos( const Vec2& Pos, float Ang, const Vec2& Size, float Mass, float Friction )
{
	Body* B = CreateBody( Size, Mass, Friction );
	B->position = Pos;
	B->rotation = Ang;
	return B;
}

Joint* CreateJoint( Body* B1, Body* B2, const Vec2& Anchor, float Softness, float Bias )
{
	Joint* J = new Joint();

	J->Set( B1, B2, Anchor );

	J->softness = Softness;
	J->biasFactor = Bias;

	return J;
}

Body* AddGround( World* W, const Vec2& Pos, float Angle, const Vec2& Size, float Friction )
{
	Body* Ground = CreateBody( Size, FLT_MAX, Friction );

	Ground->position = Pos;
	Ground->rotation = Angle;

	W->Add( Ground );

	return Ground;
}

void setup3( World* W )
{
	W->gravity = Vec2( 0, -10 );

	Body* G = AddGround( W, Vec2( 0.0f, -10.0f ), 0.0f, Vec2( 100.0f, 20.0f ), 0.2f );

	/// 8. Dominos
	AddGround( W, Vec2( -1.5f, 10.0f ), 0.0f, Vec2( 12.0f, 0.5f ), 0.2f );

	for ( int i = 0; i < 10; ++i )
	{ W->Add( CreateBodyPos( Vec2( -6.0f + 1.0f * ( float )i, 11.125f ), 0.0f, Vec2( 0.2f, 2.0f ), 10.0f, 0.1f ) ); }

	Body* B1 = AddGround( W, Vec2( 1.0f, 6.0f ), 0.3f, Vec2( 14.0f, 0.5f ), 0.2f );
	Body* B2 = AddGround( W, Vec2( -7.0f, 4.0f ), 0.0f, Vec2( 0.5f, 3.0f ), 0.2f );

	Body* B3 = CreateBodyPos( Vec2( -0.9f, 1.0f ), 0.0f, Vec2( 12.0f, 0.25f ), 20.0f, 0.2f );
	W->Add( B3 );

	W->Add( CreateJoint( G, B3, Vec2( -2.0f, 1.0f ), 0.0f, 0.2f ) );

	Body* B4 = CreateBodyPos( Vec2( -10.0f, 15.0f ), 0.0f, Vec2( 0.5f, 0.5f ), 10.0f, 0.2f );
	W->Add( B4 );

	W->Add( CreateJoint( B2, B4, Vec2( -7.0f, 15.0f ), 0.0f, 0.2f ) );

	Body* B5 = CreateBodyPos( Vec2( 6.0f, 2.5f ), 0.0f, Vec2( 2.0f, 2.0f ), 20.0f, 0.1f );
	W->Add( B5 );

	W->Add( CreateJoint( B1, B5, Vec2( 6.0f, 2.6f ), 0.0f, 0.2f ) );

	Body* B6 = CreateBodyPos( Vec2( 6.0f, 3.6f ), 0.0f, Vec2( 2.0f, 0.2f ), 10.0f, 0.2f );
	W->Add( B6 );

	W->Add( CreateJoint( B5, B6, Vec2( 7.0f, 3.5f ), 0.0f, 0.2f ) );
}
