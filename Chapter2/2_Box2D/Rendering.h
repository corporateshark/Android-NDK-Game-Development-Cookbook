#include "Wrapper_Callbacks.h"

extern unsigned char* g_FrameBuffer;

extern const int Width;
extern const int Height;

extern float XScale, YScale;
extern float XOfs, YOfs;

inline int XToScreen( float x ) { return Width / 2 + x * XScale + XOfs; }
inline int YToScreen( float y ) { return Height / 2 - y * YScale + YOfs; }

inline float ScreenToX( int x ) { return ( ( float )( x - Width / 2 )  - XOfs ) / XScale; }
inline float ScreenToY( int y ) { return -( ( float )( y - Height / 2 ) - YOfs ) / YScale; }

inline void set_pixel( unsigned char* fb, int w, int h, int x, int y, int color )
{
	if ( x < 0 || y < 0 || x > w - 1 || y > h - 1 ) { return; }

//	fb[((h - 1 - y) * w + x) * 3 + 0] = ( color       ) & 0xFF;
//	fb[((h - 1 - y) * w + x) * 3 + 1] = ( color >> 8  ) & 0xFF;
//	fb[((h - 1 - y) * w + x) * 3 + 2] = ( color >> 16 ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 0] = ( color       ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 1] = ( color >> 8  ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 2] = ( color >> 16 ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 3] = 0; // ( color >> 16 ) & 0xFF;
}

inline void swap_int( int& v1, int& v2 ) { int t = v1; v1 = v2; v2 = t; }

void line_bresenham( unsigned char* fb, int w, int h, int p1x, int p1y, int p2x, int p2y, int color );

inline void Line( int x1, int y1, int x2, int y2, int color )
{
	line_bresenham( g_FrameBuffer, Width, Height, x1, y1, x2, y2, color );
}

void Clear( int color );

inline void LineW( float x1, float y1, float x2, float y2, int color )
{
	Line( XToScreen( x1 ), YToScreen( y1 ), XToScreen( x2 ), YToScreen( y2 ), color );
}
