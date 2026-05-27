#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki.h>
#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram.h>
#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown.h>
#include <VTL/publication/text/html/VTL_publication_text_op_html.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <stdlib.h>
#include <string.h>


// Resizable byte buffer used by the Telegram HTML emitter.
typedef struct _vtl_tg_buf { char* data; size_t len; size_t cap; } vtl_tg_buf;

static int vtl_tg_buf_grow(vtl_tg_buf* b, size_t need)
{
    if (b->cap >= need) return 0;
    size_t c = b->cap ? b->cap : 256;
    while (c < need) c *= 2;
    char* n = (char*)realloc(b->data, c);
    if (!n) return -1;
    b->data = n; b->cap = c; return 0;
}

static int vtl_tg_buf_append(vtl_tg_buf* b, const char* s, size_t n)
{
    if (vtl_tg_buf_grow(b, b->len + n + 1)) return -1;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 0;
}

static int vtl_tg_buf_append_str(vtl_tg_buf* b, const char* s)
{
    return vtl_tg_buf_append(b, s, strlen(s));
}


// Recognize the most common HTML entities found in MediaWiki source.
// Returns number of consumed bytes (including the '&'), 0 if not an entity.
static size_t vtl_decode_entity(const char* s, size_t n, vtl_tg_buf* out)
{
    if (n == 0 || s[0] != '&') return 0;
    size_t semi = 0;
    for (size_t j = 1; j < n && j < 12; ++j) {
        if (s[j] == ';') { semi = j; break; }
        if (s[j] == ' ' || s[j] == '<' || s[j] == '&' || s[j] == '\n') return 0;
    }
    if (semi == 0) return 0;
    size_t elen = semi - 1;
    const char* e = s + 1;
    if (elen >= 2 && e[0] == '#') {
        // Numeric character reference; just drop for simplicity.
        return semi + 1;
    }
    if (elen == 4 && memcmp(e, "nbsp", 4) == 0) { vtl_tg_buf_append(out, " ", 1); return semi + 1; }
    if (elen == 3 && memcmp(e, "amp",  3) == 0) { vtl_tg_buf_append(out, "&amp;", 5); return semi + 1; }
    if (elen == 2 && memcmp(e, "lt",   2) == 0) { vtl_tg_buf_append(out, "&lt;",  4); return semi + 1; }
    if (elen == 2 && memcmp(e, "gt",   2) == 0) { vtl_tg_buf_append(out, "&gt;",  4); return semi + 1; }
    if (elen == 4 && memcmp(e, "quot", 4) == 0) { vtl_tg_buf_append(out, "\"",    1); return semi + 1; }
    if (elen == 4 && memcmp(e, "apos", 4) == 0) { vtl_tg_buf_append(out, "'",     1); return semi + 1; }
    if (elen == 5 && memcmp(e, "ndash",5) == 0) { vtl_tg_buf_append(out, "\xE2\x80\x93", 3); return semi + 1; }
    if (elen == 5 && memcmp(e, "mdash",5) == 0) { vtl_tg_buf_append(out, "\xE2\x80\x94", 3); return semi + 1; }
    if (elen == 6 && memcmp(e, "hellip",6) == 0) { vtl_tg_buf_append(out, "\xE2\x80\xA6", 3); return semi + 1; }
    return 0;
}


// Emit text fragment as Telegram HTML body text:
// - decode common entities
// - escape <, >, & for parse_mode=HTML
// - normalize stray [[ / ]] / | residue from MediaWiki noise
static int vtl_emit_html_text(vtl_tg_buf* b, const char* s, size_t n)
{
    size_t i = 0;
    while (i < n) {
        unsigned char c = (unsigned char)s[i];
        if (c == '&') {
            size_t skip = vtl_decode_entity(s + i, n - i, b);
            if (skip) { i += skip; continue; }
            if (vtl_tg_buf_append(b, "&amp;", 5)) return -1; ++i; continue;
        }
        if (c == '<') {
            if (vtl_tg_buf_append(b, "&lt;", 4)) return -1; ++i; continue;
        }
        if (c == '>') {
            if (vtl_tg_buf_append(b, "&gt;", 4)) return -1; ++i; continue;
        }
        // Strip wiki residue that wasn't matched as a marker (mostly inside surfaced display text).
        if (c == '[' && i + 1 < n && s[i + 1] == '[') { i += 2; continue; }
        if (c == ']' && i + 1 < n && s[i + 1] == ']') { i += 2; continue; }
        if (vtl_tg_buf_append(b, (const char*)&c, 1)) return -1;
        ++i;
    }
    return 0;
}

// Escape a raw URL for safe inclusion in an HTML href="..." attribute.
// Drops control characters and quotes; encodes & < > as entities.
static int vtl_emit_html_href(vtl_tg_buf* b, const char* s, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x20 || c == 0x7F) continue;
        if (c == '"' || c == '\'') continue;
        if (c == '&') { if (vtl_tg_buf_append(b, "&amp;", 5)) return -1; continue; }
        if (c == '<') { if (vtl_tg_buf_append(b, "&lt;", 4))  return -1; continue; }
        if (c == '>') { if (vtl_tg_buf_append(b, "&gt;", 4))  return -1; continue; }
        if (vtl_tg_buf_append(b, (const char*)&c, 1)) return -1;
    }
    return 0;
}

static VTL_AppResult vtl_text_alloc_from_marked_html(VTL_publication_Text** pp_out,
                                                     const VTL_publication_MarkedText* p_src)
{
    if (!pp_out || !p_src) return VTL_res_kErr;
    vtl_tg_buf b = {0};
    for (size_t i = 0; i < p_src->length; ++i) {
        const VTL_publication_marked_text_Part* p = &p_src->parts[i];
        if (!p->text || p->length == 0) continue;
        // Hyperlink part: emit as Telegram <a href="URL">ref</a>; ignore other flags.
        if (p->type & VTL_TEXT_MODIFICATION_LINK_URL) {
            if (vtl_tg_buf_append_str(&b, "<a href=\"")) goto oom;
            if (vtl_emit_html_href(&b, (const char*)p->text, p->length)) goto oom;
            if (vtl_tg_buf_append_str(&b, "\">ref</a>")) goto oom;
            continue;
        }
        int bold   = (p->type & VTL_TEXT_MODIFICATION_BOLD)          != 0;
        int italic = (p->type & VTL_TEXT_MODIFICATION_ITALIC)        != 0;
        int strike = (p->type & VTL_TEXT_MODIFICATION_STRIKETHROUGH) != 0;
        if (bold   && vtl_tg_buf_append_str(&b, "<b>")) goto oom;
        if (italic && vtl_tg_buf_append_str(&b, "<i>")) goto oom;
        if (strike && vtl_tg_buf_append_str(&b, "<s>")) goto oom;
        if (vtl_emit_html_text(&b, (const char*)p->text, p->length)) goto oom;
        if (strike && vtl_tg_buf_append_str(&b, "</s>")) goto oom;
        if (italic && vtl_tg_buf_append_str(&b, "</i>")) goto oom;
        if (bold   && vtl_tg_buf_append_str(&b, "</b>")) goto oom;
    }
    if (!b.data) {
        b.data = (char*)malloc(1);
        if (!b.data) return VTL_res_kErr;
        b.data[0] = '\0';
    }
    *pp_out = (VTL_publication_Text*)calloc(1, sizeof(VTL_publication_Text));
    if (!*pp_out) { free(b.data); return VTL_res_kErr; }
    (*pp_out)->text = (VTL_publication_text_Symbol*)b.data;
    (*pp_out)->length = b.len;
    return VTL_res_kOk;
oom:
    free(b.data);
    return VTL_res_kErr;
}

static VTL_AppResult vtl_marked_text_alloc_single_part(VTL_publication_MarkedText** pp_out,
                                                        const VTL_publication_Text* p_src,
                                                        VTL_publication_text_modification_Flags type)
{
    if (!pp_out || !p_src) return VTL_res_kErr;
    *pp_out = (VTL_publication_MarkedText*)calloc(1, sizeof(VTL_publication_MarkedText));
    if (!*pp_out) return VTL_res_kErr;
    (*pp_out)->parts = (VTL_publication_marked_text_Part*)calloc(1, sizeof(VTL_publication_marked_text_Part));
    if (!(*pp_out)->parts) { free(*pp_out); return VTL_res_kErr; }
    (*pp_out)->length = 1;
    (*pp_out)->parts[0].text = p_src->text;
    (*pp_out)->parts[0].length = p_src->length;
    (*pp_out)->parts[0].type = type;
    return VTL_res_kOk;
}

static VTL_AppResult vtl_text_alloc_from_marked(VTL_publication_Text** pp_out,
                                                 const VTL_publication_MarkedText* p_src)
{
    if (!pp_out || !p_src) return VTL_res_kErr;
    size_t total = 0;
    for (size_t i = 0; i < p_src->length; ++i) total += p_src->parts[i].length;
    *pp_out = (VTL_publication_Text*)calloc(1, sizeof(VTL_publication_Text));
    if (!*pp_out) return VTL_res_kErr;
    (*pp_out)->text = (VTL_publication_text_Symbol*)malloc(total + 1);
    if (!(*pp_out)->text) { free(*pp_out); return VTL_res_kErr; }
    size_t pos = 0;
    for (size_t i = 0; i < p_src->length; ++i) {
        memcpy((*pp_out)->text + pos, p_src->parts[i].text, p_src->parts[i].length);
        pos += p_src->parts[i].length;
    }
    (*pp_out)->text[total] = 0;
    (*pp_out)->length = total;
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_InitFromStandartMD(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{
    // парсим Standart Markdown (CommonMark + GFM ~~strike~~) в параллельном режиме —
    // три сканера (bold/italic/strike) бегут каждый в своём потоке со своим
    // буфером маркеров, без локов. Sequential доступен отдельно через VTL_markdown_*.
    return VTL_markdown_ParseTextParallel(p_src_text, pp_marked_text);
}

VTL_AppResult VTL_publication_marked_text_InitFromTelegramMD(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{
    // парсим Telegram MarkdownV2 в параллельном режиме —
    // у каждого из трёх сканеров (bold/italic/strike) свой буфер маркеров,
    // что даёт ускорение на больших текстах. Sequential доступен отдельно.
    return VTL_telegram_ParseTextParallel(p_src_text, pp_marked_text);
}

VTL_AppResult VTL_publication_marked_text_InitFromHTML(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{
    // парсим HTML в параллельном режиме — три сканера (bold/italic/strike)
    // на семейства тегов <b>/<strong>, <i>/<em>, <s>/<del> бегут каждый
    // в своём потоке со своим буфером маркеров, без локов.
    // Sequential доступен отдельно через VTL_html_*.
    return VTL_html_ParseTextParallel(p_src_text, pp_marked_text);
}

VTL_AppResult VTL_publication_marked_text_InitFromBB(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{ return vtl_marked_text_alloc_single_part(pp_marked_text, p_src_text, 0); }

VTL_AppResult VTL_publication_marked_text_InitFromRegularText(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{ return vtl_marked_text_alloc_single_part(pp_marked_text, p_src_text, 0); }


VTL_AppResult VTL_publication_marked_text_InitFromMediaWiki(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{
    if (!pp_marked_text || !p_src_text) return VTL_res_kInvalidParamErr;
    return VTL_mediawiki_ParseTextParallel(p_src_text, pp_marked_text);
}

VTL_AppResult VTL_publication_marked_text_Init(
    VTL_publication_MarkedText** pp_marked_text,
    const VTL_publication_Text* p_src_text,
    const VTL_publication_marked_text_MarkupType src_markup_type)
{
    switch(src_markup_type) {
        case VTL_markup_type_kStandartMD:
            return VTL_publication_marked_text_InitFromStandartMD(pp_marked_text, p_src_text);
        case VTL_markup_type_kTelegramMD:
            return VTL_publication_marked_text_InitFromTelegramMD(pp_marked_text, p_src_text);
        case VTL_markup_type_kHTML:
            return VTL_publication_marked_text_InitFromHTML(pp_marked_text, p_src_text);
        case VTL_markup_type_kBB:
            return VTL_publication_marked_text_InitFromBB(pp_marked_text, p_src_text);
        case VTL_markup_type_kMediaWiki:
            return VTL_publication_marked_text_InitFromMediaWiki(pp_marked_text, p_src_text);
        default:
            return VTL_publication_marked_text_InitFromRegularText(pp_marked_text, p_src_text);
    }
}

VTL_AppResult VTL_publication_marked_text_TransformToStandartMD(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{
    // сериализация выдаёт канонический CommonMark-синтаксис:
    // ** для bold, * для italic, ~~ для strike (GFM).
    // Линейная по символам, расспараллеливать смысла нет — упирается в memcpy.
    return VTL_markdown_SerializeText(p_marked_text, pp_text);
}

VTL_AppResult VTL_publication_marked_text_TransformToTelegramMD(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{
    // сериализация — линейная по символам, расспараллеливать смысла нет
    // (упрётся в memcpy и пропускную способность памяти)
    return VTL_telegram_SerializeText(p_marked_text, pp_text);
}

VTL_AppResult VTL_publication_marked_text_TransformToHTML(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{
    // сериализация выдаёт канонические короткие теги: <b>, <i>, <s>.
    // Спецсимволы '<' '>' '&' экранируются HTML-entities.
    // Линейная по символам, расспараллеливать смысла нет — упирается в memcpy.
    return VTL_html_SerializeText(p_marked_text, pp_text);
}

VTL_AppResult VTL_publication_marked_text_TransformToBB(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{ return vtl_text_alloc_from_marked(pp_text, p_marked_text); }


VTL_AppResult VTL_publication_marked_text_TransformToMediaWiki(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{ return VTL_mediawiki_EmitFromMarkedText(p_marked_text, pp_text); }

VTL_AppResult VTL_publication_marked_text_TransformToRegularText(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{ return vtl_text_alloc_from_marked(pp_text, p_marked_text); }
