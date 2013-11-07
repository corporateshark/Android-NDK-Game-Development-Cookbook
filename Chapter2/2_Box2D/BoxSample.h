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
	Body* Ground = CreateBody( Size /*vec2(100.0f, 20.0f)*/, FLT_MAX, Friction /*0.2f*/ );

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
