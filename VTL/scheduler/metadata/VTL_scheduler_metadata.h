#ifndef _VTL_SCHEDULER_METADATA_H
#define _VTL_SCHEDULER_METADATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/scheduler/VTL_scheduler_data.h>
#include <VTL/VTL_app_result.h>

/* JSON-схема входа Deserialize*:
 *   TG:     { "chat_id": "-100123", "parse_mode": "MarkdownV2" }
 *   Reddit: { "subreddit": "news",  "title": "My post" }
 *   VK:     { "peer_id":  123456 } */
VTL_AppResult VTL_scheduler_meta_DeserializeTG(const char*           json_str,
                                               VTL_scheduler_MetaTG* out);

VTL_AppResult VTL_scheduler_meta_DeserializeReddit(const char*               json_str,
                                                   VTL_scheduler_MetaReddit* out);

VTL_AppResult VTL_scheduler_meta_DeserializeVK(const char*           json_str,
                                               VTL_scheduler_MetaVK* out);

VTL_AppResult VTL_scheduler_meta_DeserializeVimeo(const char*              json_str,
                                                  VTL_scheduler_MetaVimeo* out);

/* Результат Serialize* — malloc'нутая строка; освобождает вызывающий. */
char* VTL_scheduler_meta_SerializeTG(const VTL_scheduler_MetaTG* meta);

char* VTL_scheduler_meta_SerializeReddit(const VTL_scheduler_MetaReddit* meta);

char* VTL_scheduler_meta_SerializeVK(const VTL_scheduler_MetaVK* meta);

char* VTL_scheduler_meta_SerializeVimeo(const VTL_scheduler_MetaVimeo* meta);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_METADATA_H */