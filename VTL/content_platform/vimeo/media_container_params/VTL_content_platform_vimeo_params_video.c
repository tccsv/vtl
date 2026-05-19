#include <VTL/content_platform/vimeo/media_container_params/VTL_content_platform_vimeo_params_video.h>

/*
 * Vimeo рекомендует H.264 для максимальной совместимости.
 * Контейнер MP4.
 */
void VTL_video_vimeo_SetParams(VTL_video_Params *p_new_params,
                               const VTL_video_Params *p_old_params) {
    memcpy(p_new_params, p_old_params, sizeof(*p_new_params));
    p_new_params->codec = VTL_video_codec_kH264;
    p_new_params->container_type = VTL_video_container_kMP4;
}
