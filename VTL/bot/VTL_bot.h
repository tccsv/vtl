#ifndef _VTL_BOT_H
#define _VTL_BOT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/utils/VTL_utils_file.h>                /* VTL_Filename */
#include <VTL/VTL_content_platform_flags.h>          /* VTL_content_platform_flags */
#include <VTL/VTL_publication_markup_text_flags.h>   /* VTL_publication_marked_text_MarkupType */

/*
 * Telegram-бот — удалённый пульт публикации VTL. Сам бот не публикует ничего:
 * он принимает от пользователя выбор платформ (флаги) и формата разметки и
 * дёргает функции основного проекта. Конкретные реализации публикации
 * прокидываются снаружи (см. main_firyulin.c), чтобы библиотека бота не тянула
 * весь VTL и оставалась тестируемой в изоляции.
 *
 * Сигнатуры handler-ов совпадают с VTL_PubicateMarkedText /
 * VTL_PubicateAudioWithMarkedText из <VTL/publication/VTL_publication.h>.
 */
typedef VTL_AppResult (*VTL_bot_PublishTextFn)(
        const VTL_Filename file_name,
        const VTL_content_platform_flags flags,
        const VTL_publication_marked_text_MarkupType markup_type);

typedef VTL_AppResult (*VTL_bot_PublishAudioFn)(
        const VTL_Filename audio_file_name,
        const VTL_Filename text_file_name,
        const VTL_publication_marked_text_MarkupType markup_type,
        const VTL_content_platform_flags flags);

/* Точки входа в функционал проекта. Любой указатель может быть NULL —
 * тогда соответствующая команда отвечает, что операция недоступна. */
typedef struct VTL_bot_Handlers
{
    VTL_bot_PublishTextFn  publish_text;
    VTL_bot_PublishAudioFn publish_audio;
} VTL_bot_Handlers;

/* Запускает цикл бота (long-polling) и блокирует поток до Ctrl+C.
 * handlers — вызовы публикации проекта (может быть NULL: бот тогда работает
 * как демонстрация выбора параметров без реальной отправки). */
VTL_AppResult VTL_bot_Run(const char* token, const VTL_bot_Handlers* handlers);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_H */
