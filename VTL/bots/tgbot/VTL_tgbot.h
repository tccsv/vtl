#ifndef _VTL_tgbot_H
#define _VTL_tgbot_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/utils/VTL_utils_file.h>                /* VTL_Filename */
#include <VTL/VTL_content_platform_flags.h>          /* VTL_content_platform_flags */
#include <VTL/VTL_publication_markup_text_flags.h>   /* VTL_publication_marked_text_MarkupType */

/* Telegram-бот для публикации VTL. */
typedef VTL_AppResult (*VTL_tgbot_PublishTextFn)(
        const VTL_Filename file_name,
        const VTL_content_platform_flags flags,
        const VTL_publication_marked_text_MarkupType markup_type);

typedef VTL_AppResult (*VTL_tgbot_PublishAudioFn)(
        const VTL_Filename audio_file_name,
        const VTL_Filename text_file_name,
        const VTL_publication_marked_text_MarkupType markup_type,
        const VTL_content_platform_flags flags);

/* Точки входа в функционал проекта (любой указатель может быть NULL). */
typedef struct VTL_tgbot_Handlers
{
    VTL_tgbot_PublishTextFn  publish_text;
    VTL_tgbot_PublishAudioFn publish_audio;
} VTL_tgbot_Handlers;

/* Запускает цикл бота (long-polling) до Ctrl+C. */
VTL_AppResult VTL_tgbot_Run(const char* token, const VTL_tgbot_Handlers* handlers);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_tgbot_H */
