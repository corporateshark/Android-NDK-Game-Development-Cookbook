#pragma once

#include <string>

#if defined( _WIN32 )
#  define OS_WINDOWS
#  define USE_OPENGL_3
#endif

typedef unsigned int     Luint;
typedef int              Lint;
typedef unsigned char    Lubyte;
typedef signed char      Lbyte;
typedef unsigned int     Lenum;
typedef unsigned short   Lushort;
typedef short            Lshort;
typedef int              Lsizei;
typedef void*            Lhandle;
