#ifndef __OGG_inl__
#define __OGG_inl__

#include "modplug/modplug.h"
#include "vorbis/vorbis/codec.h"
#include "vorbis/vorbis/vorbisfile.h"

#if defined(ANDROID)
#define OGG_ov_clear           ov_clear
#define OGG_ov_open_callbacks  ov_open_callbacks
#define OGG_ov_time_seek       ov_time_seek
#define OGG_ov_info            ov_info
#define OGG_ov_comment         ov_comment
#define OGG_ov_read            ov_read
#define ModPlug_Load_P          ModPlug_Load
#define ModPlug_Unload_P        ModPlug_Unload
#define ModPlug_Read_P          ModPlug_Read
#define ModPlug_Seek_P          ModPlug_Seek
#define ModPlug_GetSettings_P   ModPlug_GetSettings
#define ModPlug_SetSettings_P   ModPlug_SetSettings
#else

typedef ModPlugFile* ( *ModPlug_Load_func )( const void* data, int size );
typedef void ( *ModPlug_Unload_func )( ModPlugFile* file );
typedef int  ( *ModPlug_Read_func )( ModPlugFile* file, void* buffer, int size );
typedef void ( *ModPlug_Seek_func )( ModPlugFile* file, int millisecond );
typedef void ( *ModPlug_GetSettings_func )( ModPlug_Settings* settings );
typedef void ( *ModPlug_SetSettings_func )( const ModPlug_Settings* settings );

extern ModPlug_Load_func ModPlug_Load_P;
extern ModPlug_Unload_func ModPlug_Unload_P;
extern ModPlug_Read_func ModPlug_Read_P;
extern ModPlug_Seek_func ModPlug_Seek_P;
extern ModPlug_GetSettings_func ModPlug_GetSettings_P;
extern ModPlug_SetSettings_func ModPlug_SetSettings_P;

typedef int  ( __cdecl* ov_clear_func )( OggVorbis_File* vf );
typedef int  ( __cdecl* ov_open_callbacks_func )( void* datasource, OggVorbis_File* vf, char* initial, long ibytes, ov_callbacks callbacks );
typedef int  ( __cdecl* ov_time_seek_func )( OggVorbis_File* vf, double pos );
typedef long ( __cdecl* ov_read_func )( OggVorbis_File* vf, char* buffer, int length, int bigendianp, int word, int sgned, int* bitstream );

typedef vorbis_info* ( __cdecl* ov_info_func )( OggVorbis_File* vf, int link );
typedef vorbis_comment* ( __cdecl* ov_comment_func )( OggVorbis_File* vf, int link );

extern ov_clear_func          OGG_ov_clear;
extern ov_open_callbacks_func OGG_ov_open_callbacks;
extern ov_time_seek_func      OGG_ov_time_seek;
extern ov_info_func           OGG_ov_info;
extern ov_comment_func        OGG_ov_comment;
extern ov_read_func           OGG_ov_read;
#endif // OGG_DYNAMIC_LINK

bool LoadModPlug();
bool UnloadModPlug();
void LoadOGG();
void UnloadOGG();

#endif
