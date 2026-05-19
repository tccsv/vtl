#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/test/VTL_test_data.h>
#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki.h>
#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_publication_markup_text_flags.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>


static void free_marked(VTL_publication_MarkedText* m)
{
    if (!m) return;
    free(m->parts);
    free(m);
}


static void free_text(VTL_publication_Text* t)
{
    if (!t) return;
    free(t->text);
    free(t);
}


TEST(test_mediawiki_parse_inline_flags)
{
    const char* wiki = "before '''bold''' middle ''italic'' tail";
    VTL_publication_Text src = { (char*)wiki, strlen(wiki) };

    VTL_publication_MarkedText* m = NULL;
    VTL_AppResult res = VTL_mediawiki_ParseTextParallel(&src, &m);
    VTL_ASSERT(res == VTL_res_kOk, "parse must succeed");
    VTL_ASSERT(m != NULL, "result must be non-null");
    if (!m) return;


    int has_bold = 0, has_italic = 0;
    for (size_t i = 0; i < m->length; ++i) {
        if (m->parts[i].type & VTL_TEXT_MODIFICATION_BOLD)   has_bold   = 1;
        if (m->parts[i].type & VTL_TEXT_MODIFICATION_ITALIC) has_italic = 1;
    }
    VTL_ASSERT(has_bold == 1,   "must contain BOLD part");
    VTL_ASSERT(has_italic == 1, "must contain ITALIC part");
    free_marked(m);
}


TEST(test_mediawiki_round_trip)
{
    const char* wiki = "plain '''bold''' text ''italic'' end";
    VTL_publication_Text src = { (char*)wiki, strlen(wiki) };

    VTL_publication_MarkedText* m = NULL;
    VTL_AppResult res = VTL_mediawiki_ParseTextParallel(&src, &m);
    VTL_ASSERT(res == VTL_res_kOk && m != NULL, "parse ok");
    if (!m) return;

    VTL_publication_Text* round = NULL;
    res = VTL_mediawiki_EmitFromMarkedText(m, &round);
    VTL_ASSERT(res == VTL_res_kOk && round != NULL, "emit ok");
    if (!round) { free_marked(m); return; }


    VTL_ASSERT(strstr(round->text, "'''bold'''") != NULL,  "must re-emit '''bold'''");
    VTL_ASSERT(strstr(round->text, "''italic''") != NULL,  "must re-emit ''italic''");

    free_text(round);
    free_marked(m);
}


TEST(test_mediawiki_strike_tags)
{
    const char* wiki = "x <s>aaa</s> y <del>bbb</del> z";
    VTL_publication_Text src = { (char*)wiki, strlen(wiki) };

    VTL_publication_MarkedText* m = NULL;
    VTL_ASSERT(VTL_mediawiki_ParseTextParallel(&src, &m) == VTL_res_kOk, "parse ok");
    if (!m) return;

    int strike_parts = 0;
    for (size_t i = 0; i < m->length; ++i) {
        if (m->parts[i].type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) ++strike_parts;
    }
    VTL_ASSERT(strike_parts == 2, "must yield two STRIKE parts (<s> and <del>)");
    free_marked(m);
}


TEST(test_mediawiki_seq_eq_par)
{
    const char* wiki =
        "== H ==\nintro '''b''' and ''i''\n"
        "* item one\n* item two\n"
        "[[Page|label]] and [https://e.com lbl]\n"
        "{{Tpl|x=1}}<ref>note</ref><!-- c -->\n"
        "<s>strike</s> tail\n";
    VTL_publication_Text src = { (char*)wiki, strlen(wiki) };

    VTL_publication_MarkedText *a = NULL, *b = NULL;
    VTL_ASSERT(VTL_mediawiki_ParseTextSequential(&src, &a) == VTL_res_kOk, "seq ok");
    VTL_ASSERT(VTL_mediawiki_ParseTextParallel(&src, &b)   == VTL_res_kOk, "par ok");
    if (!a || !b) { free_marked(a); free_marked(b); return; }

    VTL_ASSERT(a->length == b->length, "seq and par must produce same number of parts");
    if (a->length == b->length) {
        int mismatch = 0;
        for (size_t i = 0; i < a->length; ++i) {
            if (a->parts[i].type != b->parts[i].type ||
                a->parts[i].length != b->parts[i].length) {
                mismatch = 1; break;
            }
        }
        VTL_ASSERT(mismatch == 0, "seq and par parts must match");
    }
    free_marked(a); free_marked(b);
}


TEST(test_mediawiki_dispatch_through_init)
{
    const char* wiki = "x '''y''' z";
    VTL_publication_Text src = { (char*)wiki, strlen(wiki) };

    VTL_publication_MarkedText* m = NULL;
    VTL_AppResult res = VTL_publication_marked_text_Init(&m, &src, VTL_markup_type_kMediaWiki);
    VTL_ASSERT(res == VTL_res_kOk && m != NULL, "Init(kMediaWiki) must succeed");
    if (!m) return;

    int has_bold = 0;
    for (size_t i = 0; i < m->length; ++i)
        if (m->parts[i].type & VTL_TEXT_MODIFICATION_BOLD) has_bold = 1;
    VTL_ASSERT(has_bold == 1, "dispatched Init must set BOLD flag");
    free_marked(m);
}


TEST(test_mediawiki_parallel_speedup)
{

    const char* frag =
        "== Heading ==\n"
        "para with '''bold''' and ''italic'' and <s>strike</s> plus "
        "[[Internal|label]] and [https://x.y txt] and {{Tpl|a=1}} "
        "<nowiki>'''raw'''</nowiki><ref>n</ref><!-- comment -->.\n"
        "* item one\n* item two\n** nested\n# numbered\n; term\n: def\n"
        "----\n"
        "more text with ''mixed'' ''formatting'' patterns in one line.\n\n";
    size_t flen = strlen(frag);
    size_t target = 256 * 1024;
    char* buf = (char*)malloc(target + flen + 1);
    VTL_ASSERT(buf != NULL, "alloc ok");
    if (!buf) return;
    size_t pos = 0;
    while (pos < target) { memcpy(buf + pos, frag, flen); pos += flen; }
    buf[pos] = '\0';

    VTL_publication_Text src = { buf, pos };
    const size_t iters = 5;
    double t_seq = VTL_mediawiki_BenchSequential(&src, iters);
    double t_par = VTL_mediawiki_BenchParallel(&src, iters);

    printf("  [mediawiki bench] %zu KB x %zu iters: seq=%.4fs par=%.4fs speedup=%.3fx\n",
           pos / 1024, iters, t_seq, t_par, (t_par > 0.0) ? t_seq / t_par : 0.0);


    VTL_ASSERT(t_seq > 0.0 && t_par > 0.0, "both timings must be positive");

    free(buf);
}


// ============================================================
static char* test_mw_slurp(const char* path, size_t* out_len)
{
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    if (out_len) *out_len = n;
    return buf;
}


#ifndef VTL_WIKI_FIXTURE
#define VTL_WIKI_FIXTURE "../text.wiki"
#endif

TEST(test_mediawiki_pipeline_from_file)
{
    size_t src_len = 0;
    char* src_buf = test_mw_slurp(VTL_WIKI_FIXTURE, &src_len);
    VTL_ASSERT(src_buf != NULL, "text.wiki fixture must exist");
    if (!src_buf) return;
    VTL_ASSERT(src_len > 0, "wiki fixture must be non-empty");

    VTL_publication_Text src = { src_buf, src_len };

    VTL_publication_MarkedText* marked = NULL;
    VTL_ASSERT(VTL_mediawiki_ParseTextParallel(&src, &marked) == VTL_res_kOk,
               "parallel parse of fixture must succeed");
    VTL_ASSERT(marked != NULL && marked->length > 0,
               "fixture must produce at least one part");


    VTL_publication_Text* plain = NULL;
    VTL_ASSERT(VTL_publication_marked_text_TransformToRegularText(&plain, marked) == VTL_res_kOk,
               "TransformToRegularText must succeed");
    VTL_ASSERT(plain != NULL && plain->length > 0,
               "plain output must be non-empty");
    VTL_ASSERT(plain->length <= src_len,
               "stripped text cannot be longer than source");

    free(plain->text); free(plain);
    free_marked(marked);
    free(src_buf);
}


TEST(test_mediawiki_pipeline_large)
{

    const char* frag =
        "== Section ==\n"
        "Lead with '''Bold''' and ''italic'' and [[Link|label]] inline.\n"
        "{{Infobox|key=value}} <ref>citation</ref> some text.\n"
        "* bullet '''one'''\n"
        "* bullet ''two''\n";
    size_t flen = strlen(frag);
    size_t target = 256 * 1024;
    char* buf = (char*)malloc(target + flen + 1);
    VTL_ASSERT(buf != NULL, "alloc ok");
    if (!buf) return;
    size_t pos = 0;
    while (pos < target) { memcpy(buf + pos, frag, flen); pos += flen; }
    buf[pos] = '\0';

    VTL_publication_Text src = { buf, pos };

    VTL_publication_MarkedText* marked = NULL;
    VTL_ASSERT(VTL_publication_marked_text_Init(&marked, &src, VTL_markup_type_kMediaWiki) == VTL_res_kOk,
               "Init dispatch must succeed on large text");
    VTL_ASSERT(marked != NULL, "marked text non-null");
    if (!marked) { free(buf); return; }

    VTL_publication_Text* plain = NULL;
    VTL_ASSERT(VTL_publication_marked_text_TransformToRegularText(&plain, marked) == VTL_res_kOk,
               "transform large doc to plain text");
    VTL_ASSERT(plain && plain->length > 0 && plain->length < src.length,
               "plain text must be shorter than source (markup stripped)");


    const char* out_path = "test_pipeline_out.txt";
    FILE* f = fopen(out_path, "wb");
    VTL_ASSERT(f != NULL, "open output");
    if (f) {
        size_t w = fwrite(plain->text, 1, plain->length, f);
        fclose(f);
        VTL_ASSERT(w == plain->length, "full write");
        remove(out_path);
    }

    free(plain->text); free(plain);
    free_marked(marked);
    free(buf);
}

int main(void)
{
    VTL_RUN_TEST(test_mediawiki_parse_inline_flags);
    VTL_RUN_TEST(test_mediawiki_round_trip);
    VTL_RUN_TEST(test_mediawiki_strike_tags);
    VTL_RUN_TEST(test_mediawiki_seq_eq_par);
    VTL_RUN_TEST(test_mediawiki_dispatch_through_init);
    VTL_RUN_TEST(test_mediawiki_parallel_speedup);
    VTL_RUN_TEST(test_mediawiki_pipeline_from_file);
    VTL_RUN_TEST(test_mediawiki_pipeline_large);
    return VTL_test_result();
}
