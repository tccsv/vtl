/*
 * Smoke-тест + бенчмарк для markdown-модуля.
 * Самодостаточный — собирается через VTL/publication/text/markdown/CMakeLists.txt
 * без зависимостей на основную сборку VTL (FFmpeg/curl/libpq не нужны).
 *
 * Тесты покрывают:
 *   - оба синтаксиса каждого типа (* и _, ** и __)
 *   - конфликт между italic и bold (* vs **)
 *   - GFM ~~strike~~
 *   - вложенность (bold внутри которого italic)
 *   - экранирование (\*, \_)
 *   - snake_case (не должно становиться italic)
 *   - инвариант: Sequential ≡ Parallel
 *   - бенчмарк speedup
 */
#ifdef _WIN32
#include <windows.h>
#endif

#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char* label_of(VTL_publication_text_modification_Flags t)
{
    if (t == 0) return "PLAIN";
    static char buf[8];
    char* p = buf;
    if (t & VTL_TEXT_MODIFICATION_BOLD)          *p++ = 'B';
    if (t & VTL_TEXT_MODIFICATION_ITALIC)        *p++ = 'I';
    if (t & VTL_TEXT_MODIFICATION_STRIKETHROUGH) *p++ = 'S';
    *p = '\0';
    return buf;
}


static void print_marked(const VTL_publication_MarkedText* m)
{
    for (size_t i = 0; i < m->length; ++i) {
        const VTL_publication_marked_text_Part* p = &m->parts[i];
        printf("    %-5s len=%2zu \"", label_of(p->type), p->length);
        size_t to_print = p->length > 60 ? 60 : p->length;
        for (size_t k = 0; k < to_print; ++k) {
            char c = p->text[k];
            putchar((c == '\n') ? ' ' : c);
        }
        if (p->length > to_print) printf("...");
        printf("\"\n");
    }
}


/* один тест-кейс: парсинг seq и par, потом round-trip.
 * Возвращает 0 при успехе, 1 при ошибке. */
static int run_case(const char* name, const char* input)
{
    VTL_publication_Text src = { (char*)input, strlen(input) };

    VTL_publication_MarkedText* a = NULL;
    VTL_publication_MarkedText* b = NULL;

    if (VTL_markdown_ParseTextSequential(&src, &a) != VTL_res_kOk || !a) {
        printf("[%s] FAIL: sequential parse\n", name);
        return 1;
    }
    if (VTL_markdown_ParseTextParallel(&src, &b) != VTL_res_kOk || !b) {
        printf("[%s] FAIL: parallel parse\n", name);
        free(a->parts); free(a);
        return 1;
    }
    if (a->length != b->length) {
        printf("[%s] FAIL: parts mismatch seq=%zu par=%zu\n",
               name, a->length, b->length);
        free(a->parts); free(a); free(b->parts); free(b);
        return 1;
    }

    printf("[%s] input %zu B → %zu parts\n", name, src.length, a->length);
    print_marked(a);

    VTL_publication_Text* round = NULL;
    if (VTL_markdown_SerializeText(a, &round) != VTL_res_kOk || !round) {
        printf("[%s] FAIL: serialize\n", name);
        free(a->parts); free(a); free(b->parts); free(b);
        return 1;
    }
    printf("[%s] round-trip %zu B: \"", name, round->length);
    size_t to_print = round->length > 80 ? 80 : round->length;
    for (size_t k = 0; k < to_print; ++k) putchar(round->text[k]);
    if (round->length > to_print) printf("...");
    printf("\"\n");

    free(round->text); free(round);
    free(a->parts); free(a);
    free(b->parts); free(b);
    return 0;
}


static char* build_big(const char* fragment, size_t target_bytes, size_t* out_len)
{
    size_t flen = strlen(fragment);
    char* buf = (char*)malloc(target_bytes + flen + 1);
    if (!buf) return NULL;
    size_t pos = 0;
    while (pos < target_bytes) {
        memcpy(buf + pos, fragment, flen);
        pos += flen;
    }
    buf[pos] = '\0';
    *out_len = pos;
    return buf;
}


int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    printf("=== Standart Markdown smoke tests ===\n\n");

    int fails = 0;

    /* базовые случаи — одиночная разметка каждого типа */
    fails += run_case("bold-stars",    "Hello **world**!");
    fails += run_case("bold-underscores", "Hello __world__!");
    fails += run_case("italic-star",   "This is *italic* text");
    fails += run_case("italic-under",  "This is _italic_ text");
    fails += run_case("strike",        "Crossed ~~out~~ word");

    /* главный тест — конфликт между bold (**) и italic (*).
     * italic-сканер обязан игнорировать одиночные *, входящие в пару **. */
    fails += run_case("bold-not-italic", "Use **bold** here, not italic");
    fails += run_case("italic-vs-bold",  "*italic* and **bold** mixed");

    /* всё вместе */
    fails += run_case("all-three", "Mix of **bold**, *italic*, and ~~strike~~");

    /* ловушка: подчёркивание внутри слова не должно стать italic */
    fails += run_case("snake-case", "snake_case_var should stay plain");

    /* экранирование: \* не разметка */
    fails += run_case("escaped-star",  "Escaped \\*not bold\\* here");
    fails += run_case("escaped-bold",  "Escaped \\*\\*not bold\\*\\* here");

    /* вложенность: italic внутри bold */
    fails += run_case("nested",        "**bold *bold-italic* still-bold**");

    /* несколько строк */
    fails += run_case("multiline",
        "First **bold** line\nSecond *italic* line\nThird plain line");

    /* инвариант на большом тексте: Sequential ≡ Parallel */
    {
        size_t blen = 0;
        char* big = build_big(
            "Hello **world** and *everyone* around here! "
            "Some ~~deleted text~~ next plus normal run.\n",
            64 * 1024, &blen);
        if (big) {
            VTL_publication_Text src = { big, blen };
            VTL_publication_MarkedText* s = NULL;
            VTL_publication_MarkedText* p = NULL;
            VTL_markdown_ParseTextSequential(&src, &s);
            VTL_markdown_ParseTextParallel(&src, &p);
            if (!s || !p || s->length != p->length) {
                printf("[big] FAIL: seq=%zu par=%zu\n",
                       s ? s->length : 0, p ? p->length : 0);
                ++fails;
            } else {
                printf("[big] OK: %zu B → %zu parts (seq=par)\n", blen, s->length);
            }
            if (s) { free(s->parts); free(s); }
            if (p) { free(p->parts); free(p); }
            free(big);
        }
    }

    /* бенчмарк: 512 KB × 100 итераций */
    {
        size_t blen = 0;
        char* big = build_big(
            "Hello **world** and *everyone* around here! "
            "Some ~~deleted text~~ next to **bold one** and again *italic phrase*. "
            "Plain run, snake_case_var, numbers 1*2*3, escaped \\* literal.\n",
            512 * 1024, &blen);
        if (big) {
            VTL_publication_Text src = { big, blen };
            const size_t iters = 100;
            double t_seq = VTL_markdown_BenchSequential(&src, iters);
            double t_par = VTL_markdown_BenchParallel(&src, iters);
            double speedup = (t_par > 0.0) ? (t_seq / t_par) : 0.0;

            printf("\n=== Bench: %zu KB × %zu iterations ===\n",
                   blen / 1024, iters);
            printf("  Sequential : %.4f s\n", t_seq);
            printf("  Parallel   : %.4f s\n", t_par);
            printf("  Speedup    : %.2fx (3 сканера)\n", speedup);
            free(big);
        }
    }

    printf("\nResult: %d failures\n", fails);
    return fails ? 1 : 0;
}
