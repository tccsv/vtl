#include <VTL/content_platform/tg/VTL_content_platform_tg_net.h>

static void VTL_content_platform_tg_net_ApiDataInit(VTL_net_api_data_TG* p_api_data)
{
    // p_api_data-> = ...
    return;
}

VTL_AppResult VTL_content_platform_tg_text_SendScheduled(const VTL_Filename file_name, 
    const VTL_Time sheduled_time)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);

    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_text_SendNow(const VTL_Filename file_name)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);
    
    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_marked_text_SendScheduled(const VTL_Filename file_name, 
    const VTL_Time sheduled_time)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);

    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_marked_text_SendNow(const VTL_Filename file_name)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);
    
    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_video_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);

    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_video_SendNow(const VTL_Filename file_name)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);
    
    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_video_w_text_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);

    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_video_w_text_SendNow(const VTL_Filename file_name)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);
    
    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_video_w_marked_text_SendScheduled(const VTL_Filename file_name, 
    const VTL_Time sheduled_time)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);

    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_video_w_marked_text_SendNow(const VTL_Filename file_name)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);
    
    return VTL_res_kOk;
}

VTL_AppResult VTL_content_platform_tg_audio_w_marked_text_SendNow(const VTL_Filename audio_file_name,
                                                                    const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data;
    VTL_content_platform_tg_net_ApiDataInit(&api_data);
    
    return VTL_res_kOk;
}
