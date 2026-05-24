#ifndef _VTL_SCHEDULER_DATA_H
#define _VTL_SCHEDULER_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <time.h>
#include <stdbool.h>

typedef enum _VTL_scheduler_SnType
{
    VTL_sn_kTG      = 0,   /* Telegram  */
    VTL_sn_kReddit  = 1,   /* Reddit    */
    VTL_sn_kVK      = 2,   /* ВКонтакте */
    VTL_sn_kVimeo   = 3,   /* Vimeo     */
    VTL_sn_kUnknown = -1
} VTL_scheduler_SnType;

typedef enum _VTL_scheduler_ContentType
{
    VTL_content_kTxt      = 0,
    VTL_content_kFilePath = 1,
    VTL_content_kUnknown  = -1
} VTL_scheduler_ContentType;

typedef struct _VTL_scheduler_Post
{
    long long                  id;
    time_t                     send_date_time;   /* UTC unix-timestamp */
    char*                      created_user;
    VTL_scheduler_ContentType  content_type;
    char*                      content;          /* текст ИЛИ путь к файлу */
    VTL_scheduler_SnType       sn_type;
    char*                      metadata;         /* JSON-строка */
    bool                       executed;
} VTL_scheduler_Post;

typedef struct _VTL_scheduler_PostList
{
    VTL_scheduler_Post* items;
    size_t              length;
    size_t              capacity;
} VTL_scheduler_PostList;

typedef struct _VTL_scheduler_MetaTG
{
    char chat_id[128];
    char parse_mode[32];   /* MarkdownV2 | HTML | "" */
} VTL_scheduler_MetaTG;

typedef struct _VTL_scheduler_MetaReddit
{
    char subreddit[128];
    char title[256];
} VTL_scheduler_MetaReddit;

typedef struct _VTL_scheduler_MetaVK
{
    long long peer_id;
} VTL_scheduler_MetaVK;

typedef struct _VTL_scheduler_MetaVimeo
{
    char title[256];        /* Название видео (опционально) */
    char description[1024]; /* Описание (опционально)       */
    char tags_csv[512];     /* Теги через запятую (опционально) */
} VTL_scheduler_MetaVimeo;

#ifdef __cplusplus
}
#endif

#endif /* _VTL_SCHEDULER_DATA_H */