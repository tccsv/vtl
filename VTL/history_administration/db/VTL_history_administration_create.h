#ifndef _VTL_USER_HISTORY_ADMINISTRATION_CREATE_H
#define _VTL_USER_HISTORY_ADMINISTRATION_CREATE_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/history_administration/VTL_history_administration_data.h>
#include <VTL/utils/db/VTL_db.h>
#include <VTL/VTL_app_result.h>

// Создать таблицу
VTL_AppResult VTL_history_administration_CreateTable(VTL_Database* db);

// Вставить одну публикацию
VTL_AppResult VTL_history_administration_Insert(VTL_Database* db, VTL_UserHistory* history);

// Вставить запланированную публикацию
VTL_AppResult VTL_history_administration_InsertScheduled(
    VTL_Database* db,
    VTL_UserHistory* history,
    VTL_Time scheduled_time
);

// Скопировать все записи в бэкап-таблицу
VTL_AppResult VTL_history_administration_CopyAll(VTL_Database* db);


#ifdef __cplusplus
}
#endif


#endif