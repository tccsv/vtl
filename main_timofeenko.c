/*
 * main_timofeenko.c — демонстрация моей части проекта VTL.
 *
 * Зона ответственности: AsciiDoc-парсер
 *   VTL/publication/text/asciidoc/  (модуль VTL_asciidoc)
 *
 * Что показывается:
 *   1. Парсинг inline-AsciiDoc-фрагмента (sequential + parallel)
 *      → MarkedText с флагами BOLD/ITALIC/STRIKETHROUGH.
 *   2. Парсинг файла text.adoc через VTL_asciidoc_ParseFile.
 *   3. Замер ускорения параллельного режима (15 сканеров в потоках)
 *      против sequential на документе 512 KB × 50 итераций.
 *   4. Batch-обработка нескольких файлов параллельно (поток на файл).
 *
 * Сборка (Linux/macOS/Windows-MSVC):
 *   cmake -S . -B build && cmake --build build -j
 *   ./app/main_timofeenko        # либо app/main_timofeenko.exe под Windows
 *
 * Никаких FFmpeg / libcurl / libpq не зовётся — VTL_asciidoc самодостаточен.
 */

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Печатает MarkedText: для каждой части показывает её тип
 * (обычный/жирный/курсив/зачёркнутый) и до 60 символов текста.
 */
static void print_marked_text(const VTL_publication_MarkedText* marked)
{
    if (!marked) {
        printf("  <nil>\n");
        return;
    }
    printf("  частей: %zu\n", marked->length);
    for (size_t i = 0; i < marked->length; ++i) {
        const VTL_publication_marked_text_Part* p = &marked->parts[i];
        const char* label = "обычный";
        if (p->type & VTL_TEXT_MODIFICATION_BOLD)               label = "ЖИРНЫЙ";
        else if (p->type & VTL_TEXT_MODIFICATION_ITALIC)        label = "КУРСИВ";
        else if (p->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) label = "ЗАЧЁРКНУТ";

        printf("    [%2zu] %-10s len=%-3zu \"", i, label, p->length);
        size_t to_print = p->length > 60 ? 60 : p->length;
        for (size_t k = 0; k < to_print; ++k) {
            char c = p->text[k];
            putchar((c == '\n') ? ' ' : c);
        }
        if (p->length > to_print) printf("...");
        printf("\"\n");
    }
}


/* Демо 1: парсинг короткого AsciiDoc-фрагмента из памяти */
static void demo_inline(void)
{
    const char* sample =
            "= AsciiDoc demo\n\n"
            "Параграф с *жирным* и _курсивом_, плюс [line-through]#зачёркнутый# "
            "фрагмент и `mono` для кода.\n"
            "NOTE: admonition с *выделением* внутри.\n";
    VTL_publication_Text src = { (char*)sample, strlen(sample) };

    printf("=== 1. Inline AsciiDoc → MarkedText (sequential) ===\n");
    printf("  входной фрагмент: %zu байт\n", src.length);

    VTL_publication_MarkedText* marked = NULL;
    VTL_AppResult res = VTL_asciidoc_ParseTextSequential(&src, &marked);
    if (res != VTL_res_kOk || !marked) {
        printf("  ОШИБКА: VTL_asciidoc_ParseTextSequential res=%d\n", (int)res);
        return;
    }
    print_marked_text(marked);
    free(marked->parts);
    free(marked);

    printf("\n=== 2. Inline AsciiDoc → MarkedText (parallel, 15 сканеров) ===\n");
    marked = NULL;
    res = VTL_asciidoc_ParseTextParallel(&src, &marked);
    if (res != VTL_res_kOk || !marked) {
        printf("  ОШИБКА: VTL_asciidoc_ParseTextParallel res=%d\n", (int)res);
        return;
    }
    print_marked_text(marked);
    free(marked->parts);
    free(marked);
}


/* Демо 3: парсинг внешнего файла text.adoc */
static void demo_file(const char* path)
{
    printf("\n=== 3. Парсинг файла %s ===\n", path);

    VTL_publication_MarkedText* marked = NULL;
    VTL_AppResult res = VTL_asciidoc_ParseFile(path, &marked);
    if (res != VTL_res_kOk || !marked) {
        printf("  файл недоступен или пустой (res=%d) — пропускаю\n", (int)res);
        return;
    }
    print_marked_text(marked);
    free(marked->parts);
    free(marked);
}


/*
 * Демо 4: бенч параллелизма.
 * Раздуваем фрагмент до целевого размера (≥ doc_size_kb KB), парсим
 * sequential vs parallel, считаем ускорение.
 */
static char* build_large_doc(size_t target_kb, size_t* out_len)
{
    const char* fragment =
            ":author: Timofeenko\n\n"
            "= Section Heading Level One\n"
            "== Section Heading Level Two\n\n"
            "Paragraph with *bold*, _italic_, `mono`, [line-through]#strike# inline.\n"
            "NOTE: Admonition with *bold inside*.\n"
            "WARNING: Another admonition with _italic_ markup inside it.\n\n"
            "* First bullet with *strong* inside\n"
            "* Second bullet with _emphasis_ inside\n"
            ". Numbered item one\n"
            ". Numbered item two\n\n"
            "[source,c]\n----\nint main(void) { return 0; }\n----\n\n"
            "Final paragraph with markers: *b* _i_ `m` [line-through]#s# mixed.\n\n";

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


static void demo_bench(size_t doc_size_kb, size_t iterations)
{
    size_t src_len = 0;
    char* buf = build_large_doc(doc_size_kb, &src_len);
    if (!buf) {
        printf("\n=== 4. Bench пропущен (нет памяти) ===\n");
        return;
    }
    VTL_publication_Text src = { buf, src_len };

    printf("\n=== 4. Bench: AsciiDoc %zu KB × %zu итераций ===\n",
           doc_size_kb, iterations);

    double t_seq = VTL_asciidoc_BenchSequential(&src, iterations);
    double t_par = VTL_asciidoc_BenchParallel(&src, iterations);
    double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;

    printf("  Sequential : %.4f с\n", t_seq);
    printf("  Parallel   : %.4f с\n", t_par);
    printf("  Speedup    : %.3fx\n", speedup);

    free(buf);
}


/*
 * Демо 5: batch-обработка нескольких файлов параллельно.
 * Раскладываем один и тот же fragment в N временных .adoc файлов
 * и парсим их пачкой через ParseFileBatchParallel (поток на файл).
 */
static void demo_batch(size_t doc_size_kb, size_t files)
{
    size_t src_len = 0;
    char* buf = build_large_doc(doc_size_kb, &src_len);
    if (!buf) return;

    const char* path = "_timofeenko_bench.adoc";
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

    printf("\n=== 5. Batch: %zu файлов × %zu KB параллельно ===\n",
           files, doc_size_kb);

    VTL_AppResult res = VTL_asciidoc_ParseFileBatchParallel(paths, files, out);
    if (res != VTL_res_kOk) {
        printf("  ОШИБКА: VTL_asciidoc_ParseFileBatchParallel res=%d\n", (int)res);
    } else {
        size_t total_parts = 0;
        for (size_t i = 0; i < files; ++i) {
            if (out[i]) total_parts += out[i]->length;
        }
        printf("  всего частей по %zu файлам: %zu\n", files, total_parts);
    }

    for (size_t i = 0; i < files; ++i) {
        if (out[i]) {
            free(out[i]->parts);
            free(out[i]);
        }
    }
    free(paths);
    free(out);
    remove(path);
}


int main(int argc, char** argv)
{
    const char* file_path = (argc > 1) ? argv[1] : "text.adoc";

    printf("===========================================\n");
    printf("  VTL AsciiDoc demo (main_timofeenko)\n");
    printf("===========================================\n\n");

    demo_inline();
    demo_file(file_path);
    demo_bench(/*doc_size_kb=*/512, /*iterations=*/30);
    demo_batch(/*doc_size_kb=*/128, /*files=*/8);

    printf("\n=== готово ===\n");
    return 0;
}
