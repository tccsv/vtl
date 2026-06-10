#ifndef _VTL_tgbot_api_H
#define _VTL_tgbot_api_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>

/* getMe — пишет @username бота в out_username. 1 — ок, 0 — ошибка. */
int VTL_tgbot_api_GetMe(const char* token, char* out_username, size_t cap);

/* deleteWebhook. 1 — ок, 0 — ошибка. */
int VTL_tgbot_api_DeleteWebhook(const char* token);

/* getUpdates. Возвращает тело ответа (free()), NULL при ошибке. */
char* VTL_tgbot_api_GetUpdates(const char* token, long offset, int timeout_s);

/* sendMessage. */
VTL_AppResult VTL_tgbot_api_SendMessage(const char* token,
                                      const char* chat_id,
                                      const char* text);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_tgbot_api_H */
