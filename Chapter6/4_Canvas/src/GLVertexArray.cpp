#include "GLVertexArray.h"
#include "VertexAttribs.h"
#include "LGL.h"
#include "LGLAPI.h"

extern sLGLAPI* LGL3;

clGLVertexArray::clGLVertexArray()
	: FVBOID( 0 ),
	  FVAOID( 0 ),
	  FAttribVBOOffset( L_VS_TOTAL_ATTRIBS ),
	  FEnumeratedStreams( L_VS_TOTAL_ATTRIBS ),
	  FAttribs( NULL )
{
#if defined( _WIN32 )
	LGL3->glGenVertexArrays( 1, &FVAOID );
#endif
}

clGLVertexArray::~clGLVertexArray()
{
	LGL3->glDeleteBuffers( 1, &FVBOID );
#if defined( _WIN32 )
	LGL3->glDeleteVertexArrays( 1, &FVAOID );
#endif
}

void clGLVertexArray::Bind() const
{
	LGL3->glBindBuffer( GL_ARRAY_BUFFER, FVBOID );
	LGL3->glVertexAttribPointer( L_VS_VERTEX, L_VS_VEC_COMPONENTS[ 0 ], GL_FLOAT, GL_FALSE, 0, FAttribVBOOffset[ 0 ] );
	LGL3->glEnableVertexAttribArray( L_VS_VERTEX );

	for ( int i = 1; i < L_VS_TOTAL_ATTRIBS; i++ )
	{
		LGL3->glVertexAttribPointer( i, L_VS_VEC_COMPONENTS[ i ], GL_FLOAT, GL_FALSE, 0, FAttribVBOOffset[ i ] );

		FAttribVBOOffset[ i ] ? LGL3->glEnableVertexAttribArray( i ) : LGL3->glDisableVertexAttribArray( i );
	}
}

void clGLVertexArray::Draw( bool Wireframe ) const
{
#if defined( _WIN32 )
	LGL3->glBindVertexArray( FVAOID );
#else
	Bind();
#endif

	LGL3->glDrawArrays( Wireframe ? GL_LINE_LOOP : GL_TRIANGLES, 0, static_cast<GLsizei>( FAttribs->GetActiveVertexCount() ) );
}

void clGLVertexArray::SetVertexAttribs( const clPtr<clVertexAttribs>& Attribs )
{
	FAttribs = Attribs;

	FEnumeratedStreams = FAttribs->EnumerateVertexStreams();

	// remove old vertices to allow reuse of vertex array
	LGL3->glDeleteBuffers( 1, &FVBOID );

	size_t VertexCount = FAttribs->FVertices.size();
	size_t DataSize = 0;

	for ( int i = 0; i != L_VS_TOTAL_ATTRIBS; i++ )
	{
		FAttribVBOOffset[ i ] = ( void* )DataSize;

		DataSize += FEnumeratedStreams[i] ? sizeof( float ) * L_VS_VEC_COMPONENTS[ i ] * VertexCount : 0;
	}

	LGL3->glGenBuffers( 1, &FVBOID );
	LGL3->glBindBuffer( GL_ARRAY_BUFFER, FVBOID );
	LGL3->glBufferData( GL_ARRAY_BUFFER, DataSize, NULL, GL_STREAM_DRAW );

	for ( int i = 0; i != L_VS_TOTAL_ATTRIBS; i++ )
	{
		if ( FEnumeratedStreams[i] )
		{
			LGL3->glBufferSubData( GL_ARRAY_BUFFER, ( GLintptrARB )FAttribVBOOffset[ i ], FAttribs->GetActiveVertexCount() * sizeof( float ) * L_VS_VEC_COMPONENTS[ i ], FEnumeratedStreams[ i ] );
		}
	}

#if defined( _WIN32 )
	LGL3->glBindVertexArray( FVAOID );
	Bind();
	LGL3->glBindVertexArray( 0 );
#endif
}
