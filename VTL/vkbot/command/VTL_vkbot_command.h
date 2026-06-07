#ifndef _VTL_VKBOT_COMMAND_H
#define _VTL_VKBOT_COMMAND_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/vkbot/store/VTL_vkbot_store.h>

/* Разбирает сообщение, выполняет команду над хранилищем и отвечает в peer_id. */
void VTL_vkbot_command_Handle(VTL_vkbot_Store* store, const char* token,
                              const char* peer_id, const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_VKBOT_COMMAND_H */
