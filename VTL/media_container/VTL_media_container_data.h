#ifndef _VTL_MEDIA_CONTAINER_DATA_H
#define _VTL_MEDIA_CONTAINER_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/audio/VTL_audio.h>
#include <VTL/media_container/sub/VTL_sub.h>
#include <VTL/media_container/video/VTL_video.h>

#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>



typedef struct _VTL_MediaContainer
{
    VTL_Audio audio;
    VTL_Sub sub;
    VTL_Video video;
} VTL_MediaContainer;

#ifdef __cplusplus
}
#endif


#endif