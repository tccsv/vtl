//#define _POSIX_C_SOURCE 200809L
//
//#include <VTL/VTL.h>
//#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <time.h>
//
//#ifdef _WIN32
//#include <windows.h>
//#else
//#include <unistd.h>
//#endif
//
//
///* Сколько физических потоков CPU доступно. Эффективность параллелизма
// * правильно считать относительно числа ядер, а не числа созданных потоков —
// * больше потоков чем ядер не дают дополнительного ускорения.
// * На Linux/Mac используем POSIX sysconf, на Windows — GetSystemInfo. */
//static long detect_cpu_cores(void)
//{
//#ifdef _WIN32
//    SYSTEM_INFO si;
//    GetSystemInfo(&si);
//    return (long)si.dwNumberOfProcessors;
//#else
//    long n = sysconf(_SC_NPROCESSORS_ONLN);
//    return (n > 0) ? n : 1;
//#endif
//}
//
//
///*
// * Генерация большого AsciiDoc документа в памяти. Это нужно чтобы получить
// * реальный speedup от распараллеливания: на маленьком тексте overhead на
// * создание потоков перекрывает полезную работу.
// */
//static char* build_large_asciidoc(size_t target_kb, size_t* out_len)
//{
//    /* Большой кусок текста с разными типами разметки.
//     * Чем больше типов в одном фрагменте, тем больше работы для каждого сканера. */
//    const char* fragment =
//        ":author: VTL Team\n"
//        ":revdate: 2026-05-12\n\n"
//        "= Section Heading Level One\n"
//        "== Section Heading Level Two\n\n"
//        "Paragraph with *bold*, _italic_, `mono`, ~sub~ and ^sup^ inline plus "
//        "[line-through]#strikethrough# and a link:https://example.com[example] "
//        "ссылка плюс cross-reference <<intro,intro>> в одном предложении.\n\n"
//        "NOTE: This is an admonition paragraph with *bold inside*.\n"
//        "WARNING: Another admonition with _italic_ markup inside it.\n\n"
//        "* First bullet with *strong* inside\n"
//        "* Second bullet with _emphasis_ inside\n"
//        "** Nested bullet level two with `mono`\n"
//        ". Numbered item one\n"
//        ". Numbered item two\n\n"
//        "[source,c]\n"
//        "----\n"
//        "int main(void) { return 0; }\n"
//        "----\n\n"
//        "____\n"
//        "A quoted block with *bold* and _italic_ inside the quotes.\n"
//        "____\n\n"
//        "// A comment line that should be ignored by the converter.\n\n"
//        "Final paragraph with all kinds of markers: *b* _i_ `m` ~s~ ^p^ "
//        "[line-through]#strike# link:url[txt] <<ref>> mixed together.\n\n";
//
//    size_t frag_len = strlen(fragment);
//    size_t target = target_kb * 1024;
//    char* buf = (char*)malloc(target + frag_len + 1);
//    if (!buf) return NULL;
//
//    size_t pos = 0;
//    while (pos < target) {
//        memcpy(buf + pos, fragment, frag_len);
//        pos += frag_len;
//    }
//    buf[pos] = '\0';
//    *out_len = pos;
//    return buf;
//}
//
//
//static void demo_asciidoc_parser(void)
//{
//    const char* sample =
//        ":author: Demo\n\n"
//        "= Demo Title\n\n"
//        "Plain text with *bold*, _italic_, `mono`, ~sub~, ^sup^, "
//        "[line-through]#strike# and link:url[link] markers.\n"
//        "NOTE: admonition example.\n";
//    VTL_publication_Text src = { (char*)sample, strlen(sample) };
//
//    VTL_publication_MarkedText* marked = NULL;
//    VTL_AppResult res = VTL_asciidoc_ParseTextParallel(&src, &marked);
//    if (res != VTL_res_kOk || !marked) {
//        printf("AsciiDoc demo parse failed: %d\n", (int)res);
//        return;
//    }
//
//    printf("=== AsciiDoc Parser Demo (in-memory) ===\n");
//    printf("Source bytes: %zu, parsed into %zu parts\n",
//           src.length, marked->length);
//    for (size_t i = 0; i < marked->length; ++i) {
//        const VTL_publication_marked_text_Part* p = &marked->parts[i];
//        const char* label = "regular";
//        if (p->type & VTL_TEXT_MODIFICATION_BOLD)          label = "BOLD";
//        else if (p->type & VTL_TEXT_MODIFICATION_ITALIC)   label = "ITALIC";
//        else if (p->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) label = "STRIKE";
//
//        printf("  [%zu] %-8s len=%2zu \"", i, label, p->length);
//        size_t to_print = p->length > 50 ? 50 : p->length;
//        for (size_t k = 0; k < to_print; ++k) {
//            char c = p->text[k];
//            putchar((c == '\n') ? ' ' : c);
//        }
//        if (p->length > to_print) printf("...");
//        printf("\"\n");
//    }
//
//    free(marked->parts);
//    free(marked);
//}
//
//
//static void bench_asciidoc_scanners(size_t doc_size_kb, size_t iterations)
//{
//    size_t src_len = 0;
//    char* buf = build_large_asciidoc(doc_size_kb, &src_len);
//    if (!buf) {
//        printf("bench: allocation failed\n");
//        return;
//    }
//    VTL_publication_Text src = { buf, src_len };
//
//    long cpu_cores = detect_cpu_cores();
//    printf("\n=== Scanner-Level Parallelism (15 scanners, %ld CPU cores, %zu KB doc, %zu iters) ===\n",
//           cpu_cores, doc_size_kb, iterations);
//
//    double t_seq = VTL_asciidoc_BenchSequential(&src, iterations);
//    double t_par = VTL_asciidoc_BenchParallel(&src, iterations);
//    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;
//    double efficiency = speedup / (double)cpu_cores;
//
//    printf("  Sequential : %.4f s\n", t_seq);
//    printf("  Parallel   : %.4f s\n", t_par);
//    printf("  Speedup    : %.3fx\n", speedup);
//    printf("  Efficiency : %.1f%%  (Sp/N where N=%ld cores)\n",
//           efficiency * 100.0, cpu_cores);
//
//    free(buf);
//}
//
//
//static void bench_asciidoc_batch(size_t doc_size_kb, size_t files, size_t iterations)
//{
//    size_t src_len = 0;
//    char* buf = build_large_asciidoc(doc_size_kb, &src_len);
//    if (!buf) {
//        return;
//    }
//
//    /* Записываем во временный файл — batch API работает по путям */
//    const char* path = "_bench_input.adoc";
//    FILE* f = fopen(path, "wb");
//    if (!f) { free(buf); return; }
//    fwrite(buf, 1, src_len, f);
//    fclose(f);
//    free(buf);
//
//    const char** paths = (const char**)malloc(files * sizeof(char*));
//    VTL_publication_MarkedText** out =
//        (VTL_publication_MarkedText**)calloc(files, sizeof(*out));
//    if (!paths || !out) { free(paths); free(out); remove(path); return; }
//    for (size_t i = 0; i < files; ++i) paths[i] = path;
//
//    printf("\n=== File-Batch Parallelism (%zu files x %zu KB each, %zu iters) ===\n",
//           files, doc_size_kb, iterations);
//
//    struct timespec ts;
//    clock_gettime(CLOCK_MONOTONIC, &ts);
//    double t0 = (double)ts.tv_sec + ts.tv_nsec * 1e-9;
//    for (size_t k = 0; k < iterations; ++k) {
//        VTL_asciidoc_ParseFileBatchSequential(paths, files, out);
//        for (size_t i = 0; i < files; ++i) {
//            if (out[i]) { free(out[i]->parts); free(out[i]); out[i] = NULL; }
//        }
//    }
//    clock_gettime(CLOCK_MONOTONIC, &ts);
//    double t_seq = ((double)ts.tv_sec + ts.tv_nsec * 1e-9) - t0;
//
//    clock_gettime(CLOCK_MONOTONIC, &ts);
//    t0 = (double)ts.tv_sec + ts.tv_nsec * 1e-9;
//    for (size_t k = 0; k < iterations; ++k) {
//        VTL_asciidoc_ParseFileBatchParallel(paths, files, out);
//        for (size_t i = 0; i < files; ++i) {
//            if (out[i]) { free(out[i]->parts); free(out[i]); out[i] = NULL; }
//        }
//    }
//    clock_gettime(CLOCK_MONOTONIC, &ts);
//    double t_par = ((double)ts.tv_sec + ts.tv_nsec * 1e-9) - t0;
//
//    long cpu_cores = detect_cpu_cores();
//    /* Эффективный N — минимум из числа потоков и числа ядер CPU.
//     * Больше потоков чем ядер не дают физического ускорения. */
//    double effective_n = (double)((files < (size_t)cpu_cores) ? files : (size_t)cpu_cores);
//    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;
//    double efficiency = speedup / effective_n;
//
//    printf("  Sequential : %.4f s\n", t_seq);
//    printf("  Parallel   : %.4f s\n", t_par);
//    printf("  Speedup    : %.3fx\n", speedup);
//    printf("  Efficiency : %.1f%%  (Sp/N where N=%.0f effective workers)\n",
//           efficiency * 100.0, effective_n);
//
//    free(out);
//    free(paths);
//    remove(path);
//}
//
//
//int main(void)
//{
//    srand((unsigned)time(NULL));
//
//    demo_asciidoc_parser();
//    bench_asciidoc_scanners(512, 50);
//    bench_asciidoc_batch(128, 8, 20);
//
//    /* --- Существующий бизнес-пайплайн (без изменений) --- */
//    const char* audio_files[] = {
//        "audio_ariel.mp3",
//        "audio_styuardessa.mp3",
//        "audio_xanadu.mp3"
//    };
//    int pick = rand() % 3;
//
//    printf("\n=== Text Pipeline ===\n");
//    VTL_AppResult res_text = VTL_PubicateMarkedText("text.md",
//        VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG,
//        VTL_markup_type_kTelegramMD);
//    printf("Text: %d (%s)\n\n", res_text,
//        res_text == VTL_res_kOk ? "OK" : "ERROR");
//
//    printf("=== Audio Pipeline [%d]: %s ===\n", pick, audio_files[pick]);
//    VTL_AppResult res_audio = VTL_PubicateAudioWithMarkedText(
//        audio_files[pick], "text.md",
//        VTL_markup_type_kTelegramMD,
//        VTL_CONTENT_PLATFORM_W | VTL_CONTENT_PLATFORM_TG);
//    printf("Audio: %d (%s)\n", res_audio,
//        res_audio == VTL_res_kOk ? "OK" : "ERROR");
//
//    return (res_text != VTL_res_kOk) ? res_text : res_audio;
//}

#include <VTL/media_container/img/VTL_img_core.h>
#include <VTL/media_container/img/VTL_img_filters.h>
#include <VTL/media_container/img/VTL_img_utils.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("Current working dir: %s\n", cwd);
    const char* input_path  = "./test_images/test2-1.jpg";
    const char* output_path = "./test_images/test2-1_output.jpg";

    /* 1. Проверка файла */
    if (!VTL_img_CheckFileExists(input_path)) {
        printf("File not found: %s\n", input_path);
        return 1;
    }

    if (!VTL_img_IsFormatSupported(input_path)) {
        printf("Unsupported format: %s\n", input_path);
        return 1;
    }

    printf("Input format: %s\n",
           VTL_img_GetFormatDescription(input_path));

    /* 2. Создаем контекст */
    VTL_ImageContext* ctx = VTL_img_context_Init();
    if (!ctx) {
        printf("Failed to init context\n");
        return 1;
    }

    /* 3. Загружаем изображение */
    if (VTL_img_LoadImage(input_path, ctx) != 0) {
        printf("Failed to load image\n");
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    printf("Image loaded: %dx%d\n",
           ctx->current_frame->width,
           ctx->current_frame->height);

    /* 4. Выбираем фильтр */
    const VTL_ImageFilter* filter = &VTL_img_filter_grayscale;


    printf("Applying filter: %s\n", filter->name);

    /* 5. Применяем фильтр */
    if (VTL_img_ApplyFilter(ctx, filter) != 0) {
        printf("Failed to apply filter\n");
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    /* 6. Сохраняем */
    if (VTL_img_SaveImage(output_path, ctx) != 0) {
        printf("Failed to save image\n");
        VTL_img_context_Cleanup(ctx);
        return 1;
    }

    printf("Saved to: %s\n", output_path);

    /* 7. Очистка */
    VTL_img_context_Cleanup(ctx);

    printf("Parallel example\n");

    /* 1. Подготовка списков файлов и фильтров */
    const char* input_paths[] = {
            "./test_images/2-1.jpg",
            "./test_images/2-2.jpg",
            "./test_images/2-3.jpg"
    };

    const char* output_paths[] = {
            "./test_images/2-1_out.png",
            "./test_images/2-2_out.png",
            "./test_images/2-3_out.png"
    };

    /* Разные фильтры для каждой картинки */
    const VTL_ImageFilter* filters[] = {
            &VTL_img_filter_grayscale,      // Для первой - ч/б
            &VTL_img_filter_sepia,          // Для второй - сепия
            &VTL_img_filter_gaussian_blur   // Для третьей - размытие
    };

    int count = 3;

    /* 2. Запуск пакетной обработки */
    printf("Starting parallel processing of %d images...\n", count);

    // Функция сама создаст потоки, обработает изображения и дождется завершения
    VTL_AppResult res = VTL_img_ProcessBatch(input_paths, filters, output_paths, count);

    /* 3. Проверка результата */
    if (res == VTL_res_kOk) {
        printf("Batch processing completed successfully.\n");
    } else {
        printf("Batch processing encountered errors (see stderr for details).\n");
    }

    printf("Done.\n");
    return 0;
}