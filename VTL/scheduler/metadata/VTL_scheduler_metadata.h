#ifndef _VTL_SCHEDULER_METADATA_H
#define _VTL_SCHEDULER_METADATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/scheduler/VTL_scheduler_data.h>
#include <VTL/VTL_app_result.h>

/* ------------------------------------------------------------------ */
/* Десериализация: JSON-строка → структура метаданных                  */
/* ------------------------------------------------------------------ */

/* Парсит JSON-строку metadata записи Telegram:
 * { "chat_id": "-100123", "parse_mode": "MarkdownV2" }              */
VTL_AppResult VTL_scheduler_meta_DeserializeTG(const char*            json_str,
                                                VTL_scheduler_MetaTG*  out);

/* Парсит JSON-строку metadata записи Reddit:
 * { "subreddit": "news", "title": "My post" }                       */
VTL_AppResult VTL_scheduler_meta_DeserializeReddit(const char*               json_str,
                                                    VTL_scheduler_MetaReddit* out);

/* Парсит JSON-строку metadata записи VK:
 * { "peer_id": 123456 }                                              */
VTL_AppResult VTL_scheduler_meta_DeserializeVK(const char*            json_str,
                                                VTL_scheduler_MetaVK*  out);

/* ------------------------------------------------------------------ */
/* Сериализация: структура метаданных → JSON-строка                    */
/* Результат — выделенная через malloc строка, вызывающий free()-ит.  */
/* ------------------------------------------------------------------ */

char* VTL_scheduler_meta_SerializeTG(const VTL_scheduler_MetaTG* meta);

char* VTL_scheduler_meta_SerializeReddit(const VTL_scheduler_MetaReddit* meta);

char* VTL_scheduler_meta_SerializeVK(const VTL_scheduler_MetaVK* meta);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_METADATA_H */
