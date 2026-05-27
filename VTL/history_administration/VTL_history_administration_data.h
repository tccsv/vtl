#ifndef _VTL_USER_HISTORY_ADMINISTRATION_DATA_H
#define _VTL_USER_HISTORY_ADMINISTRATION_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>
#include <stdbool.h>
#include <VTL/user/VTL_user_data.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/utils/VTL_utils_time.h>
#include <VTL/utils/VTL_utils_file.h>
#include <VTL/VTL_app_result.h>

// Тип публикации
typedef enum _VTL_PublicationType {
    VTL_PUBLICATION_TEXT_ONLY,       // Только текст
    VTL_PUBLICATION_MEDIA_ONLY,      // Только медиа (видео/фото)
    VTL_PUBLICATION_MEDIA_WITH_TEXT   // Медиа + текст
} VTL_PublicationType;

// Платформа публикации
typedef enum _VTL_Platform {
    VTL_PLATFORM_TELEGRAM = 1,
    VTL_PLATFORM_VK       = 2
} VTL_Platform;

// Статус публикации
typedef enum _VTL_PublicationStatus {
    VTL_PUBLICATION_DRAFT     = 0,   // Черновик
    VTL_PUBLICATION_SCHEDULED = 1,   // Запланирована
    VTL_PUBLICATION_PUBLISHED = 2,   // Опубликована
    VTL_PUBLICATION_FAILED    = 3    // Ошибка публикации
} VTL_PublicationStatus;

typedef struct _VTL_HistoryRecord {
    int id;                                  // ID записи в БД
    VTL_User user;                           // Пользователь
    VTL_Time user_start_time;                // Время регистрации пользователя
    VTL_Time publication_start_time;         // Время публикации
    VTL_Filename text_file_name;             // Файл с текстом
    bool has_text_file;                      // Есть ли текстовый файл
    VTL_Filename media_file_name;            // Медиафайл
    bool has_media_file;                     // Есть ли медиафайл
    VTL_content_platform_flags flags;        // Флаги платформ
    VTL_PublicationType publication_type;     // Тип публикации
    VTL_Platform platform;                   // Куда опубликовано
    VTL_PublicationStatus status;            // Статус
} VTL_HistoryRecord;

// === Инициализация ===

// Публикация только текста
void VTL_user_history_text_publication_Init(
    VTL_HistoryRecord* history,
    const VTL_User* user,
    const VTL_Filename text_file_name,
    VTL_Platform platform,
    VTL_content_platform_flags flags
);

// Публикация только медиа
void VTL_user_history_media_publication_Init(
    VTL_HistoryRecord* history,
    const VTL_User* user,
    const VTL_Filename media_file_name,
    VTL_Platform platform,
    VTL_content_platform_flags flags
);

// Публикация медиа + текст
void VTL_user_history_media_w_text_publication_Init(
    VTL_HistoryRecord* history,
    const VTL_User* user,
    const VTL_Filename text_file_name,
    const VTL_Filename media_file_name,
    VTL_Platform platform,
    VTL_content_platform_flags flags
);

// Запланированная публикация
void VTL_user_history_scheduled_Init(
    VTL_HistoryRecord* history,
    const VTL_User* user,
    const VTL_Filename text_file_name,
    const VTL_Filename media_file_name,
    VTL_Platform platform,
    VTL_content_platform_flags flags,
    VTL_Time scheduled_time
);

// Обновить статус
void VTL_user_history_SetStatus(VTL_HistoryRecord* history, VTL_PublicationStatus status);

// Очистка
void VTL_history_administration_Zeroize(VTL_HistoryRecord* history);

// Печать
void VTL_user_history_Print(const VTL_HistoryRecord* history);



#ifdef __cplusplus
}
#endif


#endif