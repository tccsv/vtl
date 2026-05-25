/*
 * VTL_logAnalyzer_search.c
 *
 * Параллельный поиск по массиву записей лога.
 * Поддерживает поиск по uid, action, status, времени и комбинированный.
 */

#include <VTL/log_analyzer/VTL_logAnalyzer_internal.h>
#include <VTL/log_analyzer/VTL_logAnalyzer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <VTL/utils/threading/VTL_utils_threading_thread_compat.h>

/* ================================================================== */
/* Внутренние функции поиска                                            */
/* ================================================================== */

/* проверить подходит ли запись под критерии */
static int VTL_logAnalyzer_search_EntryMatches(const LogEntry *e, const SearchArg *a)
{
    if (a->uid      && strcmp(e->uid,    a->uid)    != 0) return 0;
    if (a->action   && strcmp(e->action, a->action) != 0) return 0;
    if (a->status   && strcmp(e->status, a->status) != 0) return 0;
    if (a->platform && !strstr(e->detail, a->platform))   return 0;
    if (a->ts_from  && e->ts_epoch < a->ts_from)          return 0;
    if (a->ts_to    && e->ts_epoch > a->ts_to)            return 0;
    return 1;
}

/* универсальный поисковый поток — работает со своим диапазоном */
static void *VTL_logAnalyzer_search_Worker(void *arg)
{
    SearchArg *a = (SearchArg *)arg;
    VTL_logAnalyzer_ResultInit(&a->local);
    for (size_t i = a->from; i < a->to; i++)
        if (VTL_logAnalyzer_search_EntryMatches(&a->entries[i], a))
            VTL_logAnalyzer_parse_ResultAppend(&a->local, &a->entries[i]);
    return NULL;
}

/* запустить n поисковых потоков и собрать результат */
static VTL_AppResult VTL_logAnalyzer_search_Run(const SearchResult *all,
                                    int                 n,
                                    SearchArg          *args,
                                    SearchResult       *out)
{
    vtl_thread_t threads[ANALYZER_MAX_THREADS];
    size_t    per = all->count / (size_t)n;

    for (int i = 0; i < n; i++) {
        args[i].entries = all->entries;
        args[i].from    = (size_t)i * per;
        args[i].to      = (i == n-1) ? all->count : (size_t)(i+1)*per;
        vtl_thread_create(&threads[i], VTL_logAnalyzer_search_Worker, &args[i]);
    }
    for (int i = 0; i < n; i++)
        vtl_thread_join(threads[i]);

    VTL_logAnalyzer_ResultInit(out);
    for (int i = 0; i < n; i++) {
        for (size_t j = 0; j < args[i].local.count; j++)
            VTL_logAnalyzer_parse_ResultAppend(out, &args[i].local.entries[j]);
        VTL_logAnalyzer_ResultFree(&args[i].local);
    }
    return VTL_res_kOk;
}

/* ================================================================== */
/* Публичный API поиска                                                 */
/* ================================================================== */

VTL_AppResult VTL_logAnalyzer_SearchByUid(const SearchResult *all,
                                            int n, const char *uid,
                                            SearchResult *out)
{
    if (!all || !uid || n <= 0 || n > ANALYZER_MAX_THREADS) return VTL_res_kInvalidParamErr;
    SearchArg args[ANALYZER_MAX_THREADS];
    memset(args, 0, sizeof(args));
    for (int i = 0; i < n; i++) args[i].uid = uid;
    return VTL_logAnalyzer_search_Run(all, n, args, out);
}

VTL_AppResult VTL_logAnalyzer_SearchByAction(const SearchResult *all,
                                               int n, const char *action,
                                               SearchResult *out)
{
    if (!all || !action || n <= 0 || n > ANALYZER_MAX_THREADS) return VTL_res_kInvalidParamErr;
    SearchArg args[ANALYZER_MAX_THREADS];
    memset(args, 0, sizeof(args));
    for (int i = 0; i < n; i++) args[i].action = action;
    return VTL_logAnalyzer_search_Run(all, n, args, out);
}

VTL_AppResult VTL_logAnalyzer_SearchErrors(const SearchResult *all,
                                             int n, SearchResult *out)
{
    if (!all || n <= 0 || n > ANALYZER_MAX_THREADS) return VTL_res_kInvalidParamErr;
    SearchArg args[ANALYZER_MAX_THREADS];
    memset(args, 0, sizeof(args));
    for (int i = 0; i < n; i++) args[i].status = "fail";
    return VTL_logAnalyzer_search_Run(all, n, args, out);
}

VTL_AppResult VTL_logAnalyzer_SearchByTime(const SearchResult *all,
                                             int n,
                                             time_t ts_from, time_t ts_to,
                                             SearchResult *out)
{
    if (!all || n <= 0 || n > ANALYZER_MAX_THREADS) return VTL_res_kInvalidParamErr;
    SearchArg args[ANALYZER_MAX_THREADS];
    memset(args, 0, sizeof(args));
    for (int i = 0; i < n; i++) {
        args[i].ts_from = ts_from;
        args[i].ts_to   = ts_to;
    }
    return VTL_logAnalyzer_search_Run(all, n, args, out);
}

VTL_AppResult VTL_logAnalyzer_SearchMulti(const SearchResult   *all,
                                            int                   n,
                                            const SearchCriteria *c,
                                            SearchResult         *out)
{
    if (!all || !c || n <= 0 || n > ANALYZER_MAX_THREADS) return VTL_res_kInvalidParamErr;
    SearchArg args[ANALYZER_MAX_THREADS];
    memset(args, 0, sizeof(args));
    for (int i = 0; i < n; i++) {
        args[i].uid      = c->uid;
        args[i].action   = c->action;
        args[i].status   = c->status;
        args[i].platform = c->platform;
        args[i].ts_from  = c->ts_from;
        args[i].ts_to    = c->ts_to;
    }
    return VTL_logAnalyzer_search_Run(all, n, args, out);
}
