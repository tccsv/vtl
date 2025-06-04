#include <VTL/user/history/VTL_user_history_data.h>

static VTL_Time VTL_user_history_GetCurrentTime(void)
{
    return time(NULL);
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