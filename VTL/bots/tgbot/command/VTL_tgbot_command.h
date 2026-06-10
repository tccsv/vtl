#ifndef _VTL_tgbot_command_H
#define _VTL_tgbot_command_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/bots/tgbot/VTL_tgbot.h>
#include <VTL/bots/tgbot/session/VTL_tgbot_session.h>

/* Куда уходит ответ боту (Telegram или консоль). */
typedef void (*VTL_tgbot_ReplyFn)(void* sink_ud, const char* chat_id,
                                const char* text);

/* Контекст обработки команд. */
typedef struct VTL_tgbot_Context
{
    VTL_tgbot_sessionTable*    sessions;
    const VTL_tgbot_Handlers*  handlers;
    VTL_tgbot_ReplyFn          reply;
    void*                    reply_ud;
} VTL_tgbot_Context;

/* Разбирает сообщение и выполняет команду. */
void VTL_tgbot_command_Handle(VTL_tgbot_Context* ctx, const char* chat_id,
                            const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_tgbot_command_H */
