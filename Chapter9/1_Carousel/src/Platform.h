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

#if defined( __GNUC__ )

#include <typeinfo>
#include <stdint.h>

typedef int64_t       Lint64;
typedef uint64_t      Luint64;
typedef int32_t       Lint32;
typedef uint32_t      Luint32;

#else

typedef __int64          Lint64;
typedef unsigned __int64 Luint64;
typedef __int32          Lint32;
typedef unsigned __int32 Luint32;

#endif // __GNUC__