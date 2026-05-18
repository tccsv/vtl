#ifndef _VTL_USER_HISTORY_DATA_H
#define _VTL_USER_HISTORY_DATA_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>
#include <VTL/user/VTL_user_data.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>



typedef struct _VTL_UserHistory
{
    VTL_User user;
    VTL_Time user_start_time;
    VTL_Time publication_start_time;
    VTL_Filename text_file_name;
    VTL_Filename media_file_name;
    VTL_content_platform_flags flags;
} VTL_UserHistory;

void VTL_user_history_text_pubication_Init(VTL_UserHistory* p_user_history, const VTL_User* p_user, 
                                    const VTL_Filename src_media_file_name, 
                                    const VTL_content_platform_flags flags);
void VTL_user_history_text_pubication_InitSheduled(VTL_UserHistory* p_user_history, const VTL_User* p_user, 
                                    const VTL_Filename src_media_file_name,
                                    const VTL_content_platform_flags flags,
                                    const VTL_Time sheduled_time);
void VTL_user_history_media_w_text_pubication_Init(VTL_UserHistory* p_user_history, const VTL_User* p_user, 
                                const VTL_Filename src_text_file_name,
                                const VTL_Filename src_media_file_name,
                                const VTL_content_platform_flags flags);
void VTL_user_history_Zeroize(VTL_UserHistory* p_user_history);

#ifdef __cplusplus
}
#endif


#endif