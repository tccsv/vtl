#ifndef _VTL_USER_DB_H
#define _VTL_USER_DB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/user/VTL_user_data.h>
#include <VTL/utils/db/VTL_db.h>

// Создание таблицы
bool VTL_user_db_CreateTable(VTL_Database* db);

// CRUD
bool VTL_user_db_Insert(VTL_Database* db, VTL_User* user);
bool VTL_user_db_FindByNickname(VTL_Database* db, const char* nickname, VTL_User* user);
bool VTL_user_db_FindAll(VTL_Database* db);
bool VTL_user_db_DeleteByNickname(VTL_Database* db, const char* nickname);

#ifdef __cplusplus
}
#endif
#endif