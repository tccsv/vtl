#ifndef _VTL_BOT_COMMAND_H
#define _VTL_BOT_COMMAND_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/bot/store/VTL_bot_store.h>

/* Разбирает сообщение, выполняет команду над хранилищем и отвечает в chat_id. */
void VTL_bot_command_Handle(VTL_bot_Store* store, const char* token,
                            const char* chat_id, const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_COMMAND_H */
