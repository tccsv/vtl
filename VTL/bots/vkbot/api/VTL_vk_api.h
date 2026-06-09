#ifndef VTL_VK_API_H
#define VTL_VK_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>

/* Прослойка над VK API и Bots Long Poll. */

#define VTL_VK_API_HOST   "https://api.vk.com/method"
#define VTL_VK_API_VER    "5.199"
#define VTL_VK_WAIT_SEC   25

#define VTL_VK_MSG_ENC_MAX 8192

/* Координаты long-poll сервера. */
typedef struct VTL_vk_LongPoll {
    char server[256];
    char key[512];
    char ts[64];
} VTL_vk_LongPoll;

char* VTL_vk_api_Escape(const char* in, char* out, size_t cap);

/* Собирает URL метода. query может быть NULL. */
char* VTL_vk_api_BuildMethodUrl(const char* method, const char* query,
                                const char* token, char* out, size_t cap);

/* GET по url, отдаёт JSON либо NULL. */
struct json_value_t* VTL_vk_api_GetJson(const char* url);

/* groups.getLongPollServer -> lp. */
int VTL_vk_api_OpenLongPoll(const char* token, const char* group_id,
                            VTL_vk_LongPoll* lp);

/* Один a_check. Тело — malloc, free() на вызывающем. */
char* VTL_vk_api_Check(const VTL_vk_LongPoll* lp);

/* messages.send. */
int VTL_vk_api_Reply(const char* token, long long peer_id, const char* text);

int VTL_vk_api_SendMessage(const char* token, long long peer_id, const char* text);

/* groups.getById -> имя сообщества. */
int VTL_vk_api_GetCommunityName(const char* token, const char* group_id,
                                char* out_name, size_t cap);

/* groups.getById, members_count. <0 — ошибка. */
long VTL_vk_api_GetMembersCount(const char* token, const char* group_id);

/* Проверка токена. */
int VTL_vk_api_CheckToken(const char* token, const char* group_id);

/* messages.setActivity type=typing. */
int VTL_vk_api_SetTyping(const char* token, long long peer_id);

/* messages.markAsRead. */
int VTL_vk_api_MarkAsRead(const char* token, long long peer_id);

#ifdef __cplusplus
}
#endif

#endif
