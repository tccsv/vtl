#ifndef _VTL_HISTORY_ADMINISTRATION_DELETE_H
#define _VTL_HISTORY_ADMINISTRATION_DELETE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/utils/db/VTL_db.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/VTL_app_result.h>

// Удалить все публикации
VTL_AppResult VTL_history_administration_DeleteAll(VTL_Database* db);

// Удалить по пользователю
VTL_AppResult VTL_history_administration_DeleteByUser(VTL_Database* db, const char* user_name);

// Удалить по ID
VTL_AppResult VTL_history_administration_DeleteById(VTL_Database* db, int publication_id);

// Удалить до указанного времени
VTL_AppResult VTL_history_administration_DeleteBeforeTime(VTL_Database* db, const VTL_Time* p_time);

// Удалить после указанного времени
VTL_AppResult VTL_history_administration_DeleteAfterTime(VTL_Database* db, const VTL_Time* p_time);

#ifdef __cplusplus
}
#endif
#endif