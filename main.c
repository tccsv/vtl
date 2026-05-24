#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/discord/VTL_publication_text_op_discord.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>

#endif


static long detect_cpu_cores(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (long)si.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? n : 1;
#endif
}


/*
 * Генерация большого Discord MD документа в памяти.
 * На маленьком тексте overhead на создание потоков перекрывает полезную работу,
 * поэтому для честного бенчмарка нужен документ от нескольких сотен KB.
 */
static char *build_large_discord(size_t target_kb, size_t *out_len) {
    const char *fragment =
            "**Заголовок раздела** — важная часть документа.\n\n"
            "Параграф с **жирным**, *курсивом*, ~~зачёркнутым~~ и "
            "***жирным курсивом*** в одном предложении.\n\n"
            "Обычный текст без разметки для баланса нагрузки между сканерами.\n"
            "snake_case_variable и another_variable не должны триггерить курсив.\n\n"
            "**Второй жирный блок** с *курсивом внутри* и ~~зачёркнутым~~ следом.\n"
            "*Курсивный параграф* с **жирным словом** и ~~strike~~ в конце.\n\n"
            "***Весь этот текст жирный курсив*** — проверка трёх звёздочек.\n"
            "~~Весь этот текст зачёркнут~~ — проверка тильд.\n\n";

    size_t frag_len = strlen(fragment);
    size_t target = target_kb * 1024;
    char *buf = (char *) malloc(target + frag_len + 1);
    if (!buf) return NULL;

    size_t pos = 0;
    while (pos < target) {
        memcpy(buf + pos, fragment, frag_len);
        pos += frag_len;
    }
    buf[pos] = '\0';
    *out_len = pos;
    return buf;
}


/* =========================================================================
 * Демо: показывает разбор нескольких строк и roundtrip Discord MD → части → MD
 * ========================================================================= */
static void demo_discord_parser(void) {
    struct {
        const char *input;
        const char *label;
    } cases[] = {
            {"**жирный**",              "bold"},
            {"*курсив*",                "italic *"},
            {"_курсив_",                "italic _"},
            {"~~зачёркнутый~~",         "strike"},
            {"***жирный курсив***",     "bold+italic"},
            {"hello **world** bye",     "mixed"},
            {"**b** and *i* and ~~s~~", "all three"},
            {"my_variable_name",        "snake_case"},
    };
    size_t n = sizeof(cases) / sizeof(cases[0]);

    printf("=== Демо Discord MD-парсера ===\n");

    for (size_t i = 0; i < n; ++i) {
        VTL_publication_Text src = {
                (VTL_publication_text_Symbol *) cases[i].input,
                strlen(cases[i].input)
        };

        VTL_publication_MarkedText *marked = NULL;
        if (VTL_discord_ParseTextParallel(&src, &marked) != VTL_res_kOk || !marked) {
            printf("  [%-12s] FAIL\n", cases[i].label);
            continue;
        }

        VTL_publication_Text *out = NULL;
        VTL_discord_SerializeToText(marked, &out);

        printf("  [%-12s] parts=%-2zu  \"%s\" -> \"%s\"\n",
               cases[i].label, marked->length, cases[i].input,
               out ? out->text : "?");

        if (out) {
            free(out->text);
            free(out);
        }
        free(marked->parts);
        free(marked);
    }
}


/* =========================================================================
 * Бенчмарк: последовательный vs параллельный
 *
 * Каждый сканер ищет свой тип разметки независимо.
 * Sequential — все 5 сканеров в одном потоке по очереди.
 * Parallel   — каждый сканер в своём потоке, у каждого свой буфер → нет локов.
 * ========================================================================= */
static void bench_discord_scanners(size_t doc_size_kb, size_t iterations) {
    size_t src_len = 0;
    char *buf = build_large_discord(doc_size_kb, &src_len);
    if (!buf) {
        printf("bench: не удалось выделить память\n");
        return;
    }

    VTL_publication_Text src = {buf, src_len};
    long cpu_cores = detect_cpu_cores();

    printf("\n=== Discord MD: последовательный vs параллельный"
           " (%zu KB, %zu итераций, %ld ядер) ===\n",
           doc_size_kb, iterations, cpu_cores);

    /* Sequential */
    clock_t c0 = clock();
    for (size_t k = 0; k < iterations; ++k) {
        VTL_publication_MarkedText *out = NULL;
        VTL_discord_ParseTextSequential(&src, &out);
        if (out) {
            free(out->parts);
            free(out);
        }
    }
    double t_seq = (double) (clock() - c0) / CLOCKS_PER_SEC;

    /* Parallel */
    c0 = clock();
    for (size_t k = 0; k < iterations; ++k) {
        VTL_publication_MarkedText *out = NULL;
        VTL_discord_ParseTextParallel(&src, &out);
        if (out) {
            free(out->parts);
            free(out);
        }
    }
    double t_par = (double) (clock() - c0) / CLOCKS_PER_SEC;

    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;
    double efficiency = speedup / (double) cpu_cores;

    printf("  Последовательно : %.4f с\n", t_seq);
    printf("  Параллельно     : %.4f с\n", t_par);
    printf("  Ускорение       : %.3fx\n", speedup);
    printf("  Эффективность   : %.1f%%  (Sp/N, N=%ld ядер)\n",
           efficiency * 100.0, cpu_cores);

    free(buf);
}


/* =========================================================================
 * Демо: внутренний формат → Discord MD
 *
 * Создаём MarkedText вручную с разными флагами и сериализуем в строку.
 * Показывает что SerializeToText работает независимо от парсера —
 * внутренний формат может быть получен из любого источника (БД, другой парсер).
 * ========================================================================= */
static void demo_discord_serialize(void) {
    printf("\n=== Демо: внутренний формат → Discord MD ===\n");

    /* Строим MarkedText вручную:
     *   "Привет, " (plain) + "мир" (BOLD) + "! Это " (plain) +
     *   "курсив" (ITALIC) + " и " (plain) + "зачёркнуто" (STRIKE) + "." (plain) */
    /* Длины в байтах — UTF-8: кириллический символ занимает 2 байта */
    static const char s0[] = "Привет, ";
    static const char s1[] = "мир";
    static const char s2[] = "! Это ";
    static const char s3[] = "курсив";
    static const char s4[] = " и ";
    static const char s5[] = "зачёркнуто";
    static const char s6[] = ".";
    VTL_publication_marked_text_Part parts[] = {
            {s0, sizeof(s0) - 1, 0},
            {s1, sizeof(s1) - 1, VTL_TEXT_MODIFICATION_BOLD},
            {s2, sizeof(s2) - 1, 0},
            {s3, sizeof(s3) - 1, VTL_TEXT_MODIFICATION_ITALIC},
            {s4, sizeof(s4) - 1, 0},
            {s5, sizeof(s5) - 1, VTL_TEXT_MODIFICATION_STRIKETHROUGH},
            {s6, sizeof(s6) - 1, 0},
    };

    VTL_publication_MarkedText marked;
    marked.parts = parts;
    marked.length = sizeof(parts) / sizeof(parts[0]);

    /* Выводим части */
    printf("  Внутренний формат (%zu частей):\n", marked.length);
    for (size_t i = 0; i < marked.length; ++i) {
        const VTL_publication_marked_text_Part *p = &marked.parts[i];
        const char *flag = "plain";
        if (p->type & VTL_TEXT_MODIFICATION_BOLD) flag = "BOLD";
        else if (p->type & VTL_TEXT_MODIFICATION_ITALIC) flag = "ITALIC";
        else if (p->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) flag = "STRIKE";
        printf("    [%zu] %-6s \"", i, flag);
        fwrite(p->text, 1, p->length, stdout);
        printf("\"\n");
    }

    /* Сериализуем в Discord MD */
    VTL_publication_Text *out = NULL;
    VTL_AppResult res = VTL_discord_SerializeToText(&marked, &out);
    if (res != VTL_res_kOk || !out) {
        printf("  FAIL: serialize error %d\n", (int) res);
        return;
    }

    printf("  Discord MD: \"%s\"\n", out->text);

    /* Для проверки — парсим обратно */
    VTL_publication_MarkedText *reparsed = NULL;
    VTL_discord_ParseTextParallel(out, &reparsed);
    if (reparsed) {
        printf("  Roundtrip  : %zu частей (было %zu)\n",
               reparsed->length, marked.length);
        free(reparsed->parts);
        free(reparsed);
    }

    free(out->text);
    free(out);
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    srand((unsigned) time(NULL));

    demo_discord_parser();
    demo_discord_serialize();
    bench_discord_scanners(512, 50);
    return 0;
}
