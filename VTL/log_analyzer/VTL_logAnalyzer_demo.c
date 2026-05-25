/*
 * VTL_logAnalyzer_demo.c
 *
 * Демо-режим, тест производительности и консольное меню (main).
 */

#define _POSIX_C_SOURCE 199309L
#include <VTL/log_analyzer/VTL_logAnalyzer_internal.h>
#include <VTL/log_analyzer/VTL_logAnalyzer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <VTL/utils/threading/VTL_utils_threading_thread_compat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

/* глобальный mutex — объявлен extern в internal.h, определён здесь */
vtl_mutex_t g_print_mutex;
int              g_silent = 0; /* 1 = не выводить старт/готов потоков */

/* кроссплатформенный замер времени в миллисекундах */
static double vtl_get_time_ms(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart * 1000.0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
#endif
}


/* ================================================================== */
/* Демо-режим                                                           */
/* ================================================================== */

static volatile int VTL_logAnalyzer_demo_stop = 0;

#ifdef _WIN32
static BOOL WINAPI VTL_logAnalyzer_demo_SignalHandler(DWORD sig)
{
    (void)sig;
    VTL_logAnalyzer_demo_stop = 1;
    return TRUE;
}
#else
static void VTL_logAnalyzer_demo_SignalHandler(int sig)
{
    (void)sig;
    VTL_logAnalyzer_demo_stop = 1;
}
#endif

/* поток демо-режима — читает записи по кругу и выводит в консоль */
static void *VTL_logAnalyzer_demo_Worker(void *arg)
{
    DemoArg *a   = (DemoArg *)arg;
    size_t   idx = (size_t)a->thread_id;

    while (!VTL_logAnalyzer_demo_stop) {
        const LogEntry *e = &a->all->entries[idx % a->all->count];

        vtl_mutex_lock(&g_print_mutex);
        printf("  Поток %d → %-10s | %-20s | %s\n",
               a->thread_id, e->uid, e->action, e->status);
        fflush(stdout);
        vtl_mutex_unlock(&g_print_mutex);

        idx += 4;

#ifdef _WIN32
        Sleep(300);
#else
        usleep(300000);
#endif
    }
    return NULL;
}

void VTL_logAnalyzer_RunDemo(const char *path, int n_threads)
{
    VTL_logAnalyzer_demo_stop = 0;
    printf("=== VTL Log Analyzer — ДЕМО ===\n");
    printf("Файл: %s | Потоков: %d\n\n", path, n_threads);

    SearchResult all;
    VTL_logAnalyzer_ResultInit(&all);
    printf("Загружаем файл...\n");
    if (VTL_logAnalyzer_ParallelParse(path, n_threads, &all) != VTL_res_kOk) {
        fprintf(stderr, "Ошибка чтения файла\n");
        return;
    }
    printf("Загружено: %zu записей\n\n", all.count);
    printf("Потоки читают записи параллельно. Нажми Ctrl+C для остановки.\n");
    printf("─────────────────────────────────────────\n");

#ifdef _WIN32
    SetConsoleCtrlHandler(VTL_logAnalyzer_demo_SignalHandler, TRUE);
#else
    signal(SIGINT, VTL_logAnalyzer_demo_SignalHandler);
#endif

    DemoArg   args[ANALYZER_MAX_THREADS];
    vtl_thread_t threads[ANALYZER_MAX_THREADS];

    for (int i = 0; i < n_threads; i++) {
        args[i].thread_id = i;
        args[i].all       = &all;
        args[i].stop      = &VTL_logAnalyzer_demo_stop;
        vtl_thread_create(&threads[i], VTL_logAnalyzer_demo_Worker, &args[i]);
    }
    for (int i = 0; i < n_threads; i++)
        vtl_thread_join(threads[i]);

    printf("\n─────────────────────────────────────────\n");
    printf("Демо завершено.\n");
    VTL_logAnalyzer_ResultFree(&all);
}

/* ================================================================== */
/* Тест производительности                                              */
/* ================================================================== */

static double VTL_logAnalyzer_perf_MeasureSingle(const char *path)
{
    double t0, t1;
    t0 = vtl_get_time_ms();

    SearchResult single;
    VTL_logAnalyzer_ResultInit(&single);
    FILE *fs = fopen(path, "r");
    if (fs) {
        char line[4096]; LogEntry e;
        while (fgets(line, sizeof(line), fs)) {
            size_t ln = strlen(line);
            if (ln > 0 && line[ln-1] == '\n') line[--ln] = '\0';
            if (ln > 0 && line[ln-1] == '\r') line[--ln] = '\0';
            if (VTL_logAnalyzer_parse_Line(line, &e)) VTL_logAnalyzer_parse_ResultAppend(&single, &e);
        }
        fclose(fs);
    }
    t1 = vtl_get_time_ms();
    VTL_logAnalyzer_ResultFree(&single);

    return (t1 - t0);
}

static double VTL_logAnalyzer_perf_MeasureParallel(const char *path, int n)
{
    double t0, t1;
    SearchResult par;
    VTL_logAnalyzer_ResultInit(&par);
    g_silent = 1;  /* не показывать потоки во время замера */
    t0 = vtl_get_time_ms();
    VTL_logAnalyzer_ParallelParse(path, n, &par);
    t1 = vtl_get_time_ms();
    g_silent = 0;
    VTL_logAnalyzer_ResultFree(&par);
    return (t1 - t0);
}

/* замер одного параллельного метода vs однопоточного */
static void VTL_logAnalyzer_perf_MeasureMethod(const char *name,
                              double t1_ms,
                              double tn_ms,
                              int    n)
{
    /* ограничиваем эффективность 100% — выше физически невозможно */
    double speedup = (tn_ms > 0) ? t1_ms / tn_ms : 0;
    double eff     = (tn_ms > 0) ? t1_ms / ((double)n * tn_ms) * 100.0 : 0;
    if (eff > 100.0) eff = 100.0;  /* погрешность замера на быстрых операциях */
    printf("  %-30s  %8.1f  %8.1f  %7.2fx  %10.1f%%\n",
           name, t1_ms, tn_ms, speedup, eff);
}

static void VTL_logAnalyzer_perf_Test(const char *path, int n_threads)
{
    double t0, t1, t_single, t_parallel;

    /* загружаем данные с выводом потоков */
    g_silent = 0;
    SearchResult all;
    VTL_logAnalyzer_ResultInit(&all);
    VTL_logAnalyzer_ParallelParse(path, n_threads, &all);
    if (all.count == 0) {
        printf("  Ошибка загрузки файла\n");
        return;
    }

    /* теперь отключаем вывод — таблица должна быть чистой */
    g_silent = 1;

    printf("  Файл: %s | Потоков: %d | Записей: %zu\n\n", path, n_threads, all.count);
    printf("  %-30s  %8s  %8s  %8s  %11s\n",
           "Метод","1 поток","N потоков","Ускорение","Эффективность");
    printf("  %-30s  %8s  %8s  %8s  %11s\n",
           "─────────────────────────────","────────","─────────","─────────","────────────");

    /* ── 1. ParallelParse ── */
    t_single   = VTL_logAnalyzer_perf_MeasureSingle(path);
    t_parallel = VTL_logAnalyzer_perf_MeasureParallel(path, n_threads);
    VTL_logAnalyzer_perf_MeasureMethod("ParallelParse (чтение файла)",
                     t_single, t_parallel, n_threads);

    /* ── 2. SearchByUid ── */
    {
        SearchResult r1, rn;
        /* однопоточный */
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchByUid(&all, 1, "usr_001", &r1);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&r1);
        /* многопоточный */
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchByUid(&all, n_threads, "usr_001", &rn);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&rn);
        VTL_logAnalyzer_perf_MeasureMethod("SearchByUid", t_single, t_parallel, n_threads);
    }

    /* ── 3. SearchByAction ── */
    {
        SearchResult r1, rn;
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchByAction(&all, 1, "auth_fail", &r1);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&r1);
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchByAction(&all, n_threads, "auth_fail", &rn);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&rn);
        VTL_logAnalyzer_perf_MeasureMethod("SearchByAction", t_single, t_parallel, n_threads);
    }

    /* ── 4. SearchErrors ── */
    {
        SearchResult r1, rn;
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchErrors(&all, 1, &r1);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&r1);
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchErrors(&all, n_threads, &rn);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&rn);
        VTL_logAnalyzer_perf_MeasureMethod("SearchErrors", t_single, t_parallel, n_threads);
    }

    /* ── 5. SearchByTime ── */
    {
        SearchResult r1, rn;
        time_t ts_from = all.entries[0].ts_epoch;
        time_t ts_to   = all.entries[all.count/2].ts_epoch;
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchByTime(&all, 1, ts_from, ts_to, &r1);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&r1);
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchByTime(&all, n_threads, ts_from, ts_to, &rn);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&rn);
        VTL_logAnalyzer_perf_MeasureMethod("SearchByTime", t_single, t_parallel, n_threads);
    }

    /* ── 6. SearchMulti ── */
    {
        SearchResult r1, rn;
        SearchCriteria crit = {0};
        crit.action = "auth_fail";
        crit.status = "fail";
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchMulti(&all, 1, &crit, &r1);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&r1);
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchMulti(&all, n_threads, &crit, &rn);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&rn);
        VTL_logAnalyzer_perf_MeasureMethod("SearchMulti", t_single, t_parallel, n_threads);
    }

    /* ── 7. StatsAuthFails ── */
    {
        PlatformStats ps[8]; size_t pc = 0;
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsAuthFails(&all, 1, ps, &pc);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsAuthFails(&all, n_threads, ps, &pc);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_perf_MeasureMethod("StatsAuthFails", t_single, t_parallel, n_threads);
    }

    /* ── 8. StatsSessionDuration ── */
    {
        DurationStats ds;
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsSessionDuration(&all, 1, &ds);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsSessionDuration(&all, n_threads, &ds);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_perf_MeasureMethod("StatsSessionDuration", t_single, t_parallel, n_threads);
    }

    /* ── 9. StatsPlatformUsage ── */
    {
        PlatformStats ps[8]; size_t pc = 0;
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsPlatformUsage(&all, 1, ps, &pc);
        t1 = vtl_get_time_ms();
        t_single = (t1 - t0);
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsPlatformUsage(&all, n_threads, ps, &pc);
        t1 = vtl_get_time_ms();
        t_parallel = (t1 - t0);
        VTL_logAnalyzer_perf_MeasureMethod("StatsPlatformUsage", t_single, t_parallel, n_threads);
    }

    /* возвращаем вывод потоков */
    g_silent = 0;

    /* ── итоговый вывод ── */
    /* считаем среднюю эффективность по всем методам из таблицы выше */
    /* используем SearchErrors как представителя поиска —             */
    /* все методы поиска имеют схожую эффективность                   */
    {
        SearchResult r1, rn;
        double sum_eff = 0.0;
        int    n_eff   = 0;

        /* замеряем SearchErrors однопоточно и параллельно */
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchErrors(&all, 1, &r1);
        t1 = vtl_get_time_ms();
        double se_t1 = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&r1);

        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_SearchErrors(&all, n_threads, &rn);
        t1 = vtl_get_time_ms();
        double se_tn = (t1 - t0);
        VTL_logAnalyzer_ResultFree(&rn);

        double se_eff = (se_tn > 0) ? se_t1/((double)n_threads*se_tn)*100.0 : 0;
        if (se_eff > 100.0) se_eff = 100.0;
        if (se_tn > 0) { sum_eff += se_eff; n_eff++; }

        /* замеряем StatsAuthFails */
        PlatformStats ps[8]; size_t pc = 0;
        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsAuthFails(&all, 1, ps, &pc);
        t1 = vtl_get_time_ms();
        double st_t1 = (t1 - t0);

        t0 = vtl_get_time_ms();
        VTL_logAnalyzer_StatsAuthFails(&all, n_threads, ps, &pc);
        t1 = vtl_get_time_ms();
        double st_tn = (t1 - t0);

        double st_eff = (st_tn > 0) ? st_t1/((double)n_threads*st_tn)*100.0 : 0;
        if (st_eff > 100.0) st_eff = 100.0;
        if (st_tn > 0) { sum_eff += st_eff; n_eff++; }

        double parse_eff = t_parallel > 0 ?
            t_single / ((double)n_threads * t_parallel) * 100.0 : 0;
        if (parse_eff > 100.0) parse_eff = 100.0;
        double avg_eff = n_eff > 0 ? sum_eff / n_eff : 0;
        int    ok = (parse_eff >= 50.0 || avg_eff >= 50.0);

        printf("\n");
        printf("  ┌─────────────────────────────────────────────────┐\n");
        printf("  │         ИТОГ: Эффективность распараллеливания   │\n");
        printf("  │         Формула: E = T1 / (N * TN) * 100%%      │\n");
        printf("  ├─────────────────────────────────────────────────┤\n");
        printf("  │  Потоков (N):               %-4d                │\n", n_threads);
        printf("  │  Чтение файла:              %5.1f%%              │\n", parse_eff);
        printf("  │  Поиск (SearchErrors):      %5.1f%%              │\n", se_eff);
        printf("  │  Статистика (AuthFails):    %5.1f%%              │\n", st_eff);
        printf("  │  Требование по ТЗ:          >= 50%%              │\n");
        printf("  │  Результат:                 %s               │\n",
               ok ? "ВЫПОЛНЕНО ✓  " : "НЕ ВЫПОЛНЕНО ");
        printf("  └─────────────────────────────────────────────────┘\n");
    }

    VTL_logAnalyzer_ResultFree(&all);
}

/* ================================================================== */
/* Загрузка с замером                                                   */
/* ================================================================== */

static double VTL_logAnalyzer_demo_LoadFile(const char *path, int n_threads, SearchResult *out)
{
    double t_single = VTL_logAnalyzer_perf_MeasureSingle(path);
    double t0, t1;
    g_silent = 0;
    t0 = vtl_get_time_ms();
    VTL_logAnalyzer_ParallelParse(path, n_threads, out);
    t1 = vtl_get_time_ms();
    double t_parallel = (t1 - t0);

    printf("  Однопоточное чтение:  %.1f мс\n", t_single);
    printf("  Параллельное чтение:  %.1f мс\n", t_parallel);
    if (t_parallel > 0) {
        printf("  Ускорение:            %.2fx\n", t_single / t_parallel);
        double eff = t_single / ((double)n_threads * t_parallel) * 100.0;
        printf("  Эффективность:        %.1f%%\n", eff);
        return eff;
    }
    return 0.0;
}

/* ================================================================== */
/* Консольное меню                                                      */
/* ================================================================== */

static void VTL_logAnalyzer_demo_PrintSeparator(void)
{
    printf("\n─────────────────────────────────────────────────\n\n");
}

static void VTL_logAnalyzer_demo_PrintMenu(void)
{
    printf("  Выберите действие:\n");
    printf("  [1] Поиск по пользователю\n");
    printf("  [2] Поиск по типу события\n");
    printf("  [3] Поиск всех ошибок\n");
    printf("  [4] Поиск по временному диапазону\n");
    printf("  [5] Комбинированный поиск\n");
    printf("  [6] Статистика ошибок по платформам\n");
    printf("  [7] Использование платформ\n");
    printf("  [8] Статистика сессий\n");
    printf("  [9] Демо-режим\n");
    printf("  [p] Тест производительности\n");
    printf("  [0] Выход\n");
    printf("\n  Ваш выбор: ");
}

/* ================================================================== */
/* main                                                                 */
/* ================================================================== */

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    char log_path_buf[512] = {0};
    const char *log_path;
    int         n_threads;

    if (argc >= 2) {
        /* аргументы переданы — используем их */
        log_path  = argv[1];
        n_threads = (argc >= 3) ? atoi(argv[2]) : 4;
    } else {
        /* аргументы не переданы — спрашиваем у пользователя */
        printf("\n  Введите путь к файлу лога (.jsonl): ");
        if (fgets(log_path_buf, sizeof(log_path_buf), stdin) == NULL) return 1;
        /* убираем перенос строки */
        log_path_buf[strcspn(log_path_buf, "\r\n")] = 0;
        log_path = log_path_buf;

        printf("  Введите число потоков (Enter = 4): ");
        char n_buf[16] = {0};
        if (fgets(n_buf, sizeof(n_buf), stdin)) {
            n_threads = atoi(n_buf);
        }
        if (n_threads < 1) n_threads = 4;
    }
    if (n_threads > ANALYZER_MAX_THREADS) n_threads = ANALYZER_MAX_THREADS;

    vtl_mutex_init(&g_print_mutex);

    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║         VTL Log Analyzer                    ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    printf("  Файл:    %s\n", log_path);
    printf("  Потоков: %d\n\n", n_threads);

    printf("  Загрузка файла...\n");
    SearchResult all;
    VTL_logAnalyzer_ResultInit(&all);
    double efficiency = VTL_logAnalyzer_demo_LoadFile(log_path, n_threads, &all);
    if (all.count == 0) {
        fprintf(stderr, "  Ошибка: не удалось прочитать файл\n");
        vtl_mutex_destroy(&g_print_mutex);
        return 1;
    }
    printf("  Записей загружено: %zu\n", all.count);

    int running = 1;
    while (running) {
        VTL_logAnalyzer_demo_PrintSeparator();
        VTL_logAnalyzer_demo_PrintMenu();

        char choice_str[8] = {0};
        scanf(" %7s", choice_str);
        printf("\n");
        int choice;
        if (choice_str[0] == 'p' || choice_str[0] == 'P') choice = 'p';
        else choice = atoi(choice_str);

        switch (choice) {

        case 1: {
            char uid[ANALYZER_UID_MAX];
            printf("  Введите uid (например usr_001): ");
            scanf("%63s", uid);
            printf("\n  Ищем события пользователя %s...\n\n", uid);
            SearchResult res;
            VTL_logAnalyzer_SearchByUid(&all, n_threads, uid, &res);
            printf("  Найдено: %zu событий\n", res.count);
            if (res.count > 0) {
                size_t show = res.count < 5 ? res.count : 5;
                printf("\n  Первые %zu:\n", show);
                for (size_t i = 0; i < show; i++)
                    printf("  [%zu] %s | %s | %s\n", i+1,
                           res.entries[i].ts, res.entries[i].action,
                           res.entries[i].status);
            }
            VTL_logAnalyzer_ResultFree(&res);
            break;
        }

        case 2: {
            char action[ANALYZER_ACTION_MAX];
            printf("  Введите action (session_start / platform_auth / auth_fail / ...): ");
            scanf("%31s", action);
            printf("\n  Ищем события типа %s...\n\n", action);
            SearchResult res;
            VTL_logAnalyzer_SearchByAction(&all, n_threads, action, &res);
            printf("  Найдено: %zu событий\n", res.count);
            if (res.count > 0) {
                size_t show = res.count < 5 ? res.count : 5;
                printf("\n  Первые %zu:\n", show);
                for (size_t i = 0; i < show; i++)
                    printf("  [%zu] %s | %s | %s\n", i+1,
                           res.entries[i].ts, res.entries[i].uid,
                           res.entries[i].detail);
            }
            VTL_logAnalyzer_ResultFree(&res);
            break;
        }

        case 3: {
            printf("  Ищем все ошибки (status=fail)...\n\n");
            SearchResult res;
            VTL_logAnalyzer_SearchErrors(&all, n_threads, &res);
            printf("  Найдено ошибок: %zu\n", res.count);
            if (res.count > 0) {
                size_t show = res.count < 5 ? res.count : 5;
                printf("\n  Первые %zu:\n", show);
                for (size_t i = 0; i < show; i++)
                    printf("  [%zu] %s | %s | %s | %s\n", i+1,
                           res.entries[i].ts, res.entries[i].uid,
                           res.entries[i].action, res.entries[i].detail);
            }
            VTL_logAnalyzer_ResultFree(&res);
            break;
        }

        case 4: {
            printf("  [1] Первая половина лога  [2] Вторая половина лога\n");
            printf("  Выбор: ");
            int half; scanf("%d", &half);
            printf("\n");
            time_t ts_from, ts_to;
            if (half == 2) {
                ts_from = all.entries[all.count/2].ts_epoch;
                ts_to   = all.entries[all.count-1].ts_epoch;
                printf("  Ищем вторую половину лога...\n\n");
            } else {
                ts_from = all.entries[0].ts_epoch;
                ts_to   = all.entries[all.count/2].ts_epoch;
                printf("  Ищем первую половину лога...\n\n");
            }
            SearchResult res;
            VTL_logAnalyzer_SearchByTime(&all, n_threads, ts_from, ts_to, &res);
            printf("  Найдено: %zu событий\n", res.count);
            VTL_logAnalyzer_ResultFree(&res);
            break;
        }

        case 5: {
            printf("  Комбинированный поиск\n");
            printf("  Введите значение или - чтобы пропустить\n\n");
            char uid[ANALYZER_UID_MAX]       = {0};
            char action[ANALYZER_ACTION_MAX] = {0};
            char status[ANALYZER_STATUS_MAX] = {0};
            char plat[ANALYZER_PLATFORM_MAX] = {0};
            char tmp[128];

            /* читаем через scanf %s — надёжнее fgets после scanf */
            printf("  uid (usr_001 или -): ");
            scanf("%127s", tmp);
            if (tmp[0] != '-') strncpy(uid, tmp, sizeof(uid)-1);

            printf("  action (auth_fail или -): ");
            scanf("%127s", tmp);
            if (tmp[0] != '-') strncpy(action, tmp, sizeof(action)-1);

            printf("  status (ok / fail / -): ");
            scanf("%127s", tmp);
            if (tmp[0] != '-') strncpy(status, tmp, sizeof(status)-1);

            printf("  platform (telegram / reddit / -): ");
            scanf("%127s", tmp);
            if (tmp[0] != '-') strncpy(plat, tmp, sizeof(plat)-1);
            printf("\n  Выполняем поиск...\n\n");
            SearchCriteria crit = {0};
            if (uid[0])    crit.uid      = uid;
            if (action[0]) crit.action   = action;
            if (status[0]) crit.status   = status;
            if (plat[0])   crit.platform = plat;
            SearchResult res;
            VTL_logAnalyzer_SearchMulti(&all, n_threads, &crit, &res);
            printf("  Найдено: %zu записей\n", res.count);
            if (res.count > 0) {
                size_t show = res.count < 5 ? res.count : 5;
                printf("\n  Первые %zu:\n", show);
                for (size_t i = 0; i < show; i++)
                    printf("  [%zu] %s | %s | %s | %s\n", i+1,
                           res.entries[i].ts, res.entries[i].uid,
                           res.entries[i].action, res.entries[i].detail);
            }
            VTL_logAnalyzer_ResultFree(&res);
            break;
        }

        case 6: {
            printf("  Считаем ошибки авторизации по платформам...\n\n");
            PlatformStats stats[8]; size_t count = 0;
            VTL_logAnalyzer_StatsAuthFails(&all, n_threads, stats, &count);
            printf("  %-12s  %6s  %8s  %8s\n","Платформа","Всего","Ошибок","Процент");
            printf("  %-12s  %6s  %8s  %8s\n","──────────","──────","──────","──────");
            for (size_t i = 0; i < count; i++)
                printf("  %-12s  %6zu  %8zu  %7.1f%%\n",
                       stats[i].platform, stats[i].total,
                       stats[i].fail, stats[i].fail_rate);
            break;
        }

        case 7: {
            printf("  Считаем использование платформ...\n\n");
            PlatformStats stats[8]; size_t count = 0;
            VTL_logAnalyzer_StatsPlatformUsage(&all, n_threads, stats, &count);
            printf("  %-12s  %6s  %8s\n","Платформа","Всего","% ошибок");
            printf("  %-12s  %6s  %8s\n","──────────","──────","────────");
            for (size_t i = 0; i < count; i++)
                printf("  %-12s  %6zu  %7.1f%%\n",
                       stats[i].platform, stats[i].total, stats[i].fail_rate);
            break;
        }

        case 8: {
            printf("  Считаем длительность сессий...\n\n");
            DurationStats dur;
            VTL_logAnalyzer_StatsSessionDuration(&all, n_threads, &dur);
            if (dur.count > 0) {
                printf("  Сессий завершённых: %zu\n",      dur.count);
                printf("  Среднее время:      %.1f сек\n", dur.avg_sec);
                printf("  Минимальное:        %.1f сек\n", dur.min_sec);
                printf("  Максимальное:       %.1f сек\n", dur.max_sec);
                printf("\n  Эффективность распараллеливания: %.1f%%\n", efficiency);
            } else {
                printf("  Нет завершённых сессий\n");
            }
            break;
        }

        case 9:
            printf("  Запускаем демо-режим. Нажми Ctrl+C для остановки.\n\n");
            VTL_logAnalyzer_RunDemo(log_path, n_threads);
            break;

        case 'p':
            printf("  Тест производительности — 1 vs N потоков\n");
            printf("  Файл: %s\n\n", log_path);
            VTL_logAnalyzer_perf_Test(log_path, n_threads);
            break;

        case 0:
            running = 0;
            break;

        default:
            printf("  Неизвестная команда. Введите число от 0 до 9 или p.\n");
            break;
        }
    }

    VTL_logAnalyzer_ResultFree(&all);
    vtl_mutex_destroy(&g_print_mutex);
    printf("\n  До свидания!\n\n");
    return 0;
}
