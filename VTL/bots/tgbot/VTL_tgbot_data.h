#ifndef _VTL_tgbot_data_H
#define _VTL_tgbot_data_H

#ifdef __cplusplus
extern "C"
{
#endif

#define VTL_tgbot_api_BASE        "https://api.telegram.org"

/* Тайм-аут long-polling, сек. */
#define VTL_tgbot_POLL_TIMEOUT_S  10

/* Буфер ответа. */
#define VTL_tgbot_REPLY_MAX       4000

/* Файл текста по умолчанию. */
#define VTL_tgbot_DEFAULT_TEXT    "text.md"

#ifdef __cplusplus
}
#endif

#endif /* _VTL_tgbot_data_H */
