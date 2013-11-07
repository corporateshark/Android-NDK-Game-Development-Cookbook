/**
   This is an extraction from original FreeType headers

   All of this code is written by FreeType authors and presented here without any modifications

   'generic' and 'internal' names are changed to 'generic_' and 'internal_' to allow .NET C++/CLI usage
*/

/// \cond

#define ft_ptrdiff_t  ptrdiff_t

typedef unsigned int FT_UInt32;
typedef int FT_Int32;

// FTSYSTEM

typedef struct FT_MemoryRec_*  FT_Memory;

typedef void* ( *FT_Alloc_Func )( FT_Memory memory, long size );
typedef void  ( *FT_Free_Func )( FT_Memory memory, void* block );
typedef void* ( *FT_Realloc_Func )( FT_Memory memory, long cur_size, long new_size, void* block );

struct  FT_MemoryRec_
{
	void*            user;
	FT_Alloc_Func    alloc;
	FT_Free_Func     free;
	FT_Realloc_Func  realloc;
};

typedef struct FT_StreamRec_*  FT_Stream;

typedef union  FT_StreamDesc_
{
	long   value;
	void*  pointer;
} FT_StreamDesc;

typedef unsigned long ( *FT_Stream_IoFunc )( FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count );

typedef void ( *FT_Stream_CloseFunc )( FT_Stream  stream );

typedef struct  FT_StreamRec_
{
	unsigned char*       base;
	unsigned long        size;
	unsigned long        pos;
	FT_StreamDesc        descriptor;
	FT_StreamDesc        pathname;
	FT_Stream_IoFunc     read;
	FT_Stream_CloseFunc  close;
	FT_Memory            memory;
	unsigned char*       cursor;
	unsigned char*       limit;
} FT_StreamRec;

// FTIMAGE

typedef signed long  FT_Pos;

typedef struct  FT_Vector_
{
	FT_Pos  x;
	FT_Pos  y;
} FT_Vector;

typedef struct  FT_BBox_
{
	FT_Pos  xMin, yMin;
	FT_Pos  xMax, yMax;
} FT_BBox;

typedef enum  FT_Pixel_Mode_
{
   FT_PIXEL_MODE_NONE = 0,
   FT_PIXEL_MODE_MONO,
   FT_PIXEL_MODE_GRAY,
   FT_PIXEL_MODE_GRAY2,
   FT_PIXEL_MODE_GRAY4,
   FT_PIXEL_MODE_LCD,
   FT_PIXEL_MODE_LCD_V,
   FT_PIXEL_MODE_MAX
} FT_Pixel_Mode;

typedef struct  FT_Bitmap_
{
	int             rows;
	int             width;
	int             pitch;
	unsigned char*  buffer;
	short           num_grays;
	char            pixel_mode;
	char            palette_mode;
	void*           palette;
} FT_Bitmap;

typedef struct  FT_Outline_
{
	short       n_contours;
	short       n_points;
	FT_Vector*  points;
	char*       tags;
	short*      contours;
	int         flags;
} FT_Outline;

#define FT_OUTLINE_CONTOURS_MAX  SHRT_MAX
#define FT_OUTLINE_POINTS_MAX    SHRT_MAX

#define FT_OUTLINE_NONE             0x0
#define FT_OUTLINE_OWNER            0x1
#define FT_OUTLINE_EVEN_ODD_FILL    0x2
#define FT_OUTLINE_REVERSE_FILL     0x4
#define FT_OUTLINE_IGNORE_DROPOUTS  0x8
#define FT_OUTLINE_SMART_DROPOUTS   0x10
#define FT_OUTLINE_INCLUDE_STUBS    0x20

#define FT_OUTLINE_HIGH_PRECISION   0x100
#define FT_OUTLINE_SINGLE_PASS      0x200

#ifndef FT_IMAGE_TAG
#define FT_IMAGE_TAG( value, _x1, _x2, _x3, _x4 )  \
          value = ( ( (unsigned long)_x1 << 24 ) | \
                    ( (unsigned long)_x2 << 16 ) | \
                    ( (unsigned long)_x3 << 8  ) | \
                      (unsigned long)_x4         )
#endif /* FT_IMAGE_TAG */

typedef enum  FT_Glyph_Format_
{
   FT_IMAGE_TAG( FT_GLYPH_FORMAT_NONE, 0, 0, 0, 0 ),
   FT_IMAGE_TAG( FT_GLYPH_FORMAT_COMPOSITE, 'c', 'o', 'm', 'p' ),
   FT_IMAGE_TAG( FT_GLYPH_FORMAT_BITMAP,    'b', 'i', 't', 's' ),
   FT_IMAGE_TAG( FT_GLYPH_FORMAT_OUTLINE,   'o', 'u', 't', 'l' ),
   FT_IMAGE_TAG( FT_GLYPH_FORMAT_PLOTTER,   'p', 'l', 'o', 't' )
} FT_Glyph_Format;

// FTTYPES

typedef unsigned char  FT_Bool;
typedef signed short  FT_FWord;
typedef unsigned short  FT_UFWord;
typedef signed char  FT_Char;
typedef unsigned char  FT_Byte;
typedef const FT_Byte*  FT_Bytes;
typedef FT_UInt32  FT_Tag;
typedef char  FT_String;
typedef signed short  FT_Short;
typedef unsigned short  FT_UShort;
typedef signed int  FT_Int;
typedef unsigned int  FT_UInt;
typedef signed long  FT_Long;
typedef unsigned long  FT_ULong;
typedef signed short  FT_F2Dot14;
typedef signed long  FT_F26Dot6;
typedef signed long  FT_Fixed;
typedef int  FT_Error;
typedef void*  FT_Pointer;
typedef size_t  FT_Offset;
typedef ft_ptrdiff_t  FT_PtrDist;

typedef struct  FT_UnitVector_
{
	FT_F2Dot14  x;
	FT_F2Dot14  y;
} FT_UnitVector;

typedef struct  FT_Matrix_
{
	FT_Fixed  xx, xy;
	FT_Fixed  yx, yy;
} FT_Matrix;

typedef struct  FT_Data_
{
	const FT_Byte*  pointer;
	FT_Int          length;
} FT_Data;

typedef void  ( *FT_Generic_Finalizer )( void*  object );

typedef struct  FT_Generic_
{
	void*                 data;
	FT_Generic_Finalizer  finalizer;
} FT_Generic;

#define FT_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          (FT_Tag)                        \
          ( ( (FT_ULong)_x1 << 24 ) |     \
            ( (FT_ULong)_x2 << 16 ) |     \
            ( (FT_ULong)_x3 <<  8 ) |     \
              (FT_ULong)_x4         )

typedef struct FT_ListNodeRec_*  FT_ListNode;
typedef struct FT_ListRec_*  FT_List;

typedef struct  FT_ListNodeRec_
{
	FT_ListNode  prev;
	FT_ListNode  next;
	void*        data;
} FT_ListNodeRec;

typedef struct  FT_ListRec_
{
	FT_ListNode  head;
	FT_ListNode  tail;
} FT_ListRec;

#define FT_IS_EMPTY( list )  ( (list).head == 0 )
#define FT_ERROR_BASE( x )    ( (x) & 0xFF )
#define FT_ERROR_MODULE( x )  ( (x) & 0xFF00U )
#define FT_BOOL( x )  ( (FT_Bool)( x ) )

// FREETYPE.h

#ifndef FT_ENC_TAG
#define FT_ENC_TAG( value, a, b, c, d )         \
          value = ( ( (FT_UInt32)(a) << 24 ) |  \
                    ( (FT_UInt32)(b) << 16 ) |  \
                    ( (FT_UInt32)(c) <<  8 ) |  \
                      (FT_UInt32)(d)         )
#endif

typedef enum  FT_Encoding_
{
   FT_ENC_TAG( FT_ENCODING_NONE, 0, 0, 0, 0 ),
   FT_ENC_TAG( FT_ENCODING_MS_SYMBOL, 's', 'y', 'm', 'b' ),
   FT_ENC_TAG( FT_ENCODING_UNICODE,   'u', 'n', 'i', 'c' ),
   FT_ENC_TAG( FT_ENCODING_SJIS,    's', 'j', 'i', 's' ),
   FT_ENC_TAG( FT_ENCODING_GB2312,  'g', 'b', ' ', ' ' ),
   FT_ENC_TAG( FT_ENCODING_BIG5,    'b', 'i', 'g', '5' ),
   FT_ENC_TAG( FT_ENCODING_WANSUNG, 'w', 'a', 'n', 's' ),
   FT_ENC_TAG( FT_ENCODING_JOHAB,   'j', 'o', 'h', 'a' ),
   /* for backwards compatibility */
   FT_ENCODING_MS_SJIS    = FT_ENCODING_SJIS,
   FT_ENCODING_MS_GB2312  = FT_ENCODING_GB2312,
   FT_ENCODING_MS_BIG5    = FT_ENCODING_BIG5,
   FT_ENCODING_MS_WANSUNG = FT_ENCODING_WANSUNG,
   FT_ENCODING_MS_JOHAB   = FT_ENCODING_JOHAB,
   FT_ENC_TAG( FT_ENCODING_ADOBE_STANDARD, 'A', 'D', 'O', 'B' ),
   FT_ENC_TAG( FT_ENCODING_ADOBE_EXPERT,   'A', 'D', 'B', 'E' ),
   FT_ENC_TAG( FT_ENCODING_ADOBE_CUSTOM,   'A', 'D', 'B', 'C' ),
   FT_ENC_TAG( FT_ENCODING_ADOBE_LATIN_1,  'l', 'a', 't', '1' ),
   FT_ENC_TAG( FT_ENCODING_OLD_LATIN_2, 'l', 'a', 't', '2' ),
   FT_ENC_TAG( FT_ENCODING_APPLE_ROMAN, 'a', 'r', 'm', 'n' )
} FT_Encoding;

typedef struct  FT_Glyph_Metrics_
{
	FT_Pos  width;
	FT_Pos  height;
	FT_Pos  horiBearingX;
	FT_Pos  horiBearingY;
	FT_Pos  horiAdvance;
	FT_Pos  vertBearingX;
	FT_Pos  vertBearingY;
	FT_Pos  vertAdvance;
} FT_Glyph_Metrics;

typedef struct  FT_Bitmap_Size_
{
	FT_Short  height;
	FT_Short  width;
	FT_Pos    size;
	FT_Pos    x_ppem;
	FT_Pos    y_ppem;
} FT_Bitmap_Size;

typedef struct FT_LibraryRec_*  FT_Library;
typedef struct FT_ModuleRec_*  FT_Module;
typedef struct FT_DriverRec_*  FT_Driver;
typedef struct FT_RendererRec_*  FT_Renderer;
typedef struct FT_FaceRec_*  FT_Face;
typedef struct FT_SizeRec_*  FT_Size;
typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;
typedef struct FT_CharMapRec_*  FT_CharMap;

typedef struct  FT_CharMapRec_
{
	FT_Face      face;
	FT_Encoding  encoding;
	FT_UShort    platform_id;
	FT_UShort    encoding_id;
} FT_CharMapRec;

typedef struct FT_Face_InternalRec_*  FT_Face_Internal;

typedef struct  FT_FaceRec_
{
	FT_Long           num_faces;
	FT_Long           face_index;
	FT_Long           face_flags;
	FT_Long           style_flags;
	FT_Long           num_glyphs;
	FT_String*        family_name;
	FT_String*        style_name;
	FT_Int            num_fixed_sizes;
	FT_Bitmap_Size*   available_sizes;
	FT_Int            num_charmaps;
	FT_CharMap*       charmaps;
	FT_Generic        generic_;
	FT_BBox           bbox;
	FT_UShort         units_per_EM;
	FT_Short          ascender;
	FT_Short          descender;
	FT_Short          height;
	FT_Short          max_advance_width;
	FT_Short          max_advance_height;
	FT_Short          underline_position;
	FT_Short          underline_thickness;
	FT_GlyphSlot      glyph;
	FT_Size           size;
	FT_CharMap        charmap;
	FT_Driver         driver;
	FT_Memory         memory;
	FT_Stream         stream;
	FT_ListRec        sizes_list;
	FT_Generic        autohint;
	void*             extensions;
	FT_Face_Internal  internal_;
} FT_FaceRec;

#define FT_FACE_FLAG_SCALABLE          ( 1L <<  0 )
#define FT_FACE_FLAG_FIXED_SIZES       ( 1L <<  1 )
#define FT_FACE_FLAG_FIXED_WIDTH       ( 1L <<  2 )
#define FT_FACE_FLAG_SFNT              ( 1L <<  3 )
#define FT_FACE_FLAG_HORIZONTAL        ( 1L <<  4 )
#define FT_FACE_FLAG_VERTICAL          ( 1L <<  5 )
#define FT_FACE_FLAG_KERNING           ( 1L <<  6 )
#define FT_FACE_FLAG_FAST_GLYPHS       ( 1L <<  7 )
#define FT_FACE_FLAG_MULTIPLE_MASTERS  ( 1L <<  8 )
#define FT_FACE_FLAG_GLYPH_NAMES       ( 1L <<  9 )
#define FT_FACE_FLAG_EXTERNAL_STREAM   ( 1L << 10 )
#define FT_FACE_FLAG_HINTER            ( 1L << 11 )
#define FT_FACE_FLAG_CID_KEYED         ( 1L << 12 )
#define FT_FACE_FLAG_TRICKY            ( 1L << 13 )

#define FT_HAS_HORIZONTAL( face )  ( face->face_flags & FT_FACE_FLAG_HORIZONTAL )
#define FT_HAS_VERTICAL( face )    ( face->face_flags & FT_FACE_FLAG_VERTICAL )
#define FT_HAS_KERNING( face )     ( face->face_flags & FT_FACE_FLAG_KERNING )
#define FT_IS_SCALABLE( face )     ( face->face_flags & FT_FACE_FLAG_SCALABLE )
#define FT_IS_SFNT( face )         ( face->face_flags & FT_FACE_FLAG_SFNT )
#define FT_IS_FIXED_WIDTH( face )  ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
#define FT_HAS_FIXED_SIZES( face ) ( face->face_flags & FT_FACE_FLAG_FIXED_SIZES )
#define FT_HAS_FAST_GLYPHS( face )  0
#define FT_HAS_GLYPH_NAMES( face ) ( face->face_flags & FT_FACE_FLAG_GLYPH_NAMES )
#define FT_HAS_MULTIPLE_MASTERS( face ) ( face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS )
#define FT_IS_CID_KEYED( face ) ( face->face_flags & FT_FACE_FLAG_CID_KEYED )
#define FT_IS_TRICKY( face ) ( face->face_flags & FT_FACE_FLAG_TRICKY )

#define FT_STYLE_FLAG_ITALIC  ( 1 << 0 )
#define FT_STYLE_FLAG_BOLD    ( 1 << 1 )

typedef struct FT_Size_InternalRec_*  FT_Size_Internal;

typedef struct  FT_Size_Metrics_
{
	FT_UShort  x_ppem;
	FT_UShort  y_ppem;
	FT_Fixed   x_scale;
	FT_Fixed   y_scale;
	FT_Pos     ascender;
	FT_Pos     descender;
	FT_Pos     height;
	FT_Pos     max_advance;
} FT_Size_Metrics;

typedef struct  FT_SizeRec_
{
	FT_Face           face;
	FT_Generic        generic_;
	FT_Size_Metrics   metrics;
	FT_Size_Internal  internal_;
} FT_SizeRec;

typedef struct FT_SubGlyphRec_*  FT_SubGlyph;
typedef struct FT_Slot_InternalRec_*  FT_Slot_Internal;

typedef struct  FT_GlyphSlotRec_
{
	FT_Library        library;
	FT_Face           face;
	FT_GlyphSlot      next;
	FT_UInt           reserved;
	FT_Generic        generic_;
	FT_Glyph_Metrics  metrics;
	FT_Fixed          linearHoriAdvance;
	FT_Fixed          linearVertAdvance;
	FT_Vector         advance;
	FT_Glyph_Format   format;
	FT_Bitmap         bitmap;
	FT_Int            bitmap_left;
	FT_Int            bitmap_top;
	FT_Outline        outline;
	FT_UInt           num_subglyphs;
	FT_SubGlyph       subglyphs;
	void*             control_data;
	long              control_len;
	FT_Pos            lsb_delta;
	FT_Pos            rsb_delta;
	void*             other;
	FT_Slot_Internal  internal_;
} FT_GlyphSlotRec;

#define FT_OPEN_MEMORY    0x1
#define FT_OPEN_STREAM    0x2
#define FT_OPEN_PATHNAME  0x4
#define FT_OPEN_DRIVER    0x8
#define FT_OPEN_PARAMS    0x10

typedef struct  FT_Parameter_
{
	FT_ULong    tag;
	FT_Pointer  data;
} FT_Parameter;

typedef struct  FT_Open_Args_
{
	FT_UInt         flags;
	const FT_Byte*  memory_base;
	FT_Long         memory_size;
	FT_String*      pathname;
	FT_Stream       stream;
	FT_Module       driver;
	FT_Int          num_params;
	FT_Parameter*   params;
} FT_Open_Args;

typedef enum  FT_Size_Request_Type_
{
   FT_SIZE_REQUEST_TYPE_NOMINAL,
   FT_SIZE_REQUEST_TYPE_REAL_DIM,
   FT_SIZE_REQUEST_TYPE_BBOX,
   FT_SIZE_REQUEST_TYPE_CELL,
   FT_SIZE_REQUEST_TYPE_SCALES,
   FT_SIZE_REQUEST_TYPE_MAX
} FT_Size_Request_Type;

typedef struct  FT_Size_RequestRec_
{
	FT_Size_Request_Type  type;
	FT_Long               width;
	FT_Long               height;
	FT_UInt               horiResolution;
	FT_UInt               vertResolution;
} FT_Size_RequestRec;

typedef struct FT_Size_RequestRec_*  FT_Size_Request;

#define FT_LOAD_DEFAULT                      0x0
#define FT_LOAD_NO_SCALE                     0x1
#define FT_LOAD_NO_HINTING                   0x2
#define FT_LOAD_RENDER                       0x4
#define FT_LOAD_NO_BITMAP                    0x8
#define FT_LOAD_VERTICAL_LAYOUT              0x10
#define FT_LOAD_FORCE_AUTOHINT               0x20
#define FT_LOAD_CROP_BITMAP                  0x40
#define FT_LOAD_PEDANTIC                     0x80
#define FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH  0x200
#define FT_LOAD_NO_RECURSE                   0x400
#define FT_LOAD_IGNORE_TRANSFORM             0x800
#define FT_LOAD_MONOCHROME                   0x1000
#define FT_LOAD_LINEAR_DESIGN                0x2000
#define FT_LOAD_NO_AUTOHINT                  0x8000U

typedef enum  FT_Render_Mode_
{
   FT_RENDER_MODE_NORMAL = 0,
   FT_RENDER_MODE_LIGHT,
   FT_RENDER_MODE_MONO,
   FT_RENDER_MODE_LCD,
   FT_RENDER_MODE_LCD_V,
   FT_RENDER_MODE_MAX
} FT_Render_Mode;

typedef enum  FT_Kerning_Mode_
{
   FT_KERNING_DEFAULT  = 0,
   FT_KERNING_UNFITTED,
   FT_KERNING_UNSCALED
} FT_Kerning_Mode;

#define FT_SUBGLYPH_FLAG_ARGS_ARE_WORDS          1
#define FT_SUBGLYPH_FLAG_ARGS_ARE_XY_VALUES      2
#define FT_SUBGLYPH_FLAG_ROUND_XY_TO_GRID        4
#define FT_SUBGLYPH_FLAG_SCALE                   8
#define FT_SUBGLYPH_FLAG_XY_SCALE             0x40
#define FT_SUBGLYPH_FLAG_2X2                  0x80
#define FT_SUBGLYPH_FLAG_USE_MY_METRICS      0x200

#define FT_FSTYPE_INSTALLABLE_EMBEDDING         0x0000
#define FT_FSTYPE_RESTRICTED_LICENSE_EMBEDDING  0x0002
#define FT_FSTYPE_PREVIEW_AND_PRINT_EMBEDDING   0x0004
#define FT_FSTYPE_EDITABLE_EMBEDDING            0x0008
#define FT_FSTYPE_NO_SUBSETTING                 0x0100
#define FT_FSTYPE_BITMAP_EMBEDDING_ONLY         0x0200

#define FREETYPE_MAJOR  2
#define FREETYPE_MINOR  4
#define FREETYPE_PATCH  4

// FTGLYPH

typedef struct FT_Glyph_Class_  FT_Glyph_Class;
typedef struct FT_GlyphRec_*  FT_Glyph;

typedef struct  FT_GlyphRec_
{
	FT_Library             library;
	const FT_Glyph_Class*  clazz;
	FT_Glyph_Format        format;
	FT_Vector              advance;
} FT_GlyphRec;

typedef struct FT_BitmapGlyphRec_*  FT_BitmapGlyph;

typedef struct  FT_BitmapGlyphRec_
{
	FT_GlyphRec  root;
	FT_Int       left;
	FT_Int       top;
	FT_Bitmap    bitmap;
} FT_BitmapGlyphRec;

typedef struct FT_OutlineGlyphRec_*  FT_OutlineGlyph;

typedef struct  FT_OutlineGlyphRec_
{
	FT_GlyphRec  root;
	FT_Outline   outline;
} FT_OutlineGlyphRec;

typedef enum  FT_Glyph_BBox_Mode_
{
   FT_GLYPH_BBOX_UNSCALED  = 0,
   FT_GLYPH_BBOX_SUBPIXELS = 0,
   FT_GLYPH_BBOX_GRIDFIT   = 1,
   FT_GLYPH_BBOX_TRUNCATE  = 2,
   FT_GLYPH_BBOX_PIXELS    = 3
} FT_Glyph_BBox_Mode;

// FTCACHE

typedef FT_Pointer  FTC_FaceID;

typedef FT_Error ( *FTC_Face_Requester )( FTC_FaceID  face_id, FT_Library  library, FT_Pointer  request_data, FT_Face*    aface );

typedef struct FTC_ManagerRec_*  FTC_Manager;
typedef struct FTC_NodeRec_*  FTC_Node;

typedef struct  FTC_ScalerRec_
{
	FTC_FaceID  face_id;
	FT_UInt     width;
	FT_UInt     height;
	FT_Int      pixel;
	FT_UInt     x_res;
	FT_UInt     y_res;
} FTC_ScalerRec;

typedef struct FTC_ScalerRec_*  FTC_Scaler;
typedef struct FTC_CMapCacheRec_*  FTC_CMapCache;

typedef struct  FTC_ImageTypeRec_
{
	FTC_FaceID  face_id;
	FT_Int      width;
	FT_Int      height;
	FT_Int32    flags;
} FTC_ImageTypeRec;

typedef struct FTC_ImageTypeRec_*  FTC_ImageType;
typedef struct FTC_ImageCacheRec_*  FTC_ImageCache;
typedef struct FTC_SBitRec_*  FTC_SBit;

typedef struct  FTC_SBitRec_
{
	FT_Byte   width;
	FT_Byte   height;
	FT_Char   left;
	FT_Char   top;
	FT_Byte   format;
	FT_Byte   max_grays;
	FT_Short  pitch;
	FT_Char   xadvance;
	FT_Char   yadvance;
	FT_Byte*  buffer;
} FTC_SBitRec;

typedef struct FTC_SBitCacheRec_*  FTC_SBitCache;

/// \endcond
