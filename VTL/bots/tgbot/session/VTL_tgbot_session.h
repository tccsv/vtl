#ifndef _VTL_tgbot_session_H
#define _VTL_tgbot_session_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_content_platform_flags.h>
#include <VTL/VTL_publication_markup_text_flags.h>

#include <stddef.h>

/* Состояние диалога с чатом: платформы, формат и файл. */

#define VTL_tgbot_session_MAX_CHATS 64
#define VTL_tgbot_CHAT_ID_MAX       32
#define VTL_tgbot_FILE_MAX          256
#define VTL_tgbot_TOKEN_MAX         128

typedef struct VTL_tgbot_session
{
    char                                    chat_id[VTL_tgbot_CHAT_ID_MAX];
    VTL_content_platform_flags              platforms;
    VTL_publication_marked_text_MarkupType  markup;
    char                                    text_file[VTL_tgbot_FILE_MAX];
    char                                    token[VTL_tgbot_TOKEN_MAX];
    int                                     in_use;
} VTL_tgbot_session;

typedef struct VTL_tgbot_sessionTable
{
    VTL_tgbot_session items[VTL_tgbot_session_MAX_CHATS];
} VTL_tgbot_sessionTable;

/* Обнуляет таблицу сессий. */
void VTL_tgbot_session_TableInit(VTL_tgbot_sessionTable* table);

/* Находит сессию чата или создаёт новую с дефолтами. NULL — таблица переполнена. */
VTL_tgbot_session* VTL_tgbot_session_GetOrCreate(VTL_tgbot_sessionTable* table,
                                             const char* chat_id);

/* Разбирает имя платформы в бит флага. 1 — распознано, 0 — нет. */
int VTL_tgbot_session_ParsePlatform(const char* token,
                                  VTL_content_platform_flags* out_bit);

/* Разбирает имя формата в enum. 1 — распознано, 0 — нет. */
int VTL_tgbot_session_ParseFormat(const char* token,
                                VTL_publication_marked_text_MarkupType* out_markup);

/* Имя формата разметки. */
const char* VTL_tgbot_session_FormatName(VTL_publication_marked_text_MarkupType markup);

/* Перечисляет выбранные платформы в out. */
void VTL_tgbot_session_DescribePlatforms(VTL_content_platform_flags flags,
                                       char* out, size_t cap);

/* Списки имён платформ и форматов. */
const char* VTL_tgbot_session_PlatformsHelp(void);
const char* VTL_tgbot_session_FormatsHelp(void);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_tgbot_session_H */
