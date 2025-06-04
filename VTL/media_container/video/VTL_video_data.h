#ifndef _VTL_VIDEO_DATA_H
#define _VTL_VIDEO_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/utils/VTL_buffer_data.h>
#include <VTL/VTL_app_result.h>

#include <stdint.h>
#include <stdbool.h>

typedef uint_least32_t VTL_video_Bitrate;
typedef uint_least16_t VTL_video_FPS;
typedef uint_least16_t VTL_video_ResolutionScalar;
typedef struct _VTL_video_Resolution
{
    VTL_video_ResolutionScalar width;
    VTL_video_ResolutionScalar height;
} VTL_video_Resolution;

typedef enum _VTL_video_Codec
{
    VTL_video_codec_kDefault = 0,
    VTL_video_codec_kAV1,
    VTL_video_codec_kH264,
    VTL_video_codec_kH265,
    VTL_video_codec_kMPEG1,
    VTL_video_codec_kMPEG2,
    VTL_video_codec_kMPEG4,
    VTL_video_codec_kRawVideo,
    VTL_video_codec_kVP5,
    VTL_video_codec_kVP6,
    VTL_video_codec_kVP7,
    VTL_video_codec_kVP8,
    VTL_video_codec_kVP9,

    //TODO Добавить кодеков
} VTL_video_Codec;

typedef enum _VTL_video_ContainerType
{
    VTL_video_container_kDefault = 0,
    VTL_video_container_kMov,
    VTL_video_container_kMP4,
    VTL_video_container_kMKV,
    VTL_video_container_kWebM
    //// TO DO дописать для проверяемых форматов файлов, если появятся 
} VTL_video_ContainerType;

typedef struct _VTL_video_Params
{
    VTL_video_Bitrate bitrate;
    VTL_video_FPS fps;
    VTL_video_Resolution resolution;
    VTL_video_Codec codec;
    VTL_video_ContainerType container_type;
} VTL_video_Params;

typedef struct _VTL_video_MetaData
{
    size_t data_size;
    VTL_video_Params params;
} VTL_video_MetaData;

typedef VTL_BufferData VTL_video_Data;

typedef struct _VTL_Video
{
    VTL_video_MetaData meta_data;
    VTL_video_Data* p_current_part;
} VTL_Video;


// VTL_AppResult VTL_video_data_Decode(VTL_video_Data **pp_video_part, const VTL_video_Params* p_params);
VTL_AppResult VTL_video_part_Encode(VTL_video_Data **pp_video_part, const VTL_video_Params* p_old_params, 
                                        const VTL_video_Params* p_new_params);

VTL_AppResult VTL_video_params_BitrateSet(VTL_video_Params* p_params, const VTL_video_Bitrate bitrate);
VTL_AppResult VTL_video_params_FpsSet(VTL_video_Params* p_params, const VTL_video_FPS bitrate);
VTL_AppResult VTL_video_params_CodecSet(VTL_video_Params* p_params, const VTL_video_Codec codec);
VTL_AppResult VTL_video_params_ResolutionSet(VTL_video_Params* p_params, const VTL_video_Resolution resolution);
VTL_AppResult VTL_video_params_ContainerTypeSet(VTL_video_Params* p_params, const VTL_video_ContainerType container_type);


#ifdef __cplusplus
}
#endif


#endif