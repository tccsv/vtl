#ifndef _VTL_LOGANALYZER_INTERNAL_H
#define _VTL_LOGANALYZER_INTERNAL_H

/*
 * vtl_la_internal.h
 * Внутренние структуры и общий mutex — используются всеми модулями.
 * Этот заголовок НЕ является публичным API — только для internal use.
 */

#include <VTL/log_analyzer/VTL_logAnalyzer.h>
#include <VTL/utils/threading/VTL_utils_threading_thread_compat.h>

/* mutex для вывода в консоль — чтобы строки потоков не перемешивались */
extern vtl_mutex_t g_print_mutex;
extern int              g_silent; /* 1 = не выводить прогресс потоков */

/* задание для потока-парсера */
typedef struct {
    const char   *path;
    long          offset;
    long          size;
    int           thread_id;
    SearchResult  local;
} ChunkArg;

/* задание для потока-поисковика */
typedef struct {
    const LogEntry *entries;
    size_t          from;
    size_t          to;
    SearchResult    local;
    const char     *uid;
    const char     *action;
    const char     *status;
    const char     *platform;
    time_t          ts_from;
    time_t          ts_to;
} SearchArg;

/* задание для потока статистики платформ */
typedef struct {
    const LogEntry *entries;
    size_t          from;
    size_t          to;
    PlatformStats   stats[8];
    size_t          count;
} PlatStatsArg;

/* задание для потока статистики сессий */
typedef struct {
    const LogEntry *entries;
    size_t          from;
    size_t          to;
    double          sum_sec;
    double          min_sec;
    double          max_sec;
    size_t          count;
} DurStatsArg;

/* задание для потока демо-режима */
typedef struct {
    int           thread_id;
    SearchResult *all;
    volatile int *stop;
} DemoArg;

/* внутренние утилиты — используются в нескольких файлах */
VTL_AppResult VTL_logAnalyzer_parse_ResultAppend(SearchResult *r, const LogEntry *e);
int            VTL_logAnalyzer_parse_Line(const char *line, LogEntry *out);
void           VTL_logAnalyzer_stats_ExtractPlatform(const char *detail, char *out, size_t max);
PlatformStats *VTL_logAnalyzer_stats_FindOrAdd(PlatformStats *stats, size_t *count, const char *name);

#endif /* _VTL_LOGANALYZER_INTERNAL_H */
