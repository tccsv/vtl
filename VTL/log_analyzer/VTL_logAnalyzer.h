#ifndef _VTL_LOGANALYZER_H
#define _VTL_LOGANALYZER_H

#ifdef __cplusplus
extern "C"
{
#endif


/*
 * vtl_log_analyzer.h
 * Публичный API параллельного анализатора логов VTL.
 *
 * Модули:
 *   vtl_la_parse.c  — парсинг и параллельное чтение файла
 *   vtl_la_search.c — параллельный поиск
 *   vtl_la_stats.c  — параллельная статистика
 *   vtl_la_demo.c   — демо-режим, тест производительности, main
 */

#include <stddef.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/*  Константы                                                           */
/* ------------------------------------------------------------------ */

#define ANALYZER_MAX_THREADS    16
#define ANALYZER_UID_MAX        64
#define ANALYZER_ACTION_MAX     32
#define ANALYZER_STATUS_MAX      8
#define ANALYZER_DETAIL_MAX    256
#define ANALYZER_TS_MAX         32
#define ANALYZER_PLATFORM_MAX   32
#define ANALYZER_PATH_MAX      512

/* ------------------------------------------------------------------ */
/*  Структура одной записи лога                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    char   ts[ANALYZER_TS_MAX];
    char   uid[ANALYZER_UID_MAX];
    char   action[ANALYZER_ACTION_MAX];
    char   status[ANALYZER_STATUS_MAX];
    char   detail[ANALYZER_DETAIL_MAX];
    time_t ts_epoch;
} LogEntry;

/* ------------------------------------------------------------------ */
/*  Массив результатов поиска                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    LogEntry *entries;
    size_t    count;
    size_t    capacity;
} SearchResult;

/* ------------------------------------------------------------------ */
/*  Критерии для комбинированного поиска                                */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *uid;
    const char *action;
    const char *status;
    const char *platform;
    time_t      ts_from;
    time_t      ts_to;
} SearchCriteria;

/* ------------------------------------------------------------------ */
/*  Статистика                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    char   platform[ANALYZER_PLATFORM_MAX];
    size_t total;
    size_t fail;
    double fail_rate;
} PlatformStats;

typedef struct {
    double avg_sec;
    double min_sec;
    double max_sec;
    size_t count;
} DurationStats;

/* ------------------------------------------------------------------ */
/*  Коды возврата                                                       */
/* ------------------------------------------------------------------ */

typedef enum {
    VTL_res_kOk           =  0,
    VTL_res_kFileOpenErr     = -1,
    VTL_res_kMemAllocErr   = -2,
    VTL_res_kErr   = -3,
    VTL_res_kInvalidParamErr = -4
} VTL_AppResult;

/* ================================================================== */
/*  API — vtl_la_parse.c                                               */
/* ================================================================== */

void           VTL_logAnalyzer_ResultInit(SearchResult *r);
void           VTL_logAnalyzer_ResultFree(SearchResult *r);
long           VTL_logAnalyzer_FileGetSize(const char *path);
VTL_AppResult VTL_logAnalyzer_ChunkSplit(const char *path, int n,
                                           long *offsets, long *sizes);
VTL_AppResult VTL_logAnalyzer_ResultMerge(SearchResult *locals, int n,
                                            SearchResult *out);
VTL_AppResult VTL_logAnalyzer_ParallelParse(const char *path, int n_threads,
                                              SearchResult *out);

/* ================================================================== */
/*  API — vtl_la_search.c                                              */
/* ================================================================== */

VTL_AppResult VTL_logAnalyzer_SearchByUid(const SearchResult *all,
                                            int n, const char *uid,
                                            SearchResult *out);
VTL_AppResult VTL_logAnalyzer_SearchByAction(const SearchResult *all,
                                               int n, const char *action,
                                               SearchResult *out);
VTL_AppResult VTL_logAnalyzer_SearchErrors(const SearchResult *all,
                                             int n, SearchResult *out);
VTL_AppResult VTL_logAnalyzer_SearchByTime(const SearchResult *all,
                                             int n,
                                             time_t ts_from, time_t ts_to,
                                             SearchResult *out);
VTL_AppResult VTL_logAnalyzer_SearchMulti(const SearchResult *all,
                                            int n,
                                            const SearchCriteria *c,
                                            SearchResult *out);

/* ================================================================== */
/*  API — vtl_la_stats.c                                               */
/* ================================================================== */

VTL_AppResult VTL_logAnalyzer_StatsAuthFails(const SearchResult *all,
                                               int n,
                                               PlatformStats *out,
                                               size_t *out_count);
VTL_AppResult VTL_logAnalyzer_StatsSessionDuration(const SearchResult *all,
                                                     int n,
                                                     DurationStats *out);
VTL_AppResult VTL_logAnalyzer_StatsPlatformUsage(const SearchResult *all,
                                                   int n,
                                                   PlatformStats *out,
                                                   size_t *out_count);

/* ================================================================== */
/*  API — vtl_la_demo.c                                                */
/* ================================================================== */

void VTL_logAnalyzer_RunDemo(const char *path, int n_threads);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_LOGANALYZER_H */
