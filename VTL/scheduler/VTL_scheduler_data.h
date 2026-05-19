#ifndef _VTL_SCHEDULER_DATA_H
#define _VTL_SCHEDULER_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <time.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* Перечисления                                                         */
/* ------------------------------------------------------------------ */

/* sn_type — тип социальной сети */
typedef enum _VTL_scheduler_SnType
{
    VTL_sn_kTG      = 0,   /* Telegram  */
    VTL_sn_kReddit  = 1,   /* Reddit    */
    VTL_sn_kVK      = 2,   /* ВКонтакте */
    VTL_sn_kUnknown = -1
} VTL_scheduler_SnType;

/* content_type — что лежит в поле content */
typedef enum _VTL_scheduler_ContentType
{
    VTL_content_kTxt      = 0,   /* сырой текст            */
    VTL_content_kFilePath = 1,   /* путь к файлу на диске  */
    VTL_content_kUnknown  = -1
} VTL_scheduler_ContentType;

/* ------------------------------------------------------------------ */
/* Одна запись из таблицы scheduled_posts                              */
/* ------------------------------------------------------------------ */
typedef struct _VTL_scheduler_Post
{
    long long                  id;
    time_t                     send_date_time;   /* UTC unix-timestamp    */
    char*                      created_user;
    VTL_scheduler_ContentType  content_type;
    char*                      content;          /* текст ИЛИ путь к файлу */
    VTL_scheduler_SnType       sn_type;
    char*                      metadata;         /* JSON-строка           */
    bool                       executed;
} VTL_scheduler_Post;

/* ------------------------------------------------------------------ */
/* Массив записей                                                       */
/* ------------------------------------------------------------------ */
typedef struct _VTL_scheduler_PostList
{
    VTL_scheduler_Post* items;
    size_t              length;
    size_t              capacity;
} VTL_scheduler_PostList;

/* ------------------------------------------------------------------ */
/* Метаданные конкретных соц-сетей (десериализованные)                 */
/* ------------------------------------------------------------------ */

/* Telegram: chat_id + parse_mode */
typedef struct _VTL_scheduler_MetaTG
{
    char chat_id[128];
    char parse_mode[32];   /* MarkdownV2 | HTML | "" */
} VTL_scheduler_MetaTG;

/* Reddit: subreddit + title */
typedef struct _VTL_scheduler_MetaReddit
{
    char subreddit[128];
    char title[256];
} VTL_scheduler_MetaReddit;

/* VK: peer_id */
typedef struct _VTL_scheduler_MetaVK
{
    long long peer_id;
} VTL_scheduler_MetaVK;

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_DATA_H */
