#ifndef _VTL_CONTENT_PLATFORM_TG_NET_H
#define _VTL_CONTENT_PLATFORM_TG_NET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/VTL_publication_data.h>
#include <VTL/user/VTL_user_data.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/VTL_app_result.h>

typedef struct _VTL_net_api_data_TG
{
    int x;
} VTL_net_api_data_TG;


VTL_AppResult VTL_content_platform_tg_audio_w_marked_text_SendNow(const VTL_Filename audio_file_name,
                                                                    const VTL_Filename text_file_name);

VTL_AppResult VTL_content_platform_tg_marked_text_SendScheduled(const VTL_Filename file_name, 
    const VTL_Time sheduled_time);

VTL_AppResult VTL_content_platform_tg_marked_text_SendNow(const VTL_Filename file_name);


#ifdef __cplusplus
}
#endif


#endif