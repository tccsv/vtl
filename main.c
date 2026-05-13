#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#ifndef VTL_MINIMAL_BUILD
#include <VTL/VTL.h>
#endif
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


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


static char* build_large_asciidoc(size_t target_kb, size_t* out_len)
{

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
        printf("AsciiDoc demo parse failed: %d\n", (int)res);
        return;
    }

    printf("=== AsciiDoc Parser Demo (in-memory) ===\n");
    printf("Source bytes: %zu, parsed into %zu parts\n",
           src.length, marked->length);
    for (size_t i = 0; i < marked->length; ++i) {
        const VTL_publication_marked_text_Part* p = &marked->parts[i];
        const char* label = "regular";
        if (p->type & VTL_TEXT_MODIFICATION_BOLD)          label = "BOLD";
        else if (p->type & VTL_TEXT_MODIFICATION_ITALIC)   label = "ITALIC";
        else if (p->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) label = "STRIKE";

        printf("  [%zu] %-8s len=%2zu \"", i, label, p->length);
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
        printf("bench: allocation failed\n");
        return;
    }
    VTL_publication_Text src = { buf, src_len };

    long cpu_cores = detect_cpu_cores();
    printf("\n=== Scanner-Level Parallelism (15 scanners, %ld CPU cores, %zu KB doc, %zu iters) ===\n",
           cpu_cores, doc_size_kb, iterations);

    double t_seq = VTL_asciidoc_BenchSequential(&src, iterations);
    double t_par = VTL_asciidoc_BenchParallel(&src, iterations);
    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;
    double efficiency = speedup / (double)cpu_cores;

    printf("  Sequential : %.4f s\n", t_seq);
    printf("  Parallel   : %.4f s\n", t_par);
    printf("  Speedup    : %.3fx\n", speedup);
    printf("  Efficiency : %.1f%%  (Sp/N where N=%ld cores)\n",
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

    printf("\n=== File-Batch Parallelism (%zu files x %zu KB each, %zu iters) ===\n",
           files, doc_size_kb, iterations);

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t0 = (double)ts.tv_sec + ts.tv_nsec * 1e-9;
    for (size_t k = 0; k < iterations; ++k) {
        VTL_asciidoc_ParseFileBatchSequential(paths, files, out);
        for (size_t i = 0; i < files; ++i) {
            if (out[i]) { free(out[i]->parts); free(out[i]); out[i] = NULL; }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t_seq = ((double)ts.tv_sec + ts.tv_nsec * 1e-9) - t0;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    t0 = (double)ts.tv_sec + ts.tv_nsec * 1e-9;
    for (size_t k = 0; k < iterations; ++k) {
        VTL_asciidoc_ParseFileBatchParallel(paths, files, out);
        for (size_t i = 0; i < files; ++i) {
            if (out[i]) { free(out[i]->parts); free(out[i]); out[i] = NULL; }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t_par = ((double)ts.tv_sec + ts.tv_nsec * 1e-9) - t0;

    long cpu_cores = detect_cpu_cores();

    double effective_n = (double)((files < (size_t)cpu_cores) ? files : (size_t)cpu_cores);
    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;
    double efficiency = speedup / effective_n;

    printf("  Sequential : %.4f s\n", t_seq);
    printf("  Parallel   : %.4f s\n", t_par);
    printf("  Speedup    : %.3fx\n", speedup);
    printf("  Efficiency : %.1f%%  (Sp/N where N=%.0f effective workers)\n",
           efficiency * 100.0, effective_n);

    free(out);
    free(paths);
    remove(path);
}


int main(int argc, char** argv)
{
    srand((unsigned)time(NULL));


    int use_mediawiki = 0;
    const char* text_path = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--mediawiki") == 0 || strcmp(argv[i], "--wiki") == 0) {
            use_mediawiki = 1;
        } else if (argv[i][0] != '-') {
            text_path = argv[i];
        }
    }
    if (!text_path) {
        text_path = use_mediawiki ? "text.wiki" : "text.md";
    }

    demo_asciidoc_parser();
    bench_asciidoc_scanners(512, 50);
    bench_asciidoc_batch(128, 8, 20);

#ifdef VTL_MINIMAL_BUILD

    (void)use_mediawiki; (void)text_path;
    printf("\n=== Minimal build — pipelines disabled ===\n");
    return 0;
#else
    VTL_publication_marked_text_MarkupType markup =
        use_mediawiki ? VTL_markup_type_kMediaWiki : VTL_markup_type_kTelegramMD;


    const char* audio_files[] = {
        "audio_ariel.mp3",
        "audio_styuardessa.mp3",
        "audio_xanadu.mp3"
    };
    int pick = rand() % 3;

    printf("\n=== Text Pipeline (%s, %s) ===\n",
           use_mediawiki ? "MediaWiki" : "TelegramMD", text_path);
    VTL_AppResult res_text = VTL_PubicateMarkedText(text_path,
        VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG,
        markup);
    printf("Text: %d (%s)\n\n", res_text,
        res_text == VTL_res_kOk ? "OK" : "ERROR");


    if (use_mediawiki) {
        printf("Audio: skipped (--mediawiki mode)\n");
        return res_text;
    }

    printf("=== Audio Pipeline [%d]: %s ===\n", pick, audio_files[pick]);
    VTL_AppResult res_audio = VTL_PubicateAudioWithMarkedText(
        audio_files[pick], text_path,
        markup,
        VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG);
    printf("Audio: %d (%s)\n", res_audio,
        res_audio == VTL_res_kOk ? "OK" : "ERROR");

    return (res_text != VTL_res_kOk) ? res_text : res_audio;
#endif
}
