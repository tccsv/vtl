#ifndef _VTL_user_history_Send_H
#define _VTL_user_history_Send_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/user/history/VTL_user_history_data.h>
#include <VTL/utils/VTL_encryptor.h>
#include <VTL/VTL_app_result.h>

VTL_AppResult VTL_user_history_SaveTextPublication(const VTL_Filename file_name, const VTL_content_platform_flags flags);
VTL_AppResult VTL_user_history_SaveMediaWTextPublication(const VTL_Filename src_text_file_name,
                                                            const VTL_Filename src_media_file_name, 
                                                            const VTL_content_platform_flags flags);


#ifdef __cplusplus
}
#endif


#endif