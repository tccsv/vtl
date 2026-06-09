#ifndef _VTL_BOT_SESSION_H
#define _VTL_BOT_SESSION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_content_platform_flags.h>
#include <VTL/VTL_publication_markup_text_flags.h>

#include <stddef.h>

/*
 * Состояние диалога с одним чатом: какие платформы, какой формат разметки и
 * какой файл выбраны для следующей публикации. Хранится в памяти (выбор живёт
 * пока бот запущен) — никакой персистентной БД боту не нужно.
 */

#define VTL_BOT_SESSION_MAX_CHATS 64
#define VTL_BOT_CHAT_ID_MAX       32
#define VTL_BOT_FILE_MAX          256

typedef struct VTL_bot_Session
{
    char                                    chat_id[VTL_BOT_CHAT_ID_MAX];
    VTL_content_platform_flags              platforms;
    VTL_publication_marked_text_MarkupType  markup;
    char                                    text_file[VTL_BOT_FILE_MAX];
    int                                     in_use;
} VTL_bot_Session;

typedef struct VTL_bot_SessionTable
{
    VTL_bot_Session items[VTL_BOT_SESSION_MAX_CHATS];
} VTL_bot_SessionTable;

/* Обнуляет таблицу сессий. */
void VTL_bot_session_TableInit(VTL_bot_SessionTable* table);

/* Находит сессию чата или создаёт новую с дефолтами (платформы W+TG, формат
 * TelegramMD, файл VTL_BOT_DEFAULT_TEXT). NULL — таблица переполнена. */
VTL_bot_Session* VTL_bot_session_GetOrCreate(VTL_bot_SessionTable* table,
                                             const char* chat_id);

/* Разбирает имя платформы (tg/w/reddit/vimeo/yt/vk) в бит флага.
 * 1 — распознано (бит в *out_bit), 0 — неизвестно. */
int VTL_bot_session_ParsePlatform(const char* token,
                                  VTL_content_platform_flags* out_bit);

/* Разбирает имя формата (md/telegram/html/bb/asciidoc/mediawiki) в enum.
 * 1 — распознано (*out_markup), 0 — неизвестно. */
int VTL_bot_session_ParseFormat(const char* token,
                                VTL_publication_marked_text_MarkupType* out_markup);

/* Человекочитаемое имя формата разметки. */
const char* VTL_bot_session_FormatName(VTL_publication_marked_text_MarkupType markup);

/* Перечисляет выбранные платформы в out ("TG, W"); "—" если ни одной. */
void VTL_bot_session_DescribePlatforms(VTL_content_platform_flags flags,
                                       char* out, size_t cap);

/* Список поддерживаемых имён платформ и форматов (для справки). */
const char* VTL_bot_session_PlatformsHelp(void);
const char* VTL_bot_session_FormatsHelp(void);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_SESSION_H */
