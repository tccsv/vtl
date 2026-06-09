#ifndef _VTL_BOT_COMMAND_H
#define _VTL_BOT_COMMAND_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/bot/VTL_bot.h>
#include <VTL/bot/session/VTL_bot_session.h>

/* Куда уходит ответ боту. В проде — sendMessage в Telegram (см. VTL_bot.c),
 * в локальном демо — печать в консоль. Развязка ввода-вывода от логики команд
 * делает диспетчер тестируемым и позволяет гонять бота без сети. */
typedef void (*VTL_bot_ReplyFn)(void* sink_ud, const char* chat_id,
                                const char* text);

/* Контекст обработки команд: сессии чатов, точки входа в проект, sink ответов. */
typedef struct VTL_bot_Context
{
    VTL_bot_SessionTable*    sessions;
    const VTL_bot_Handlers*  handlers;
    VTL_bot_ReplyFn          reply;
    void*                    reply_ud;
} VTL_bot_Context;

/* Разбирает сообщение чата, выполняет команду и отвечает в chat_id. */
void VTL_bot_command_Handle(VTL_bot_Context* ctx, const char* chat_id,
                            const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_COMMAND_H */
