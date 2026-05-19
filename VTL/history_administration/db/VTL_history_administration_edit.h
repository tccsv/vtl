#ifndef _VTL_HISTORY_ADMINISTRATION_READ_H
#define _VTL_HISTORY_ADMINISTRATION_READ_H
#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/history_administration/VTL_history_administration_data.h>
#include <VTL/utils/db/VTL_db.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/VTL_app_result.h>

// Обновить статус публикации
VTL_AppResult VTL_history_administration_UpdateStatus(
    VTL_Database* db,
    int publication_id,
    VTL_PublicationStatus new_status
);

// Обновить текстовый файл
VTL_AppResult VTL_history_administration_UpdateTextFile(
    VTL_Database* db,
    int publication_id,
    const char* new_text_file
);

// Обновить медиафайл
VTL_AppResult VTL_history_administration_UpdateMediaFile(
    VTL_Database* db,
    int publication_id,
    const char* new_media_file
);

// Обновить платформу
VTL_AppResult VTL_history_administration_UpdatePlatform(
    VTL_Database* db,
    int publication_id,
    VTL_Platform new_platform
);

// Перепланировать время публикации
VTL_AppResult VTL_history_administration_Reschedule(
    VTL_Database* db,
    int publication_id,
    VTL_Time new_time
);

#ifdef __cplusplus
}
#endif
#endif