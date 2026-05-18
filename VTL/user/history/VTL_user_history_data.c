#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE
#include <time.h>
#include <string.h>
#include <VTL/user/history/VTL_user_history_data.h>

time_t time(time_t *t);

time_t VTL_user_history_GetCurrentTime(void)
{
    time_t current_time;
    return time(&current_time);
}

void VTL_user_history_text_pubication_Init(VTL_UserHistory* p_user_history, const VTL_User* p_user, 
                                const VTL_Filename src_text_file_name,
                                const VTL_content_platform_flags flags)
{
    p_user_history->user = *p_user;
    p_user_history->flags = flags;
    p_user_history->user_start_time = VTL_user_history_GetCurrentTime();
    p_user_history->publication_start_time = p_user_history->user_start_time;
    memcpy(p_user_history->text_file_name, src_text_file_name, VTL_FILENAME_MAX_LENGTH);

}

void VTL_user_history_media_w_text_pubication_Init(VTL_UserHistory* p_user_history, const VTL_User* p_user, 
                                const VTL_Filename src_text_file_name,
                                const VTL_Filename src_media_file_name,
                                const VTL_content_platform_flags flags)
{
    p_user_history->user = *p_user;
    p_user_history->flags = flags;
    p_user_history->user_start_time = VTL_user_history_GetCurrentTime();
    p_user_history->publication_start_time = p_user_history->user_start_time;
    memcpy(p_user_history->text_file_name, src_text_file_name, VTL_FILENAME_MAX_LENGTH);
    memcpy(p_user_history->media_file_name, src_media_file_name, VTL_FILENAME_MAX_LENGTH);

}

void VTL_user_history_text_pubication_InitSheduled(VTL_UserHistory* p_user_history, const VTL_User* p_user, 
                                    const VTL_Filename src_media_file_name,
                                    const VTL_content_platform_flags flags,
                                    const VTL_Time sheduled_time)
{
    p_user_history->user = *p_user;
    p_user_history->flags = flags;
    p_user_history->user_start_time = VTL_user_history_GetCurrentTime();
    p_user_history->publication_start_time = sheduled_time;
    memcpy(p_user_history->text_file_name, src_media_file_name, VTL_FILENAME_MAX_LENGTH);
}

void VTL_user_history_Zeroize(VTL_UserHistory* p_user_history)
{
    return;
}