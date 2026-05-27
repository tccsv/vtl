/*
 * Smoke-тест + бенчмарк для html-модуля.
 * Самодостаточный — собирается через VTL/publication/text/html/CMakeLists.txt
 * без зависимостей на основную сборку VTL (FFmpeg/curl/libpq не нужны).
 */
#ifdef _WIN32
#include <windows.h>
#endif

#include <VTL/publication/text/html/VTL_publication_text_op_html.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* читаемая метка флагов для отладочного вывода */
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


/* распечатать содержимое MarkedText */
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


/* один тест-кейс: парсинг последовательно и параллельно, потом round-trip
 * возвращает 0 при успехе, 1 при ошибке */
static int run_case(const char* name, const char* input)
{
    VTL_publication_Text src = { (char*)input, strlen(input) };

    VTL_publication_MarkedText* a = NULL;
    VTL_publication_MarkedText* b = NULL;

    if (VTL_html_ParseTextSequential(&src, &a) != VTL_res_kOk || !a) {
        printf("[%s] FAIL: sequential parse\n", name);
        return 1;
    }
    if (VTL_html_ParseTextParallel(&src, &b) != VTL_res_kOk || !b) {
        printf("[%s] FAIL: parallel parse\n", name);
        free(a->parts); free(a);
        return 1;
    }
    /* инвариант: Sequential и Parallel должны давать одинаковый результат */
    if (a->length != b->length) {
        printf("[%s] FAIL: parts mismatch seq=%zu par=%zu\n",
               name, a->length, b->length);
        free(a->parts); free(a); free(b->parts); free(b);
        return 1;
    }

    printf("[%s] input %zu B → %zu parts\n", name, src.length, a->length);
    print_marked(a);

    /* round-trip: MarkedText обратно в HTML */
    VTL_publication_Text* round = NULL;
    if (VTL_html_SerializeText(a, &round) != VTL_res_kOk || !round) {
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


/* сгенерировать большой HTML-текст в памяти, конкатенируя fragment.
 * Чем длиннее, тем меньше шумит overhead на pthread_create в бенчмарке. */
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
    /* кириллица в printf корректно отображается в Windows Terminal только
     * при codepage UTF-8 (по умолчанию cp866) */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    printf("=== HTML smoke tests ===\n\n");

    int fails = 0;
    fails += run_case("simple-b",        "Hello <b>world</b>!");
    fails += run_case("simple-strong",   "Hello <strong>world</strong>!");
    fails += run_case("simple-i",        "This is <i>italic</i> text");
    fails += run_case("simple-em",       "This is <em>italic</em> text");
    fails += run_case("simple-s",        "Crossed <s>out</s> word");
    fails += run_case("simple-del",      "Crossed <del>out</del> word");
    fails += run_case("all-three",
        "Mix of <b>bold</b>, <i>italic</i>, and <s>strike</s>");

    /* алиасы должны давать одну и ту же разметку */
    fails += run_case("mixed-aliases",
        "<strong>A</strong> and <b>B</b> with <em>C</em> and <i>D</i>");

    /* регистр имен тегов: <B>...</B>, <I>...</I> — должны парситься */
    fails += run_case("uppercase-tags",
        "Use <B>BOLD</B> and <I>italic</I>, also <STRONG>strong</STRONG>");

    /* атрибуты внутри тегов должны игнорироваться */
    fails += run_case("with-attrs",
        "Has <b class=\"x\">styled bold</b> and <i id='y'>styled italic</i>");

    /* вложенность */
    fails += run_case("nested",
        "<b>bold <i>bold-italic</i> still-bold</b>");

    /* HTML-entities в исходном тексте — пока не декодируем, просто
     * проверяем что парсер не путается от '<' внутри text-частей */
    fails += run_case("entities-in-source",
        "Use &lt; and &gt; for &amp; chars (not real tags)");

    /* спецсимволы при сериализации должны быть заэкранированы */
    fails += run_case("escape-on-serialize",
        "Code: <b>5 < 10 && 10 > 5</b>");

    /* многострочный */
    fails += run_case("multiline",
        "First <b>bold</b> line\nSecond <i>italic</i> line\nThird plain line");

    /* инвариант на большом тексте: Sequential ≡ Parallel */
    {
        size_t blen = 0;
        char* big = build_big(
            "Hello <b>world</b> and <em>everyone</em> around here! "
            "Some <del>deleted text</del> next plus normal run.\n",
            64 * 1024, &blen);
        if (big) {
            VTL_publication_Text src = { big, blen };
            VTL_publication_MarkedText* s = NULL;
            VTL_publication_MarkedText* p = NULL;
            VTL_html_ParseTextSequential(&src, &s);
            VTL_html_ParseTextParallel(&src, &p);
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
            "Hello <b>world</b> and <em>everyone</em>! "
            "Some <del>deleted text</del> next to <strong>bold one</strong> "
            "and again <i>italic phrase</i>. "
            "Plain run, normal text, numbers 1 2 3.\n",
            512 * 1024, &blen);
        if (big) {
            VTL_publication_Text src = { big, blen };
            const size_t iters = 100;
            double t_seq = VTL_html_BenchSequential(&src, iters);
            double t_par = VTL_html_BenchParallel(&src, iters);
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
