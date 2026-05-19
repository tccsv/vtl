#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/VTL.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_compat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


/* Сколько физических потоков CPU доступно. Эффективность параллелизма
 * правильно считать относительно числа ядер, а не числа созданных потоков —
 * больше потоков чем ядер не дают дополнительного ускорения.
 * На Linux/Mac используем POSIX sysconf, на Windows — GetSystemInfo. */
static long detect_cpu_cores(void)
{
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
 * Генерация большого AsciiDoc документа в памяти. Это нужно чтобы получить
 * реальный speedup от распараллеливания: на маленьком тексте overhead на
 * создание потоков перекрывает полезную работу.
 */
static char* build_large_asciidoc(size_t target_kb, size_t* out_len)
{
    /* Большой кусок текста с разными типами разметки.
     * Чем больше типов в одном фрагменте, тем больше работы для каждого сканера. */
    const char* fragment =
            ":author: VTL Team\n"
            ":revdate: 2026-05-12\n\n"
            "= Section Heading Level One\n"
            "== Section Heading Level Two\n\n"
            "Paragraph with *bold*, _italic_, `mono`, ~sub~ and ^sup^ inline plus "
            "[line-through]#strikethrough# and a link:https://example.com[example] "
            "ссылка плюс cross-reference <<intro,intro>> в одном предложении.\n\n"
            "NOTE: This is an admonition paragraph with *bold inside*.\n"
            "WARNING: Another admonition with _italic_ markup inside it.\n\n"
            "* First bullet with *strong* inside\n"
            "* Second bullet with _emphasis_ inside\n"
            "** Nested bullet level two with `mono`\n"
            ". Numbered item one\n"
            ". Numbered item two\n\n"
            "[source,c]\n"
            "----\n"
            "int main(void) { return 0; }\n"
            "----\n\n"
            "____\n"
            "A quoted block with *bold* and _italic_ inside the quotes.\n"
            "____\n\n"
            "// A comment line that should be ignored by the converter.\n\n"
            "Final paragraph with all kinds of markers: *b* _i_ `m` ~s~ ^p^ "
            "[line-through]#strike# link:url[txt] <<ref>> mixed together.\n\n";

    size_t frag_len = strlen(fragment);
    size_t target = target_kb * 1024;
    char* buf = (char*)malloc(target + frag_len + 1);
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


static void demo_asciidoc_parser(void)
{
    const char* sample =
            ":author: Demo\n\n"
            "= Demo Title\n\n"
            "Plain text with *bold*, _italic_, `mono`, ~sub~, ^sup^, "
            "[line-through]#strike# and link:url[link] markers.\n"
            "NOTE: admonition example.\n";
    VTL_publication_Text src = { (char*)sample, strlen(sample) };

    VTL_publication_MarkedText* marked = NULL;
    VTL_AppResult res = VTL_asciidoc_ParseTextParallel(&src, &marked);
    if (res != VTL_res_kOk || !marked) {
        printf("AsciiDoc демо: ошибка парсинга: %d\n", (int)res);
        return;
    }

    printf("=== Демо AsciiDoc-парсера (в памяти) ===\n");
    printf("Байт исходника: %zu, разобрано на %zu частей\n",
           src.length, marked->length);
    for (size_t i = 0; i < marked->length; ++i) {
        const VTL_publication_marked_text_Part* p = &marked->parts[i];
        const char* label = "обычный";
        if (p->type & VTL_TEXT_MODIFICATION_BOLD)          label = "ЖИРНЫЙ";
        else if (p->type & VTL_TEXT_MODIFICATION_ITALIC)   label = "КУРСИВ";
        else if (p->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) label = "ЗАЧЁРКНУТ";

        printf("  [%zu] %-10s длина=%2zu \"", i, label, p->length);
        size_t to_print = p->length > 50 ? 50 : p->length;
        for (size_t k = 0; k < to_print; ++k) {
            char c = p->text[k];
            putchar((c == '\n') ? ' ' : c);
        }
        if (p->length > to_print) printf("...");
        printf("\"\n");
    }

    free(marked->parts);
    free(marked);
}


static void bench_asciidoc_scanners(size_t doc_size_kb, size_t iterations)
{
    size_t src_len = 0;
    char* buf = build_large_asciidoc(doc_size_kb, &src_len);
    if (!buf) {
        printf("bench: не удалось выделить память\n");
        return;
    }
    VTL_publication_Text src = { buf, src_len };

    long cpu_cores = detect_cpu_cores();
    printf("\n=== Параллелизм на уровне сканеров (15 сканеров, %ld ядер CPU, документ %zu KB, %zu итераций) ===\n",
           cpu_cores, doc_size_kb, iterations);

    double t_seq = VTL_asciidoc_BenchSequential(&src, iterations);
    double t_par = VTL_asciidoc_BenchParallel(&src, iterations);
    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;
    double efficiency = speedup / (double)cpu_cores;

    printf("  Последовательно : %.4f с\n", t_seq);
    printf("  Параллельно     : %.4f с\n", t_par);
    printf("  Ускорение       : %.3fx\n", speedup);
    printf("  Эффективность   : %.1f%%  (Sp/N, где N=%ld ядер)\n",
           efficiency * 100.0, cpu_cores);

    free(buf);
}


static void bench_asciidoc_batch(size_t doc_size_kb, size_t files, size_t iterations)
{
    size_t src_len = 0;
    char* buf = build_large_asciidoc(doc_size_kb, &src_len);
    if (!buf) {
        return;
    }

    /* Записываем во временный файл — batch API работает по путям */
    const char* path = "_bench_input.adoc";
    FILE* f = fopen(path, "wb");
    if (!f) { free(buf); return; }
    fwrite(buf, 1, src_len, f);
    fclose(f);
    free(buf);

    const char** paths = (const char**)malloc(files * sizeof(char*));
    VTL_publication_MarkedText** out =
            (VTL_publication_MarkedText**)calloc(files, sizeof(*out));
    if (!paths || !out) { free(paths); free(out); remove(path); return; }
    for (size_t i = 0; i < files; ++i) paths[i] = path;

    printf("\n=== Параллелизм пакета файлов (%zu файлов по %zu KB, %zu итераций) ===\n",
           files, doc_size_kb, iterations);

    double t0 = vtl_monotonic_seconds();
    for (size_t k = 0; k < iterations; ++k) {
        VTL_asciidoc_ParseFileBatchSequential(paths, files, out);
        for (size_t i = 0; i < files; ++i) {
            if (out[i]) { free(out[i]->parts); free(out[i]); out[i] = NULL; }
        }
    }
    double t_seq = vtl_monotonic_seconds() - t0;

    t0 = vtl_monotonic_seconds();
    for (size_t k = 0; k < iterations; ++k) {
        VTL_asciidoc_ParseFileBatchParallel(paths, files, out);
        for (size_t i = 0; i < files; ++i) {
            if (out[i]) { free(out[i]->parts); free(out[i]); out[i] = NULL; }
        }
    }
    double t_par = vtl_monotonic_seconds() - t0;

    long cpu_cores = detect_cpu_cores();
    /* Эффективный N — минимум из числа потоков и числа ядер CPU.
     * Больше потоков чем ядер не дают физического ускорения. */
    double effective_n = (double)((files < (size_t)cpu_cores) ? files : (size_t)cpu_cores);
    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;
    double efficiency = speedup / effective_n;

    printf("  Последовательно : %.4f с\n", t_seq);
    printf("  Параллельно     : %.4f с\n", t_par);
    printf("  Ускорение       : %.3fx\n", speedup);
    printf("  Эффективность   : %.1f%%  (Sp/N, где N=%.0f эффективных воркеров)\n",
           efficiency * 100.0, effective_n);

    free(out);
    free(paths);
    remove(path);
}


int main(void)
{
#ifdef _WIN32
    /* Кириллица в printf — UTF-8 в исходниках. Дефолтная codepage консоли
     * (cmd/PowerShell) — OEM (cp866 в RU-локали), отсюда крякозябры.
     * Переключаем codepage процесса на UTF-8 — нативно для Windows Terminal
     * и работает в любом host'е, кто умеет UTF-8 (а это все актуальные). */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    srand((unsigned)time(NULL));

    demo_asciidoc_parser();
    bench_asciidoc_scanners(512, 50);
    bench_asciidoc_batch(128, 8, 20);

    const char* audio_files[] = {
            "audio_ariel.mp3",
            "audio_styuardessa.mp3",
            "audio_xanadu.mp3"
    };
    int pick = rand() % 3;

    printf("\n=== Text Pipeline ===\n");
    VTL_AppResult res_text = VTL_PubicateMarkedText("text.md",
                                                    VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG,
                                                    VTL_markup_type_kTelegramMD);
    printf("Text: %d (%s)\n\n", res_text,
           res_text == VTL_res_kOk ? "OK" : "ERROR");

    printf("=== Audio Pipeline [%d]: %s ===\n", pick, audio_files[pick]);
    VTL_AppResult res_audio = VTL_PubicateAudioWithMarkedText(
            audio_files[pick], "text.md",
            VTL_markup_type_kTelegramMD,
            VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG);
    printf("Audio: %d (%s)\n", res_audio,
           res_audio == VTL_res_kOk ? "OK" : "ERROR");

    return (res_text != VTL_res_kOk) ? res_text : res_audio;
}