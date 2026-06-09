#ifndef VTL_VK_BOT_H
#define VTL_VK_BOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/utils/VTL_utils_file.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/VTL_publication_markup_text_flags.h>

/* Бот сообщества ВКонтакте. Публикаторы передаются извне. */
typedef VTL_AppResult (*VTL_vk_PostTextFn)(const VTL_Filename,
                                           const VTL_content_platform_flags,
                                           const VTL_publication_marked_text_MarkupType);

typedef VTL_AppResult (*VTL_vk_PostAudioFn)(const VTL_Filename,
                                            const VTL_Filename,
                                            const VTL_publication_marked_text_MarkupType,
                                            const VTL_content_platform_flags);

typedef struct VTL_vk_Publishers {
    VTL_vk_PostTextFn  text;   /* NULL -> /post недоступна */
    VTL_vk_PostAudioFn audio;
} VTL_vk_Publishers;

/* Запуск long-poll и обработка сообщений до SIGINT. pub может быть NULL. */
VTL_AppResult VTL_vk_bot_Serve(const char* token, const char* group_id,
                               const VTL_vk_Publishers* pub);

#ifdef __cplusplus
}
#endif

#endif
