#ifndef _VTL_VKBOT_DATA_H
#define _VTL_VKBOT_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

/* VK-бот «личные финансы» (расходы / доходы / категории / лимит) —
 * общие константы модуля. */

/* База методов VK API: <base>/<method>?...&access_token=<token>&v=<version> */
#define VTL_VKBOT_API_BASE      "https://api.vk.com/method"

/* Версия VK API. */
#define VTL_VKBOT_API_VERSION   "5.199"

#define VTL_VKBOT_DEFAULT_STORE "vkbot_data.json"

/* Тайм-аут Bots Long Poll (act=a_check), сек. Сервер держит соединение
 * до wait секунд, пока нет событий. */
#define VTL_VKBOT_POLL_WAIT_S   25

/* Буфер ответа (лимит сообщения VK — ~4096 символов). */
#define VTL_VKBOT_REPLY_MAX     4000

#ifdef __cplusplus
}
#endif

#endif /* _VTL_VKBOT_DATA_H */
