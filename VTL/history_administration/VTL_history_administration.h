#ifndef _VTL_USER_HISTORY_ADMINISTRATION_H
#define _VTL_USER_HISTORY_ADMINISTRATION_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/user/history/VTL_user_history.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/VTL_app_result.h>


typedef VTL_StandartString VTL_DecryptedUserName;

VTL_AppResult VTL_history_administration_ShowAll(void);
VTL_AppResult VTL_history_administration_DownloadAll(void);
VTL_AppResult VTL_history_administration_CopyAll(void);
VTL_AppResult VTL_history_administration_DeleteAll(void);
VTL_AppResult VTL_history_administration_DeleteBeforeTime(const VTL_Time* p_time);
VTL_AppResult VTL_history_administration_DeleteAfterTime(const VTL_Time* p_time);
VTL_AppResult VTL_history_administration_ShowBeforeTime(const VTL_Time* p_time);
VTL_AppResult VTL_history_administration_ShowAfterTime(const VTL_Time* p_time);



VTL_AppResult VTL_history_administration_ShowByUser(const VTL_DecryptedUserName user_name);
VTL_AppResult VTL_history_administration_DeleteByUser(const VTL_DecryptedUserName user_name);


#ifdef __cplusplus
}
#endif


#endif