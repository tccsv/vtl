#include <VTL/user/history/db/VTL_user_history_Save.h>

typedef struct _VTL_EncryptedUserHistory
{
    VTL_EncryptedString user;
    VTL_EncryptedString publication_start_time;
    VTL_EncryptedString src_file_name;
} VTL_EncryptedUserHistory;

// typedef int VTL_EncryptedUserHistory;

static void VTL_user_history_Encrypt(const VTL_UserHistory* p_history, VTL_EncryptedUserHistory* p_encrypted_history)
{
    return;
}

//TO DO добавить стандартные операции с БД

static VTL_AppResult VTL_user_history_WriteToDB(const VTL_EncryptedUserHistory* p_history)
{    
    return VTL_res_kOk;
}


static VTL_AppResult VTL_user_history_Send(const VTL_UserHistory* p_history)
{    
    VTL_EncryptedUserHistory encrypted_history;
    VTL_user_history_Encrypt(p_history, &encrypted_history);
    return VTL_user_history_WriteToDB(&encrypted_history);
}

VTL_AppResult VTL_user_history_SaveTextPublication(const VTL_Filename file_name, const VTL_content_platform_flags flags)
{
    VTL_User user;
    VTL_user_Init(&user);
    VTL_UserHistory history;    
    VTL_user_history_text_pubication_Init(&history, &user, file_name, flags);
    return VTL_user_history_Send(&history);    
}

VTL_AppResult VTL_user_history_SaveMediaWTextPublication(const VTL_Filename src_text_file_name,
                                                            const VTL_Filename src_media_file_name, 
                                                            const VTL_content_platform_flags flags)
{
    VTL_User user;
    VTL_user_Init(&user);
    VTL_UserHistory history;    
    VTL_user_history_media_w_text_pubication_Init(&history, &user, src_text_file_name, src_media_file_name, flags);
    return VTL_user_history_Send(&history); 
}