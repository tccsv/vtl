/*
 * VTL_logAnalyzer_stats.c
 *
 * Параллельная статистика: ошибки авторизации, использование платформ,
 * длительность сессий.
 */

#include <VTL/log_analyzer/VTL_logAnalyzer_internal.h>
#include <VTL/log_analyzer/VTL_logAnalyzer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <VTL/utils/threading/VTL_utils_threading_thread_compat.h>

/* ================================================================== */
/* Вспомогательные функции                                              */
/* ================================================================== */

/* вытащить название платформы из поля detail */
void VTL_logAnalyzer_stats_ExtractPlatform(const char *detail, char *out, size_t max)
{
    const char *p = strstr(detail, "platform:");
    if (!p) { strncpy(out, "unknown", max); out[max-1]='\0'; return; }
    p += strlen("platform:");
    size_t i = 0;
    while (*p && *p != ' ' && *p != ',' && i + 1 < max)
        out[i++] = *p++;
    out[i] = '\0';
    if (i == 0) strncpy(out, "unknown", max);
}

/* найти или добавить платформу в локальный массив потока */
PlatformStats *VTL_logAnalyzer_stats_FindOrAdd(PlatformStats *stats, size_t *count,
                               const char *name)
{
    for (size_t i = 0; i < *count; i++)
        if (strcmp(stats[i].platform, name) == 0) return &stats[i];
    if (*count >= 8) return NULL;
    PlatformStats *p = &stats[(*count)++];
    strncpy(p->platform, name, ANALYZER_PLATFORM_MAX-1);
    p->total = p->fail = 0;
    p->fail_rate = 0.0;
    return p;
}

/* ================================================================== */
/* Статистика ошибок авторизации                                        */
/* ================================================================== */

static void *VTL_logAnalyzer_stats_AuthFailsWorker(void *arg)
{
    PlatStatsArg *a = (PlatStatsArg *)arg;
    a->count = 0;
    memset(a->stats, 0, sizeof(a->stats));

    for (size_t i = a->from; i < a->to; i++) {
        const LogEntry *e = &a->entries[i];
        if (strcmp(e->action, "platform_auth") != 0 &&
            strcmp(e->action, "auth_fail")     != 0) continue;

        char plat[ANALYZER_PLATFORM_MAX];
        VTL_logAnalyzer_stats_ExtractPlatform(e->detail, plat, sizeof(plat));

        /* каждый поток пишет в свой локальный массив — mutex не нужен */
        PlatformStats *p = VTL_logAnalyzer_stats_FindOrAdd(a->stats, &a->count, plat);
        if (p) {
            p->total++;
            if (strcmp(e->status, "fail") == 0) p->fail++;
        }
    }
    return NULL;
}

VTL_AppResult VTL_logAnalyzer_StatsAuthFails(const SearchResult *all,
                                               int                 n,
                                               PlatformStats      *out,
                                               size_t             *out_count)
{
    if (!all || n <= 0 || n > ANALYZER_MAX_THREADS || !out || !out_count)
        return VTL_res_kInvalidParamErr;

    PlatStatsArg args[ANALYZER_MAX_THREADS];
    vtl_thread_t    threads[ANALYZER_MAX_THREADS];
    size_t       per = all->count / (size_t)n;

    for (int i = 0; i < n; i++) {
        args[i].entries = all->entries;
        args[i].from    = (size_t)i * per;
        args[i].to      = (i == n-1) ? all->count : (size_t)(i+1)*per;
        vtl_thread_create(&threads[i], VTL_logAnalyzer_stats_AuthFailsWorker, &args[i]);
    }
    for (int i = 0; i < n; i++)
        vtl_thread_join(threads[i]);

    /* сливаем локальную статистику потоков в общую */
    *out_count = 0;
    memset(out, 0, 8 * sizeof(PlatformStats));
    for (int i = 0; i < n; i++) {
        for (size_t j = 0; j < args[i].count; j++) {
            PlatformStats *p = VTL_logAnalyzer_stats_FindOrAdd(out, out_count,
                                              args[i].stats[j].platform);
            if (p) {
                p->total += args[i].stats[j].total;
                p->fail  += args[i].stats[j].fail;
            }
        }
    }
    for (size_t i = 0; i < *out_count; i++)
        out[i].fail_rate = (out[i].total > 0)
            ? (double)out[i].fail / out[i].total * 100.0 : 0.0;

    return VTL_res_kOk;
}

/* ================================================================== */
/* Статистика длительности сессий                                       */
/* ================================================================== */

static void *VTL_logAnalyzer_stats_SessionDurationWorker(void *arg)
{
    DurStatsArg *a = (DurStatsArg *)arg;
    a->sum_sec = 0.0;
    a->min_sec = 1e18;
    a->max_sec = 0.0;
    a->count   = 0;

    for (size_t i = a->from; i < a->to; i++) {
        if (strcmp(a->entries[i].action, "session_start") != 0) continue;
        time_t     start = a->entries[i].ts_epoch;
        const char *uid  = a->entries[i].uid;

        /* ищем парное session_end для того же пользователя */
        for (size_t j = i+1; j < a->to && j < i+10000; j++) {
            if (strcmp(a->entries[j].uid, uid) == 0 &&
                strcmp(a->entries[j].action, "session_end") == 0) {
                double dur = difftime(a->entries[j].ts_epoch, start);
                a->sum_sec += dur;
                if (dur < a->min_sec) a->min_sec = dur;
                if (dur > a->max_sec) a->max_sec = dur;
                a->count++;
                break;
            }
        }
    }
    return NULL;
}

VTL_AppResult VTL_logAnalyzer_StatsSessionDuration(const SearchResult *all,
                                                     int                 n,
                                                     DurationStats      *out)
{
    if (!all || n <= 0 || n > ANALYZER_MAX_THREADS || !out)
        return VTL_res_kInvalidParamErr;

    DurStatsArg args[ANALYZER_MAX_THREADS];
    vtl_thread_t   threads[ANALYZER_MAX_THREADS];
    size_t      per = all->count / (size_t)n;

    for (int i = 0; i < n; i++) {
        args[i].entries = all->entries;
        args[i].from    = (size_t)i * per;
        args[i].to      = (i == n-1) ? all->count : (size_t)(i+1)*per;
        vtl_thread_create(&threads[i], VTL_logAnalyzer_stats_SessionDurationWorker, &args[i]);
    }
    for (int i = 0; i < n; i++)
        vtl_thread_join(threads[i]);

    out->count   = 0;
    out->avg_sec = 0.0;
    out->min_sec = 1e18;
    out->max_sec = 0.0;
    double total_sum = 0.0;

    for (int i = 0; i < n; i++) {
        total_sum    += args[i].sum_sec;
        out->count   += args[i].count;
        if (args[i].count > 0) {
            if (args[i].min_sec < out->min_sec) out->min_sec = args[i].min_sec;
            if (args[i].max_sec > out->max_sec) out->max_sec = args[i].max_sec;
        }
    }
    if (out->count > 0) out->avg_sec = total_sum / out->count;
    if (out->count == 0) out->min_sec = 0.0;
    return VTL_res_kOk;
}

/* ================================================================== */
/* Статистика использования платформ                                    */
/* ================================================================== */

static void *VTL_logAnalyzer_stats_PlatformUsageWorker(void *arg)
{
    PlatStatsArg *a = (PlatStatsArg *)arg;
    a->count = 0;
    memset(a->stats, 0, sizeof(a->stats));

    for (size_t i = a->from; i < a->to; i++) {
        const LogEntry *e = &a->entries[i];
        if (strcmp(e->action, "platform_auth") != 0 &&
            strcmp(e->action, "token_refresh") != 0 &&
            strcmp(e->action, "auth_fail")     != 0) continue;

        char plat[ANALYZER_PLATFORM_MAX];
        VTL_logAnalyzer_stats_ExtractPlatform(e->detail, plat, sizeof(plat));

        PlatformStats *p = VTL_logAnalyzer_stats_FindOrAdd(a->stats, &a->count, plat);
        if (p) {
            p->total++;
            if (strcmp(e->status, "fail") == 0) p->fail++;
        }
    }
    return NULL;
}

VTL_AppResult VTL_logAnalyzer_StatsPlatformUsage(const SearchResult *all,
                                                   int                 n,
                                                   PlatformStats      *out,
                                                   size_t             *out_count)
{
    if (!all || n <= 0 || n > ANALYZER_MAX_THREADS || !out || !out_count)
        return VTL_res_kInvalidParamErr;

    PlatStatsArg args[ANALYZER_MAX_THREADS];
    vtl_thread_t    threads[ANALYZER_MAX_THREADS];
    size_t       per = all->count / (size_t)n;

    for (int i = 0; i < n; i++) {
        args[i].entries = all->entries;
        args[i].from    = (size_t)i * per;
        args[i].to      = (i == n-1) ? all->count : (size_t)(i+1)*per;
        vtl_thread_create(&threads[i], VTL_logAnalyzer_stats_PlatformUsageWorker, &args[i]);
    }
    for (int i = 0; i < n; i++)
        vtl_thread_join(threads[i]);

    *out_count = 0;
    memset(out, 0, 8 * sizeof(PlatformStats));
    for (int i = 0; i < n; i++) {
        for (size_t j = 0; j < args[i].count; j++) {
            PlatformStats *p = VTL_logAnalyzer_stats_FindOrAdd(out, out_count,
                                              args[i].stats[j].platform);
            if (p) {
                p->total += args[i].stats[j].total;
                p->fail  += args[i].stats[j].fail;
            }
        }
    }
    for (size_t i = 0; i < *out_count; i++)
        out[i].fail_rate = (out[i].total > 0)
            ? (double)out[i].fail / out[i].total * 100.0 : 0.0;

    return VTL_res_kOk;
}
