#ifndef _VTL_HISTORY_ADMINISTRATION_READ_H
#define _VTL_HISTORY_ADMINISTRATION_READ_H
#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/history_administration/VTL_history_administration_data.h>
#include <VTL/utils/db/VTL_db.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/VTL_app_result.h>

// Показать все публикации
VTL_AppResult VTL_history_administration_ShowAll(VTL_Database* db);

// Показать по пользователю
VTL_AppResult VTL_history_administration_ShowByUser(VTL_Database* db, const char* user_name);

// Показать до указанного времени
VTL_AppResult VTL_history_administration_ShowBeforeTime(VTL_Database* db, const VTL_Time* p_time);

// Показать после указанного времени
VTL_AppResult VTL_history_administration_ShowAfterTime(VTL_Database* db, const VTL_Time* p_time);

// Показать по платформе (Telegram/VK)
VTL_AppResult VTL_history_administration_ShowByPlatform(VTL_Database* db, VTL_Platform platform);

// Показать по статусу
VTL_AppResult VTL_history_administration_ShowByStatus(VTL_Database* db, VTL_PublicationStatus status);

// Найти одну запись по ID
VTL_AppResult VTL_history_administration_FindById(VTL_Database* db, int id, VTL_UserHistory* history);

// Экспорт всей истории в CSV файл
VTL_AppResult VTL_history_administration_DownloadAll(VTL_Database* db);

#ifdef __cplusplus
}
#endif
#endif