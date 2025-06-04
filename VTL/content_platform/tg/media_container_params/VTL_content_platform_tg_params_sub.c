#include <VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_sub.h>


void VTL_sub_tg_SetParams(VTL_sub_Params *p_new_params, const VTL_sub_Params *p_old_params)
{
    memcpy(p_new_params, p_old_params, sizeof(*p_new_params));
    p_new_params->text_size = 25;
}