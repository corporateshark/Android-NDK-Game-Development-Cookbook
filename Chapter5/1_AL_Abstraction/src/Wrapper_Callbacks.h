#ifndef _Wrapper_Callbacks_h_
#define _Wrapper_Callbacks_h_

#include <string>

const int ImageWidth = 512;
const int ImageHeight = 512;

extern unsigned char* g_FrameBuffer;

void OnStart( const std::string& RootPath );
void OnDestroy();
void OnDrawFrame();
void OnTimer( float Delta );
void OnKeyUp( int code );
void OnKeyDown( int code );
void OnMouseDown( int btn, int x, int y );
void OnMouseMove( int x, int y );
void OnMouseUp( int btn, int x, int y );

#endif
