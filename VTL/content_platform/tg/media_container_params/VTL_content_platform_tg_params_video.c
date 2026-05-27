#include <VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_video.h>


void VTL_video_tg_SetParams(VTL_video_Params *p_new_params, const VTL_video_Params *p_old_params)
{
    memcpy(p_new_params, p_old_params, sizeof(*p_new_params));
    p_new_params->codec = VTL_video_codec_kH265;
}