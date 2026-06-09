#ifndef VTL_VK_COMMAND_H
#define VTL_VK_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/bots/vkbot/session/VTL_vk_session.h>

/* Разбор и маршрутизация команд диалога. */

/* Разобрать текст от peer и выполнить команду. */
void VTL_vk_hub_Handle(VTL_vk_Hub* hub, long long peer_id, const char* text);

void VTL_vk_command_Help(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Targets(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Markup(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Source(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Show(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Token(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Post(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_PostAudio(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_TargetsList(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_MarkupsList(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Me(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);
void VTL_vk_command_Ping(VTL_vk_Hub* hub, VTL_vk_Dialog* d, long long peer_id, const char* args);

#ifdef __cplusplus
}
#endif

#endif
