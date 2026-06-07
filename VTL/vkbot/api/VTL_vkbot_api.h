#ifndef _VTL_VKBOT_API_H
#define _VTL_VKBOT_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>

/* Обёртка над VK API (Bots Long Poll) поверх HTTP-клиента проекта
 * (VTL/utils/curl). */

/* groups.getLongPollServer — получает параметры long-poll сервера.
 * Пишет адрес сервера, ключ и ts в выданные буферы. 1 — ок, 0 — ошибка. */
int VTL_vkbot_api_GetLongPollServer(const char* token, const char* group_id,
                                    char* out_server, size_t server_cap,
                                    char* out_key,    size_t key_cap,
                                    char* out_ts,     size_t ts_cap);

/* act=a_check. Возвращает тело ответа (JSON); вызывающий делает free().
 * NULL — ошибка запроса или не-200. */
char* VTL_vkbot_api_GetUpdates(const char* server, const char* key,
                               const char* ts, int wait_s);

/* messages.send. Текст уходит url-encoded в теле POST; экранировать
 * вручную не нужно. */
VTL_AppResult VTL_vkbot_api_SendMessage(const char* token, const char* peer_id,
                                        const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_VKBOT_API_H */
