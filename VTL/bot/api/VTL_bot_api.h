#ifndef _VTL_BOT_API_H
#define _VTL_BOT_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>

/* Обёртка над Telegram Bot API поверх HTTP-клиента проекта (VTL/utils/curl). */

/* getMe — пишет @username бота в out_username (cap байт). 1 — ок, 0 — ошибка. */
int VTL_bot_api_GetMe(const char* token, char* out_username, size_t cap);

/* deleteWebhook. Сбрасывает webhook бота, иначе getUpdates даёт HTTP 409
 * (Conflict). Зовётся один раз на старте long-polling. 1 — ок, 0 — ошибка. */
int VTL_bot_api_DeleteWebhook(const char* token);

/* getUpdates. Возвращает тело ответа (JSON); вызывающий делает free().
 * NULL — ошибка запроса или не-200. */
char* VTL_bot_api_GetUpdates(const char* token, long offset, int timeout_s);

/* sendMessage. Текст уходит JSON-полем, экранировать вручную не нужно. */
VTL_AppResult VTL_bot_api_SendMessage(const char* token,
                                      const char* chat_id,
                                      const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_API_H */
