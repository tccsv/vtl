#ifndef _VTL_BOT_DATA_H
#define _VTL_BOT_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Telegram-бот «задачи + напоминания» — общие константы модуля. */

/* URL метода API: <base>/bot<token>/<method> */
#define VTL_BOT_API_BASE        "https://api.telegram.org"

#define VTL_BOT_DEFAULT_STORE   "bot_data.json"

/* Тайм-аут long-polling, сек. Заодно задаёт частоту проверки напоминаний. */
#define VTL_BOT_POLL_TIMEOUT_S  10

/* Буфер ответа боту (лимит Telegram — 4096 байт). */
#define VTL_BOT_REPLY_MAX       4000

#ifdef __cplusplus
}
#endif

#endif /* _VTL_BOT_DATA_H */
