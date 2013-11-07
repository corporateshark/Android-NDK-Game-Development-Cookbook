#include <stdlib.h>

#include "LGL.h"

sLGLAPI* LGL3 = NULL;

void OnDrawFrame()
{
	LGL3->glClearColor( 1.0, 0.0, 0.0, 0.0 );
	LGL3->glClear( GL_COLOR_BUFFER_BIT );
}

void OnStart( const std::string& RootPath ) {}
void OnKeyUp( int code ) {}
void OnKeyDown( int code ) {}
void OnMouseDown( int btn, int x, int y ) {}
void OnMouseMove( int x, int y ) {}
void OnMouseUp( int btn, int x, int y ) {}
