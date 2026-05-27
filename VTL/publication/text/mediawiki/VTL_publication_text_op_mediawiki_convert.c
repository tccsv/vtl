#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_convert.h>
#include <VTL/VTL_publication_markup_text_flags.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>


static int VTL_mediawiki_marker_cmp(const void* a, const void* b)
{
    const VTL_mediawiki_Marker* ma = (const VTL_mediawiki_Marker*)a;
    const VTL_mediawiki_Marker* mb = (const VTL_mediawiki_Marker*)b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return  1;
    if (ma->kind < mb->kind) return -1;
    if (ma->kind > mb->kind) return  1;
    return 0;
}

VTL_AppResult VTL_mediawiki_SortMarkers(VTL_mediawiki_MarkerList* list)
{
    if (!list) return VTL_res_kInvalidParamErr;
    if (list->length > 1) {
        qsort(list->items, list->length, sizeof(VTL_mediawiki_Marker),
              VTL_mediawiki_marker_cmp);
    }
    return VTL_res_kOk;
}

VTL_AppResult VTL_mediawiki_MergeMarkers(const VTL_mediawiki_MarkerList* parts,
                                          size_t parts_count,
                                          VTL_mediawiki_MarkerList* out)
{
    if (!parts || !out) return VTL_res_kInvalidParamErr;


    size_t total = 0;
    for (size_t i = 0; i < parts_count; ++i) total += parts[i].length;

    VTL_AppResult res = VTL_mediawiki_MarkerListReserve(out, total);
    if (res != VTL_res_kOk) return res;

    for (size_t i = 0; i < parts_count; ++i) {
        for (size_t j = 0; j < parts[i].length; ++j) {
            res = VTL_mediawiki_MarkerListPush(out, parts[i].items[j]);
            if (res != VTL_res_kOk) return res;
        }
    }
    return VTL_mediawiki_SortMarkers(out);
}


static VTL_publication_marked_text_Part VTL_mediawiki_make_part(const char* text_start,
    size_t length, VTL_publication_text_modification_Flags flags)
{
    VTL_publication_marked_text_Part p;
    p.text = (VTL_publication_text_Symbol*)text_start;
    p.length = length;
    p.type = flags;
    return p;
}


// Span resolver for non-paired markers (links, refs, headings, generic tags, etc.).
// Outputs the inner-text byte range to emit and which modification flag to OR on top
// of the running flags, plus optional prefix/suffix literals.
typedef struct _VTL_mediawiki_span_info
{
    size_t inner_off;
    size_t inner_len;
    VTL_publication_text_modification_Flags add_flag;
    const char* prefix;
    const char* suffix;
    bool drop_inner;
} VTL_mediawiki_span_info;

static void VTL_mediawiki_resolve_span(const char* text,
                                       const VTL_mediawiki_Marker* m,
                                       VTL_mediawiki_span_info* out)
{
    out->inner_off = 0;
    out->inner_len = 0;
    out->add_flag = 0;
    out->prefix = NULL;
    out->suffix = NULL;
    out->drop_inner = true;

    size_t L = m->length;
    size_t base = m->pos;

    switch (m->kind) {
        case VTL_mediawiki_marker_kInternalLink: {
            // [[Target]] / [[Target|Display]] / [[File:foo|caption]]
            if (L < 4) return;
            size_t inner_off = base + 2;
            size_t inner_len = L - 4;
            // Drop image/file/category links — captions are usually noise in text mode.
            static const char* drop_ns[] = { "File:", "Image:", "Category:", "Wikipedia:" };
            for (size_t k = 0; k < sizeof(drop_ns) / sizeof(*drop_ns); ++k) {
                size_t nlen = strlen(drop_ns[k]);
                if (inner_len >= nlen && memcmp(text + inner_off, drop_ns[k], nlen) == 0) return;
            }
            size_t pipe = (size_t)-1;
            for (size_t i = 0; i < inner_len; ++i) {
                if (text[inner_off + i] == '|') pipe = i;
            }
            if (pipe != (size_t)-1) {
                inner_off += pipe + 1;
                inner_len -= pipe + 1;
            } else {
                // Skip namespaced links without a display label.
                for (size_t i = 0; i < inner_len; ++i) {
                    if (text[inner_off + i] == ':') return;
                }
            }
            if (inner_len == 0) return;
            out->inner_off = inner_off;
            out->inner_len = inner_len;
            out->add_flag = VTL_TEXT_MODIFICATION_BOLD;
            out->drop_inner = false;
            break;
        }
        case VTL_mediawiki_marker_kExternalLink: {
            // [http://url display text]; bold the display portion.
            if (L < 3) return;
            size_t inner_off = base + 1;
            size_t inner_len = L - 2;
            size_t sp = (size_t)-1;
            for (size_t i = 0; i < inner_len; ++i) {
                if (text[inner_off + i] == ' ') { sp = i; break; }
            }
            if (sp == (size_t)-1 || sp + 1 >= inner_len) return;
            size_t disp_off = inner_off + sp + 1;
            size_t disp_len = inner_len - sp - 1;
            // Skip displays that are just template/markup junk.
            if (disp_len >= 2 && text[disp_off] == '{' && text[disp_off + 1] == '{') return;
            if (disp_len >= 1 && text[disp_off] == '<') return;
            out->inner_off = disp_off;
            out->inner_len = disp_len;
            out->add_flag = VTL_TEXT_MODIFICATION_BOLD;
            out->drop_inner = false;
            break;
        }
        case VTL_mediawiki_marker_kRef: {
            // <ref ...>content</ref> or <ref ... />. Self-closing: drop.
            if (L < 6) return;
            if (text[base + L - 2] == '/') return;
            size_t open_end = (size_t)-1;
            for (size_t i = 4; i < L; ++i) {
                if (text[base + i] == '>') { open_end = i; break; }
            }
            if (open_end == (size_t)-1) return;
            if (L < open_end + 1 + 6) return;
            size_t close_off = L - 6;
            if (close_off <= open_end + 1) return;
            size_t inner_off = base + open_end + 1;
            size_t inner_len = close_off - open_end - 1;
            while (inner_len > 0 && (text[inner_off] == ' ' || text[inner_off] == '\n')) {
                inner_off++; inner_len--;
            }
            if (inner_len == 0) return;
            // Try to extract an http(s) URL from the ref body (works for both
            // {{cite ...|url=...}} templates and bare-URL refs). If found, emit
            // it as an anchor so it becomes a clickable link in the renderer.
            size_t url_off = 0, url_len = 0;
            for (size_t i = 0; i + 7 <= inner_len; ++i) {
                const char* q = text + inner_off + i;
                int is_https = (i + 8 <= inner_len) && memcmp(q, "https://", 8) == 0;
                int is_http  = !is_https && memcmp(q, "http://", 7) == 0;
                if (!is_http && !is_https) continue;
                size_t s = inner_off + i;
                size_t e = s;
                while (e < inner_off + inner_len) {
                    char c = text[e];
                    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
                        c == '|' || c == '}'  || c == '<'  || c == ']'  ||
                        c == '"' || c == '\'' || c == ')') break;
                    ++e;
                }
                if (e > s + 8) { url_off = s; url_len = e - s; }
                break;
            }
            if (url_len > 0) {
                out->inner_off = url_off;
                out->inner_len = url_len;
                out->add_flag = VTL_TEXT_MODIFICATION_LINK_URL;
                out->prefix = " [";
                out->suffix = "]";
                out->drop_inner = false;
                break;
            }
            // Pure citation template without a discoverable URL -> compact placeholder.
            if (inner_len >= 2 && text[inner_off] == '{' && text[inner_off + 1] == '{') {
                out->prefix = " [ref]";
                return;
            }
            out->inner_off = inner_off;
            out->inner_len = inner_len;
            out->add_flag = VTL_TEXT_MODIFICATION_ITALIC;
            out->prefix = " [";
            out->suffix = "]";
            out->drop_inner = false;
            break;
        }
        case VTL_mediawiki_marker_kHeading: {
            int lvl = m->level;
            if (lvl <= 0 || L < (size_t)(2 * lvl)) return;
            size_t inner_off = base + lvl;
            size_t inner_len = L - 2 * lvl;
            while (inner_len > 0 && text[inner_off] == ' ') { inner_off++; inner_len--; }
            while (inner_len > 0 && text[inner_off + inner_len - 1] == ' ') inner_len--;
            if (inner_len == 0) return;
            out->inner_off = inner_off;
            out->inner_len = inner_len;
            out->add_flag = VTL_TEXT_MODIFICATION_BOLD;
            out->prefix = "\n";
            out->suffix = "\n";
            out->drop_inner = false;
            break;
        }
        case VTL_mediawiki_marker_kListItem:
            // Drop the bullet prefix; the rest of the line stays as regular text.
            out->prefix = "• ";
            break;
        case VTL_mediawiki_marker_kHorizontalRule:
            out->prefix = "\n— — —\n";
            break;
        case VTL_mediawiki_marker_kGenericTag: {
            // level = opening-tag length, aux = closing-tag length (0 if self-closing).
            size_t open_len = (size_t)m->level;
            size_t close_len = (size_t)m->aux;
            if (close_len == 0 || open_len == 0 || open_len + close_len >= L) return;
            out->inner_off = base + open_len;
            out->inner_len = L - open_len - close_len;
            if (out->inner_len == 0) return;
            out->add_flag = VTL_TEXT_MODIFICATION_BOLD;
            out->drop_inner = false;
            break;
        }
        case VTL_mediawiki_marker_kTemplate:
        case VTL_mediawiki_marker_kComment:
        case VTL_mediawiki_marker_kNowikiStart:
        case VTL_mediawiki_marker_kNowikiEnd:
        default:
            break;
    }
}

VTL_AppResult VTL_mediawiki_ConvertToMarkedText(const VTL_publication_Text* src,
                                                 const VTL_mediawiki_MarkerList* markers,
                                                 VTL_publication_MarkedText** out)
{
    if (!src || !src->text || !markers || !out) return VTL_res_kInvalidParamErr;

    VTL_publication_MarkedText* block =
        (VTL_publication_MarkedText*)malloc(sizeof(VTL_publication_MarkedText));
    if (!block) return VTL_res_kMemAllocErr;


    /* Защита от integer overflow при подсчёте max_parts: при `markers->length`
     * близком к SIZE_MAX/4 умножение завернётся, malloc вернёт маленький блок
     * и последующая запись уйдёт за границу. */
    if (markers->length > (SIZE_MAX - 2) / 4
        || (markers->length * 4 + 2) > SIZE_MAX / sizeof(VTL_publication_marked_text_Part)) {
        free(block);
        return VTL_res_kMemAllocErr;
    }
    size_t max_parts = markers->length * 4 + 2;
    block->parts = (VTL_publication_marked_text_Part*)malloc(
        max_parts * sizeof(VTL_publication_marked_text_Part));
    if (!block->parts) { free(block); return VTL_res_kMemAllocErr; }
    block->length = 0;

    VTL_publication_text_modification_Flags flags = 0;
    size_t cursor = 0;

    for (size_t i = 0; i < markers->length; ++i) {
        const VTL_mediawiki_Marker* m = &markers->items[i];

        if (m->pos < cursor) continue;

        if (m->pos > cursor) {
            block->parts[block->length++] = VTL_mediawiki_make_part(
                src->text + cursor, m->pos - cursor, flags);
        }

        switch (m->kind) {
            case VTL_mediawiki_marker_kBoldStart:    flags |=  VTL_TEXT_MODIFICATION_BOLD;          break;
            case VTL_mediawiki_marker_kBoldEnd:      flags &= ~VTL_TEXT_MODIFICATION_BOLD;          break;
            case VTL_mediawiki_marker_kItalicStart:  flags |=  VTL_TEXT_MODIFICATION_ITALIC;        break;
            case VTL_mediawiki_marker_kItalicEnd:    flags &= ~VTL_TEXT_MODIFICATION_ITALIC;        break;
            case VTL_mediawiki_marker_kStrikeStart:  flags |=  VTL_TEXT_MODIFICATION_STRIKETHROUGH; break;
            case VTL_mediawiki_marker_kStrikeEnd:    flags &= ~VTL_TEXT_MODIFICATION_STRIKETHROUGH; break;

            default: {
                VTL_mediawiki_span_info info;
                VTL_mediawiki_resolve_span(src->text, m, &info);
                if (info.prefix) {
                    block->parts[block->length++] = VTL_mediawiki_make_part(
                        info.prefix, strlen(info.prefix), flags);
                }
                if (!info.drop_inner && info.inner_len > 0) {
                    block->parts[block->length++] = VTL_mediawiki_make_part(
                        src->text + info.inner_off, info.inner_len,
                        flags | info.add_flag);
                }
                if (info.suffix) {
                    block->parts[block->length++] = VTL_mediawiki_make_part(
                        info.suffix, strlen(info.suffix), flags);
                }
                break;
            }
        }

        cursor = m->pos + m->length;
    }

    if (cursor < src->length) {
        block->parts[block->length++] = VTL_mediawiki_make_part(
            src->text + cursor, src->length - cursor, flags);
    }

    *out = block;
    return VTL_res_kOk;
}


typedef struct _VTL_mediawiki_emit_marker
{
    int flag;
    const char* open;
    const char* close;
} VTL_mediawiki_emit_marker;

static const VTL_mediawiki_emit_marker VTL_mediawiki_emit_markers[] = {
    { VTL_TEXT_MODIFICATION_BOLD,          "'''", "'''" },
    { VTL_TEXT_MODIFICATION_ITALIC,        "''",  "''"  },
    { VTL_TEXT_MODIFICATION_STRIKETHROUGH, "<s>", "</s>" }
};
#define VTL_MEDIAWIKI_EMIT_MARKERS_COUNT \
    (sizeof(VTL_mediawiki_emit_markers) / sizeof(VTL_mediawiki_emit_markers[0]))


typedef struct _VTL_mediawiki_string_builder
{
    char* data;
    size_t length;
    size_t capacity;
} VTL_mediawiki_string_builder;

static VTL_AppResult VTL_mediawiki_sb_reserve(VTL_mediawiki_string_builder* sb, size_t need)
{
    if (sb->capacity >= need) return VTL_res_kOk;
    size_t new_cap = sb->capacity ? sb->capacity : 64;
    while (new_cap < need) new_cap *= 2;
    char* new_data = (char*)realloc(sb->data, new_cap);
    if (!new_data) return VTL_res_kMemAllocErr;
    sb->data = new_data;
    sb->capacity = new_cap;
    return VTL_res_kOk;
}

static VTL_AppResult VTL_mediawiki_sb_append(VTL_mediawiki_string_builder* sb,
                                              const char* data, size_t len)
{
    VTL_AppResult res = VTL_mediawiki_sb_reserve(sb, sb->length + len + 1);
    if (res != VTL_res_kOk) return res;
    memcpy(sb->data + sb->length, data, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
    return VTL_res_kOk;
}

static VTL_AppResult VTL_mediawiki_sb_append_str(VTL_mediawiki_string_builder* sb,
                                                  const char* s)
{
    return VTL_mediawiki_sb_append(sb, s, strlen(s));
}

VTL_AppResult VTL_mediawiki_EmitFromMarkedText(const VTL_publication_MarkedText* src,
                                                VTL_publication_Text** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_mediawiki_string_builder sb = { NULL, 0, 0 };

    for (size_t i = 0; i < src->length; ++i) {
        const VTL_publication_marked_text_Part* p = &src->parts[i];
        if (!p->text || p->length == 0) continue;


        for (size_t k = 0; k < VTL_MEDIAWIKI_EMIT_MARKERS_COUNT; ++k) {
            if (p->type & VTL_mediawiki_emit_markers[k].flag) {
                if (VTL_mediawiki_sb_append_str(&sb, VTL_mediawiki_emit_markers[k].open) != VTL_res_kOk) {
                    free(sb.data); return VTL_res_kMemAllocErr;
                }
            }
        }

        if (VTL_mediawiki_sb_append(&sb, p->text, p->length) != VTL_res_kOk) {
            free(sb.data); return VTL_res_kMemAllocErr;
        }


        for (size_t k = VTL_MEDIAWIKI_EMIT_MARKERS_COUNT; k-- > 0; ) {
            if (p->type & VTL_mediawiki_emit_markers[k].flag) {
                if (VTL_mediawiki_sb_append_str(&sb, VTL_mediawiki_emit_markers[k].close) != VTL_res_kOk) {
                    free(sb.data); return VTL_res_kMemAllocErr;
                }
            }
        }
    }

    VTL_publication_Text* result = (VTL_publication_Text*)malloc(sizeof(VTL_publication_Text));
    if (!result) { free(sb.data); return VTL_res_kMemAllocErr; }

    if (!sb.data) {
        sb.data = (char*)malloc(1);
        if (!sb.data) { free(result); return VTL_res_kMemAllocErr; }
        sb.data[0] = '\0';
    }
    result->text = sb.data;
    result->length = sb.length;
    *out = result;
    return VTL_res_kOk;
}
