#ifndef _VTL_MEDIA_CONTAINER_AUDIO_INFRA_CONFIG_H
#define _VTL_MEDIA_CONTAINER_AUDIO_INFRA_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/media_container/audio/VTL_media_container_audio_data.h>
#include <VTL/media_container/audio/infra/VTL_media_container_audio_infra_file.h>
#include <VTL/utils/VTL_utils_file.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


typedef struct _VTL_audio_Congif
{
    VTL_Filename file_name;
    VTL_audio_Params params;
} VTL_audio_Congif;

typedef struct _VTL_audio_Configs
{
    VTL_audio_Congif* data;
    size_t length;
} VTL_audio_Configs;


#ifdef __cplusplus
}
#endif

#endif



