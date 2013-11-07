/*VERTEX_PROGRAM*/

uniform mat4 in_ModelViewProjectionMatrix;

in vec4 in_Vertex;
in vec4 in_TexCoord;

out vec2 TexCoord;

void main()
{
//	gl_Position = in_ModelViewProjectionMatrix * in_Vertex;

	gl_Position = vec4( in_Vertex.x, in_Vertex.y, in_Vertex.z, in_Vertex.w ) * vec4( 2.0, -2.0, 1.0, 1.0 ) + vec4( -1.0, 1.0, 0.0, 0.0 );	

	TexCoord = in_TexCoord.xy;
}

/*FRAGMENT_PROGRAM*/

in vec2 TexCoord;

uniform sampler2D Texture0;
uniform sampler2D Texture1; // top
uniform sampler2D Texture2; // bottom

uniform float b_Flags[7];

uniform float b_MoveLeft;
uniform float b_Down;
uniform float b_MoveRight;
uniform float b_TurnLeft;
uniform float b_TurnRight;
uniform float b_Reset;
uniform float b_Paused;

out vec4 out_FragColor;

bool ContainsPoint( vec2 Point, vec4 Rect )
{
	return Point.x >= Rect.x && Point.y >= Rect.y && Point.x <= Rect.z && Point.y <= Rect.w;
}

void main()
{
	const vec4 MoveLeft  = vec4( 0.0,  0.863, 0.32, 1.0 );
	const vec4 Down      = vec4( 0.32, 0.863, 0.67, 1.0 );
	const vec4 MoveRight = vec4( 0.67, 0.863, 1.0,  1.0 );
	const vec4 TurnLeft  = vec4( 0.0,  0.7,  0.4,  0.863);
	const vec4 TurnRight = vec4( 0.6,  0.7,  1.0,  0.863);
	const vec4 Reset     = vec4( 0.0,  0.0,  0.2,  0.1 );
	const vec4 Paused    = vec4( 0.8,  0.0,  1.0,  0.1 );

	vec4 Color       = texture( Texture0, TexCoord );
	vec4 ColorHighT  = texture( Texture1, TexCoord * vec2( 4.0, 8.0 ) );
	vec4 ColorHighB  = texture( Texture2, TexCoord * vec2( 1.0, 2.0 ) );

	if ( b_MoveLeft  > 0.5 && ContainsPoint( TexCoord.xy, MoveLeft   ) ) Color = ColorHighB;
	if ( b_Down      > 0.5 && ContainsPoint( TexCoord.xy, Down       ) ) Color = ColorHighB;
	if ( b_MoveRight > 0.5 && ContainsPoint( TexCoord.xy, MoveRight  ) ) Color = ColorHighB;
	if ( b_TurnLeft  > 0.5 && ContainsPoint( TexCoord.xy, TurnLeft   ) ) Color = ColorHighB;
	if ( b_TurnRight > 0.5 && ContainsPoint( TexCoord.xy, TurnRight  ) ) Color = ColorHighB;

	if ( b_Reset     > 0.5 && ContainsPoint( TexCoord.xy, Reset      ) ) Color = ColorHighT;
//	if ( b_Paused    > 0.5 && ContainsPoint( TexCoord.xy, Paused     ) ) Color = ColorHighT;

   out_FragColor = Color;
}
