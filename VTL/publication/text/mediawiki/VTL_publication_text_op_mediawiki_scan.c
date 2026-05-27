#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_scan.h>
#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_convert.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <VTL/utils/threading/VTL_utils_threading_thread_compat.h>


#define VTL_MEDIAWIKI_SCANNER_COUNT 13


static bool VTL_mediawiki_is_at_line_start(const char* text, size_t pos)
{
    if (pos == 0) return true;
    return text[pos - 1] == '\n';
}


static size_t VTL_mediawiki_line_end(const char* text, size_t text_len, size_t from)
{
    size_t i = from;
    while (i < text_len && text[i] != '\n') ++i;
    return i;
}


static size_t VTL_mediawiki_run_length(const char* text, size_t text_len, size_t pos, char c)
{
    size_t n = 0;
    while (pos + n < text_len && text[pos + n] == c) ++n;
    return n;
}


static void* VTL_mediawiki_scan_apostrophes(VTL_mediawiki_ScanContext* ctx,
                                             size_t target_run_len,
                                             VTL_mediawiki_MarkerKind start_kind,
                                             VTL_mediawiki_MarkerKind end_kind)
{
    bool open = false;
    size_t i = 0;
    while (i < ctx->text_length) {
        if (ctx->text[i] != '\'') { ++i; continue; }
        size_t run = VTL_mediawiki_run_length(ctx->text, ctx->text_length, i, '\'');


        bool match = (run == target_run_len) || (run >= 5 && target_run_len <= 3);
        if (match) {
            VTL_mediawiki_Marker m = {
                open ? end_kind : start_kind,
                i, target_run_len, 0
            };
            if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            open = !open;
        }
        i += run;
    }
    return NULL;
}

void* VTL_mediawiki_ScanBold(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_mediawiki_scan_apostrophes(ctx, 3,
        VTL_mediawiki_marker_kBoldStart, VTL_mediawiki_marker_kBoldEnd);
}

void* VTL_mediawiki_ScanItalic(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_mediawiki_scan_apostrophes(ctx, 2,
        VTL_mediawiki_marker_kItalicStart, VTL_mediawiki_marker_kItalicEnd);
}


static void* VTL_mediawiki_scan_tag_pair(VTL_mediawiki_ScanContext* ctx,
                                          const char* open_tag, size_t open_len,
                                          const char* close_tag, size_t close_len,
                                          VTL_mediawiki_MarkerKind kind_start,
                                          VTL_mediawiki_MarkerKind kind_end)
{
    size_t i = 0;
    while (i + open_len <= ctx->text_length) {
        if (memcmp(ctx->text + i, open_tag, open_len) != 0) { ++i; continue; }

        size_t end_pos = (size_t)-1;
        for (size_t j = i + open_len; j + close_len <= ctx->text_length; ++j) {
            if (memcmp(ctx->text + j, close_tag, close_len) == 0) { end_pos = j; break; }
        }
        if (end_pos == (size_t)-1) { ++i; continue; }
        VTL_mediawiki_Marker s = { kind_start, i, open_len, 0 };
        VTL_mediawiki_Marker e = { kind_end, end_pos, close_len, 0 };
        if (VTL_mediawiki_MarkerListPush(ctx->out, s) != VTL_res_kOk) return (void*)1;
        if (VTL_mediawiki_MarkerListPush(ctx->out, e) != VTL_res_kOk) return (void*)1;
        i = end_pos + close_len;
    }
    return NULL;
}

void* VTL_mediawiki_ScanStrike(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    void* r = VTL_mediawiki_scan_tag_pair(ctx, "<s>", 3, "</s>", 4,
        VTL_mediawiki_marker_kStrikeStart, VTL_mediawiki_marker_kStrikeEnd);
    if (r != NULL) return r;
    return VTL_mediawiki_scan_tag_pair(ctx, "<del>", 5, "</del>", 6,
        VTL_mediawiki_marker_kStrikeStart, VTL_mediawiki_marker_kStrikeEnd);
}


void* VTL_mediawiki_ScanHeading(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_mediawiki_is_at_line_start(ctx->text, i)) {
            i = VTL_mediawiki_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }

        size_t left = 0;
        while (i + left < ctx->text_length && ctx->text[i + left] == '=' && left < 6) ++left;
        if (left == 0) {
            i = VTL_mediawiki_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t line_end = VTL_mediawiki_line_end(ctx->text, ctx->text_length, i);

        size_t right = 0;
        while (right < left && line_end > i + left + right &&
               ctx->text[line_end - 1 - right] == '=') ++right;
        if (right == left && line_end > i + left * 2) {
            VTL_mediawiki_Marker m = {
                VTL_mediawiki_marker_kHeading, i, line_end - i, (int)left
            };
            if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = line_end + 1;
    }
    return NULL;
}


void* VTL_mediawiki_ScanList(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_mediawiki_is_at_line_start(ctx->text, i)) {
            i = VTL_mediawiki_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t level = 0;
        while (i + level < ctx->text_length) {
            char c = ctx->text[i + level];
            if (c == '*' || c == '#' || c == ';' || c == ':') ++level;
            else break;
        }
        if (level > 0 && i + level < ctx->text_length && ctx->text[i + level] == ' ') {
            VTL_mediawiki_Marker m = {
                VTL_mediawiki_marker_kListItem, i, level + 1, (int)level
            };
            if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = VTL_mediawiki_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}


void* VTL_mediawiki_ScanHorizontalRule(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_mediawiki_is_at_line_start(ctx->text, i)) {
            i = VTL_mediawiki_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t run = VTL_mediawiki_run_length(ctx->text, ctx->text_length, i, '-');
        size_t line_end = VTL_mediawiki_line_end(ctx->text, ctx->text_length, i);
        if (run >= 4 && i + run == line_end) {
            VTL_mediawiki_Marker m = {
                VTL_mediawiki_marker_kHorizontalRule, i, run, 0
            };
            if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = line_end + 1;
    }
    return NULL;
}


static void* VTL_mediawiki_scan_bracketed(VTL_mediawiki_ScanContext* ctx,
                                           const char* open_seq, size_t open_len,
                                           const char* close_seq, size_t close_len,
                                           VTL_mediawiki_MarkerKind kind,
                                           bool stop_at_newline)
{
    size_t i = 0;
    while (i + open_len <= ctx->text_length) {
        if (memcmp(ctx->text + i, open_seq, open_len) != 0) { ++i; continue; }
        size_t end_pos = (size_t)-1;
        for (size_t j = i + open_len; j + close_len <= ctx->text_length; ++j) {
            if (stop_at_newline && ctx->text[j] == '\n') break;
            if (memcmp(ctx->text + j, close_seq, close_len) == 0) { end_pos = j; break; }
        }
        if (end_pos == (size_t)-1) { ++i; continue; }
        VTL_mediawiki_Marker m = {
            kind, i, end_pos + close_len - i, 0
        };
        if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = end_pos + close_len;
    }
    return NULL;
}

void* VTL_mediawiki_ScanInternalLink(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + 4 <= ctx->text_length) {
        if (ctx->text[i] != '[' || ctx->text[i + 1] != '[') { ++i; continue; }
        // Brace-balanced so [[File:foo|caption with [[nested]] link]] collapses to one span.
        int depth = 1;
        size_t j = i + 2;
        size_t end_pos = (size_t)-1;
        bool hit_newline = false;
        while (j + 1 < ctx->text_length) {
            if (ctx->text[j] == '\n') { hit_newline = true; break; }
            if (ctx->text[j] == '[' && ctx->text[j + 1] == '[') { depth++; j += 2; continue; }
            if (ctx->text[j] == ']' && ctx->text[j + 1] == ']') {
                depth--; j += 2;
                if (depth == 0) { end_pos = j; break; }
                continue;
            }
            ++j;
        }
        if (hit_newline || end_pos == (size_t)-1) { ++i; continue; }
        VTL_mediawiki_Marker m = {
            VTL_mediawiki_marker_kInternalLink, i, end_pos - i, 0, 0
        };
        if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = end_pos;
    }
    return NULL;
}


void* VTL_mediawiki_ScanExternalLink(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + 1 < ctx->text_length) {
        if (ctx->text[i] != '[') { ++i; continue; }

        if (i + 1 < ctx->text_length && ctx->text[i + 1] == '[') { i += 2; continue; }

        size_t p = i + 1;
        bool is_http  = (p + 7 <= ctx->text_length && memcmp(ctx->text + p, "http://",  7) == 0);
        bool is_https = (p + 8 <= ctx->text_length && memcmp(ctx->text + p, "https://", 8) == 0);
        if (!is_http && !is_https) { ++i; continue; }


        size_t end_pos = (size_t)-1;
        for (size_t j = p; j < ctx->text_length; ++j) {
            if (ctx->text[j] == '\n') break;
            if (ctx->text[j] == ']') { end_pos = j; break; }
        }
        if (end_pos == (size_t)-1) { ++i; continue; }

        VTL_mediawiki_Marker m = {
            VTL_mediawiki_marker_kExternalLink, i, end_pos - i + 1, 0
        };
        if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = end_pos + 1;
    }
    return NULL;
}

void* VTL_mediawiki_ScanTemplate(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + 4 <= ctx->text_length) {
        if (ctx->text[i] != '{' || ctx->text[i + 1] != '{') { ++i; continue; }
        // Brace-balanced match so nested templates collapse into one outer span.
        int depth = 1;
        size_t j = i + 2;
        size_t end_pos = (size_t)-1;
        while (j + 1 < ctx->text_length) {
            if (ctx->text[j] == '{' && ctx->text[j + 1] == '{') { depth++; j += 2; continue; }
            if (ctx->text[j] == '}' && ctx->text[j + 1] == '}') {
                depth--; j += 2;
                if (depth == 0) { end_pos = j; break; }
                continue;
            }
            ++j;
        }
        if (end_pos == (size_t)-1) { ++i; continue; }
        VTL_mediawiki_Marker m = {
            VTL_mediawiki_marker_kTemplate, i, end_pos - i, 0
        };
        if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = end_pos;
    }
    return NULL;
}


void* VTL_mediawiki_ScanNowiki(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_mediawiki_scan_tag_pair(ctx, "<nowiki>", 8, "</nowiki>", 9,
        VTL_mediawiki_marker_kNowikiStart, VTL_mediawiki_marker_kNowikiEnd);
}

void* VTL_mediawiki_ScanRef(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;


    size_t i = 0;
    while (i + 5 <= ctx->text_length) {
        if (memcmp(ctx->text + i, "<ref", 4) != 0) { ++i; continue; }

        size_t open_end = (size_t)-1;
        for (size_t j = i + 4; j < ctx->text_length; ++j) {
            if (ctx->text[j] == '>') { open_end = j; break; }
            if (ctx->text[j] == '\n') break;
        }
        if (open_end == (size_t)-1) { ++i; continue; }

        if (open_end > 0 && ctx->text[open_end - 1] == '/') {
            VTL_mediawiki_Marker m = {
                VTL_mediawiki_marker_kRef, i, open_end - i + 1, 0
            };
            if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            i = open_end + 1;
            continue;
        }

        size_t close_pos = (size_t)-1;
        for (size_t j = open_end + 1; j + 6 <= ctx->text_length; ++j) {
            if (memcmp(ctx->text + j, "</ref>", 6) == 0) { close_pos = j; break; }
        }
        if (close_pos == (size_t)-1) { ++i; continue; }
        VTL_mediawiki_Marker m = {
            VTL_mediawiki_marker_kRef, i, close_pos + 6 - i, 0
        };
        if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = close_pos + 6;
    }
    return NULL;
}

void* VTL_mediawiki_ScanComment(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + 4 <= ctx->text_length) {
        if (memcmp(ctx->text + i, "<!--", 4) != 0) { ++i; continue; }
        size_t close = (size_t)-1;
        for (size_t j = i + 4; j + 3 <= ctx->text_length; ++j) {
            if (memcmp(ctx->text + j, "-->", 3) == 0) { close = j; break; }
        }
        if (close == (size_t)-1) { ++i; continue; }
        VTL_mediawiki_Marker m = {
            VTL_mediawiki_marker_kComment, i, close + 3 - i, 0
        };
        if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = close + 3;
    }
    return NULL;
}


static int VTL_mediawiki_ascii_lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

static bool VTL_mediawiki_is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool VTL_mediawiki_is_alnum(char c)
{
    return VTL_mediawiki_is_alpha(c) || (c >= '0' && c <= '9');
}

static bool VTL_mediawiki_tag_name_equals(const char* a, size_t a_len, const char* b)
{
    size_t b_len = strlen(b);
    if (a_len != b_len) return false;
    for (size_t i = 0; i < a_len; ++i) {
        if (VTL_mediawiki_ascii_lower((unsigned char)a[i]) != b[i]) return false;
    }
    return true;
}

// Already covered by dedicated scanners — skip in the generic-tag pass.
static bool VTL_mediawiki_is_known_tag(const char* name, size_t name_len)
{
    static const char* known[] = { "s", "del", "ref", "nowiki" };
    for (size_t i = 0; i < sizeof(known) / sizeof(*known); ++i) {
        if (VTL_mediawiki_tag_name_equals(name, name_len, known[i])) return true;
    }
    return false;
}

// Generic HTML-style tag: <tag attrs>content</tag> or self-closing <tag .../>.
// Stores opening-tag length in `level` and closing-tag length in `aux` (0 for self-closing).
void* VTL_mediawiki_ScanGenericTag(void* arg)
{
    VTL_mediawiki_ScanContext* ctx = (VTL_mediawiki_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + 2 < ctx->text_length) {
        if (ctx->text[i] != '<') { ++i; continue; }
        size_t p = i + 1;
        if (p >= ctx->text_length) break;
        if (ctx->text[p] == '/' || ctx->text[p] == '!') { ++i; continue; }
        if (!VTL_mediawiki_is_alpha(ctx->text[p])) { ++i; continue; }

        size_t name_start = p;
        while (p < ctx->text_length && VTL_mediawiki_is_alnum(ctx->text[p])) ++p;
        size_t name_len = p - name_start;
        if (name_len == 0) { ++i; continue; }
        if (VTL_mediawiki_is_known_tag(ctx->text + name_start, name_len)) { ++i; continue; }

        size_t open_end = (size_t)-1;
        for (size_t j = p; j < ctx->text_length; ++j) {
            if (ctx->text[j] == '\n') break;
            if (ctx->text[j] == '>') { open_end = j; break; }
        }
        if (open_end == (size_t)-1) { ++i; continue; }

        size_t open_len = open_end - i + 1;
        bool self_closing = (open_end > 0 && ctx->text[open_end - 1] == '/');
        if (self_closing) {
            VTL_mediawiki_Marker m = {
                VTL_mediawiki_marker_kGenericTag, i, open_len, (int)open_len, 0
            };
            if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            i = open_end + 1;
            continue;
        }

        size_t close_pos = (size_t)-1;
        size_t close_len = 0;
        for (size_t j = open_end + 1; j + name_len + 3 <= ctx->text_length; ++j) {
            if (ctx->text[j] != '<' || ctx->text[j + 1] != '/') continue;
            bool match = true;
            for (size_t k = 0; k < name_len; ++k) {
                if (VTL_mediawiki_ascii_lower((unsigned char)ctx->text[j + 2 + k]) !=
                    VTL_mediawiki_ascii_lower((unsigned char)ctx->text[name_start + k])) {
                    match = false; break;
                }
            }
            if (match && ctx->text[j + 2 + name_len] == '>') {
                close_pos = j;
                close_len = name_len + 3;
                break;
            }
        }
        if (close_pos == (size_t)-1) { ++i; continue; }

        VTL_mediawiki_Marker m = {
            VTL_mediawiki_marker_kGenericTag, i,
            close_pos + close_len - i,
            (int)open_len, (int)close_len
        };
        if (VTL_mediawiki_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = close_pos + close_len;
    }
    return NULL;
}


typedef void* (*VTL_mediawiki_scanner_fn)(void*);


static const VTL_mediawiki_scanner_fn VTL_mediawiki_all_scanners[VTL_MEDIAWIKI_SCANNER_COUNT] = {
    VTL_mediawiki_ScanBold,
    VTL_mediawiki_ScanItalic,
    VTL_mediawiki_ScanStrike,
    VTL_mediawiki_ScanHeading,
    VTL_mediawiki_ScanList,
    VTL_mediawiki_ScanHorizontalRule,
    VTL_mediawiki_ScanInternalLink,
    VTL_mediawiki_ScanExternalLink,
    VTL_mediawiki_ScanTemplate,
    VTL_mediawiki_ScanNowiki,
    VTL_mediawiki_ScanRef,
    VTL_mediawiki_ScanComment,
    VTL_mediawiki_ScanGenericTag
};

/* Защита от рассинхрона VTL_MEDIAWIKI_SCANNER_COUNT и фактического числа
 * сканеров: добавление/удаление сканера без обновления дефайна → fail компиляции. */
_Static_assert(
    sizeof(VTL_mediawiki_all_scanners) / sizeof(VTL_mediawiki_all_scanners[0])
        == VTL_MEDIAWIKI_SCANNER_COUNT,
    "VTL_MEDIAWIKI_SCANNER_COUNT must match length of VTL_mediawiki_all_scanners");


VTL_AppResult VTL_mediawiki_ParseDocumentSequential(const VTL_publication_Text* src,
                                                     VTL_mediawiki_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_mediawiki_ScanContext ctx = { src->text, src->length, out };
    for (size_t i = 0; i < VTL_MEDIAWIKI_SCANNER_COUNT; ++i) {
        if (VTL_mediawiki_all_scanners[i](&ctx) != NULL) {
            return VTL_res_kErr;
        }
    }
    return VTL_mediawiki_SortMarkers(out);
}


VTL_AppResult VTL_mediawiki_ParseDocumentParallel(const VTL_publication_Text* src,
                                                   VTL_mediawiki_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;


    VTL_mediawiki_MarkerList partial_lists[VTL_MEDIAWIKI_SCANNER_COUNT];
    VTL_mediawiki_ScanContext contexts[VTL_MEDIAWIKI_SCANNER_COUNT];
    vtl_thread_t threads[VTL_MEDIAWIKI_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_MEDIAWIKI_SCANNER_COUNT; ++i) {
        VTL_mediawiki_MarkerListInit(&partial_lists[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial_lists[i];
    }


    size_t spawned = 0;
    for (size_t i = 0; i < VTL_MEDIAWIKI_SCANNER_COUNT; ++i) {
        if (vtl_thread_create(&threads[i],
                              VTL_mediawiki_all_scanners[i], &contexts[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < VTL_MEDIAWIKI_SCANNER_COUNT; ++i) {
        VTL_mediawiki_all_scanners[i](&contexts[i]);
    }


    void* worst = NULL;
    for (size_t i = 0; i < spawned; ++i) {
        void *res = vtl_thread_join(threads[i]);
        if (res != NULL) worst = res;
    }

    VTL_AppResult merge_res = VTL_mediawiki_MergeMarkers(partial_lists,
                                                         VTL_MEDIAWIKI_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_MEDIAWIKI_SCANNER_COUNT; ++i) {
        VTL_mediawiki_MarkerListFree(&partial_lists[i]);
    }

    if (worst != NULL) return VTL_res_kErr;
    return merge_res;
}
