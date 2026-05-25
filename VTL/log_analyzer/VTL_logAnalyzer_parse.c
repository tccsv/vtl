/*
 * VTL_logAnalyzer_parse.c
 *
 * Парсер JSONL строк + параллельное чтение файла.
 * Здесь живёт всё что связано с чтением и разбором лога.
 */


#include <VTL/log_analyzer/VTL_logAnalyzer_internal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <VTL/utils/threading/VTL_utils_threading_thread_compat.h>

/* ================================================================== */
/* Утилиты для массива результатов                                      */
/* ================================================================== */

void VTL_logAnalyzer_ResultInit(SearchResult *r)
{
    r->entries  = NULL;
    r->count    = 0;
    r->capacity = 0;
}

void VTL_logAnalyzer_ResultFree(SearchResult *r)
{
    free(r->entries);
    r->entries  = NULL;
    r->count    = 0;
    r->capacity = 0;
}

/* добавить запись в корзину — при нехватке места увеличиваем вдвое */
VTL_AppResult VTL_logAnalyzer_parse_ResultAppend(SearchResult *r, const LogEntry *e)
{
    if (r->count >= r->capacity) {
        size_t new_cap = (r->capacity == 0) ? 256 : r->capacity * 2;
        LogEntry *tmp = realloc(r->entries, new_cap * sizeof(LogEntry));
        if (!tmp) return VTL_res_kMemAllocErr;
        r->entries  = tmp;
        r->capacity = new_cap;
    }
    r->entries[r->count++] = *e;
    return VTL_res_kOk;
}

/* ================================================================== */
/* Парсер строк JSONL                                                   */
/* ================================================================== */

/* найти значение поля в строке JSON по имени ключа */
static int VTL_logAnalyzer_parse_JsonGetStr(const char *line, const char *key,
                           char *out, size_t out_max)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *p = strstr(line, search);
    if (!p) return 0;
    p += strlen(search);
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < out_max) {
        if (*p == '\\' && *(p+1)) { p++; }
        out[i++] = *p++;
    }
    out[i] = '\0';
    return 1;
}

/* перевести строку времени в числовой time_t */
static time_t VTL_logAnalyzer_parse_Timestamp(const char *ts)
{
    struct tm t = {0};
    if (sscanf(ts, "%d-%d-%dT%d:%d:%dZ",
               &t.tm_year, &t.tm_mon, &t.tm_mday,
               &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
        return 0;
    t.tm_year -= 1900;
    t.tm_mon  -= 1;
    t.tm_isdst = 0;
#if defined(_WIN32) || defined(__MINGW32__)
    return _mkgmtime(&t);
#else
    return timegm(&t);
#endif
}

/* разобрать одну строку лога в структуру LogEntry */
int VTL_logAnalyzer_parse_Line(const char *line, LogEntry *out)
{
    if (!VTL_logAnalyzer_parse_JsonGetStr(line, "ts",     out->ts,     sizeof(out->ts)))     return 0;
    if (!VTL_logAnalyzer_parse_JsonGetStr(line, "uid",    out->uid,    sizeof(out->uid)))    return 0;
    if (!VTL_logAnalyzer_parse_JsonGetStr(line, "action", out->action, sizeof(out->action))) return 0;
    if (!VTL_logAnalyzer_parse_JsonGetStr(line, "status", out->status, sizeof(out->status))) return 0;
    VTL_logAnalyzer_parse_JsonGetStr(line, "detail", out->detail, sizeof(out->detail));
    out->ts_epoch = VTL_logAnalyzer_parse_Timestamp(out->ts);
    return 1;
}

/* ================================================================== */
/* Параллельное чтение файла                                            */
/* ================================================================== */

/* узнать размер файла в байтах */
long VTL_logAnalyzer_FileGetSize(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0) ? (long)st.st_size : -1L;
}

/* разделить файл на n кусков по границам строк */
VTL_AppResult VTL_logAnalyzer_ChunkSplit(const char *path,
                                           int         n,
                                           long       *offsets,
                                           long       *sizes)
{
    if (!path || n <= 0 || !offsets || !sizes) return VTL_res_kInvalidParamErr;

    long total = VTL_logAnalyzer_FileGetSize(path);
    if (total <= 0) return VTL_res_kFileOpenErr;

    FILE *f = fopen(path, "rb");
    if (!f) return VTL_res_kFileOpenErr;

    long chunk = total / n;

    for (int i = 0; i < n; i++) {
        offsets[i] = i * chunk;
        if (i == n - 1) {
            sizes[i] = total - offsets[i];
        } else {
            /* сдвигаем границу до ближайшего \n */
            long end = (i + 1) * chunk;
            fseek(f, end, SEEK_SET);
            int c;
            while ((c = fgetc(f)) != EOF && c != '\n') end++;
            if (c == '\n') end++;
            sizes[i] = end - offsets[i];
        }
    }

    fclose(f);
    return VTL_res_kOk;
}

/* функция потока-парсера — читает свой кусок файла */
static void *VTL_logAnalyzer_parse_ChunkWorker(void *arg)
{
    ChunkArg *a = (ChunkArg *)arg;
    VTL_logAnalyzer_ResultInit(&a->local);

    if (!g_silent) {
        vtl_mutex_lock(&g_print_mutex);
        printf("      Поток %d: старт  (байты %ld — %ld)\n",
               a->thread_id, a->offset, a->offset + a->size - 1);
        fflush(stdout);
        vtl_mutex_unlock(&g_print_mutex);
    }

    /* каждый поток открывает файл сам — у каждого своя позиция чтения */
    FILE *f = fopen(a->path, "rb");
    if (!f) return NULL;

    fseek(f, a->offset, SEEK_SET);

    char     line[4096];
    long     read_bytes = 0;
    LogEntry entry;

    while (read_bytes < a->size && fgets(line, sizeof(line), f)) {
        read_bytes += (long)strlen(line);
        /* убираем 
 на Windows */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len-1] == '\r') line[--len] = '\0';
        if (VTL_logAnalyzer_parse_Line(line, &entry))
            VTL_logAnalyzer_parse_ResultAppend(&a->local, &entry);
    }

    fclose(f);

    if (!g_silent) {
        vtl_mutex_lock(&g_print_mutex);
        printf("      Поток %d: готов  → %zu записей\n",
               a->thread_id, a->local.count);
        fflush(stdout);
        vtl_mutex_unlock(&g_print_mutex);
    }

    return NULL;
}

/* слить n локальных корзин в одну общую */
VTL_AppResult VTL_logAnalyzer_ResultMerge(SearchResult *locals,
                                            int           n,
                                            SearchResult *out)
{
    VTL_logAnalyzer_ResultInit(out);
    for (int i = 0; i < n; i++)
        for (size_t j = 0; j < locals[i].count; j++)
            if (VTL_logAnalyzer_parse_ResultAppend(out, &locals[i].entries[j]) != VTL_res_kOk)
                return VTL_res_kMemAllocErr;
    return VTL_res_kOk;
}

/* запустить n потоков для параллельного парсинга файла */
VTL_AppResult VTL_logAnalyzer_ParallelParse(const char   *path,
                                              int           n_threads,
                                              SearchResult *out)
{
    if (!path || n_threads <= 0 || n_threads > ANALYZER_MAX_THREADS || !out)
        return VTL_res_kInvalidParamErr;

    long offsets[ANALYZER_MAX_THREADS];
    long sizes[ANALYZER_MAX_THREADS];

    VTL_AppResult rc = VTL_logAnalyzer_ChunkSplit(path, n_threads, offsets, sizes);
    if (rc != VTL_res_kOk) return rc;

    ChunkArg  args[ANALYZER_MAX_THREADS];
    vtl_thread_t threads[ANALYZER_MAX_THREADS];

    for (int i = 0; i < n_threads; i++) {
        args[i].path      = path;
        args[i].offset    = offsets[i];
        args[i].size      = sizes[i];
        args[i].thread_id = i;
        vtl_thread_create(&threads[i], VTL_logAnalyzer_parse_ChunkWorker, &args[i]);
    }
    for (int i = 0; i < n_threads; i++)
        vtl_thread_join(threads[i]);

    SearchResult locals[ANALYZER_MAX_THREADS];
    for (int i = 0; i < n_threads; i++)
        locals[i] = args[i].local;

    rc = VTL_logAnalyzer_ResultMerge(locals, n_threads, out);

    for (int i = 0; i < n_threads; i++)
        VTL_logAnalyzer_ResultFree(&locals[i]);

    return rc;
}
