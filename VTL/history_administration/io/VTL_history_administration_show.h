#ifndef _VTL_USER_HISTORY_ADMINISTRATION_SHOW_H
#define _VTL_USER_HISTORY_ADMINISTRATION_SHOW_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/history_administration/VTL_history_administration_data.h>
#include <VTL/utils/db/VTL_db.h>
#include <VTL/VTL_app_result.h>

// Вывести одну публикацию подробно
void VTL_show_PublicationDetailed(const VTL_UserHistory* history);

// Вывести одну публикацию кратко (одна строка)
void VTL_show_PublicationShort(const VTL_UserHistory* history);

// Вывести заголовок таблицы
void VTL_show_TableHeader(void);

// Вывести разделитель
void VTL_show_Separator(void);

// Интерактивное меню администратора
VTL_AppResult VTL_show_AdminMenu(VTL_Database* db);

// Интерактивный просмотр истории пользователя
VTL_AppResult VTL_show_UserHistoryInteractive(VTL_Database* db);

// Показать статистику по всем публикациям
VTL_AppResult VTL_show_Statistics(VTL_Database* db);

#ifdef __cplusplus
}
#endif


#endif