#ifndef _VTL_CONTENT_PLATFORM_VIMEO_PARAMS_VIDEO_H
#define _VTL_CONTENT_PLATFORM_VIMEO_PARAMS_VIDEO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/media_container/video/VTL_video_data.h>
#include <string.h>

/*
 * Устанавливает параметры видео для публикации на Vimeo.
 * Vimeo рекомендует H.264, MP4, до 4K.
 */
void VTL_video_vimeo_SetParams(VTL_video_Params *p_new_params,
                               const VTL_video_Params *p_old_params);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_CONTENT_PLATFORM_VIMEO_PARAMS_VIDEO_H */
