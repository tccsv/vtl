#ifndef VTL_VK_API_H
#define VTL_VK_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Тонкая прослойка над VK API (https://api.vk.com/method/...) и Bots Long Poll. */

#define VTL_VK_API_HOST   "https://api.vk.com/method"
#define VTL_VK_API_VER    "5.199"
#define VTL_VK_WAIT_SEC   25

/* Запас под percent-encoded текст ответа (до ~3x от исходного). */
#define VTL_VK_MSG_ENC_MAX 8192

/* Координаты long-poll сервера, полученные от groups.getLongPollServer. */
typedef struct VTL_vk_LongPoll {
    char server[256];
    char key[512];     /* ключ — JWT, бывает 200+ символов */
    char ts[64];
} VTL_vk_LongPoll;

/* Percent-encode строки в out (для query). Возвращает out. */
char* VTL_vk_api_Escape(const char* in, char* out, size_t cap);

/* groups.getLongPollServer -> заполняет lp. 1 — ок, 0 — ошибка. */
int VTL_vk_api_OpenLongPoll(const char* token, const char* group_id,
                            VTL_vk_LongPoll* lp);

/* Один a_check к long-poll серверу. Возвращает тело ответа (malloc, free()
 * на вызывающем) либо NULL. */
char* VTL_vk_api_Check(const VTL_vk_LongPoll* lp);

/* messages.send. 1 — ок, 0 — ошибка транспорта/VK. */
int VTL_vk_api_Reply(const char* token, long long peer_id, const char* text);

#ifdef __cplusplus
}
#endif

#endif
