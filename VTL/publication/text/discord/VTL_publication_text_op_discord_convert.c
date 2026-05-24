#include <VTL/publication/text/discord/VTL_publication_text_op_discord_convert.h>
#include <stdlib.h>
#include <string.h>

static VTL_publication_text_modification_Flags VTL_discord_ApplyMarker(VTL_discord_MarkerKind kind, VTL_publication_text_modification_Flags cur) {
    switch (kind) {
        case VTL_discord_marker_kBoldStart:
            return cur | VTL_TEXT_MODIFICATION_BOLD;
        case VTL_discord_marker_kBoldEnd:
            return cur & ~VTL_TEXT_MODIFICATION_BOLD;
        case VTL_discord_marker_kItalicStart:
            return cur | VTL_TEXT_MODIFICATION_ITALIC;
        case VTL_discord_marker_kItalicEnd:
            return cur & ~VTL_TEXT_MODIFICATION_ITALIC;
        case VTL_discord_marker_kStrikeStart:
            return cur | VTL_TEXT_MODIFICATION_STRIKETHROUGH;
        case VTL_discord_marker_kStrikeEnd:
            return cur & ~VTL_TEXT_MODIFICATION_STRIKETHROUGH;
        case VTL_discord_marker_kBoldItalicStart:
            return cur | (VTL_TEXT_MODIFICATION_BOLD | VTL_TEXT_MODIFICATION_ITALIC);
        case VTL_discord_marker_kBoldItalicEnd:
            return cur & ~(VTL_TEXT_MODIFICATION_BOLD | VTL_TEXT_MODIFICATION_ITALIC);
        default:
            return cur;
    }
}

VTL_AppResult VTL_discord_ConvertToMarkedText(const VTL_publication_Text *src, const VTL_discord_MarkerList *markers, VTL_publication_MarkedText **out) {
    if (!src || !markers || !out) {
        return VTL_res_kInvalidParamErr;
    }

    size_t max_parts = markers->length + 1;
    VTL_publication_MarkedText *block = (VTL_publication_MarkedText *) malloc(sizeof(*block));
    if (!block) {
        return VTL_res_kMemAllocErr;
    }

    block->parts = (VTL_publication_marked_text_Part *) malloc(max_parts * sizeof(*block->parts));
    if (!block->parts) {
        free(block);
        return VTL_res_kMemAllocErr;
    }
    block->length = 0;

    VTL_publication_text_modification_Flags flags = 0;
    size_t pos = 0;

    for (size_t i = 0; i < markers->length; ++i) {
        const VTL_discord_Marker *m = &markers->items[i];
        if (m->pos > pos) {
            VTL_publication_marked_text_Part *p = &block->parts[block->length++];
            p->text = src->text + pos;
            p->length = m->pos - pos;
            p->type = flags;
        }
        flags = VTL_discord_ApplyMarker(m->kind, flags);
        pos = m->pos + m->length;
    }
    if (pos < src->length) {
        VTL_publication_marked_text_Part *p = &block->parts[block->length++];
        p->text = src->text + pos;
        p->length = src->length - pos;
        p->type = flags;
    }
    /* Текст без разметки */
    if (block->length == 0 && src->length > 0) {
        block->parts[0] = (VTL_publication_marked_text_Part) {src->text, src->length, 0};
        block->length = 1;
    }

    *out = block;
    return VTL_res_kOk;
}

static void VTL_discord_GetTags(VTL_publication_text_modification_Flags f, const char **open, size_t *olen, const char **close, size_t *clen) {
    int b = (f & VTL_TEXT_MODIFICATION_BOLD) != 0;
    int i = (f & VTL_TEXT_MODIFICATION_ITALIC) != 0;
    int s = (f & VTL_TEXT_MODIFICATION_STRIKETHROUGH) != 0;

    if (s && b && i) {
        *open = "~~***";
        *olen = 5;
        *close = "***~~";
        *clen = 5;
        return;
    }
    if (s && b) {
        *open = "~~**";
        *olen = 4;
        *close = "**~~";
        *clen = 4;
        return;
    }
    if (s && i) {
        *open = "~~*";
        *olen = 3;
        *close = "*~~";
        *clen = 3;
        return;
    }
    if (s) {
        *open = "~~";
        *olen = 2;
        *close = "~~";
        *clen = 2;
        return;
    }
    if (b && i) {
        *open = "***";
        *olen = 3;
        *close = "***";
        *clen = 3;
        return;
    }
    if (b) {
        *open = "**";
        *olen = 2;
        *close = "**";
        *clen = 2;
        return;
    }
    if (i) {
        *open = "*";
        *olen = 1;
        *close = "*";
        *clen = 1;
        return;
    }
    *open = "";
    *olen = 0;
    *close = "";
    *clen = 0;
}

VTL_AppResult VTL_discord_ConvertFromMarkedText(const VTL_publication_MarkedText *src, VTL_publication_Text **out) {
    if (!src || !out) {
        return VTL_res_kInvalidParamErr;
    }
    size_t total = 0;
    for (size_t i = 0; i < src->length; ++i) {
        const char *o, *c;
        size_t ol, cl;
        VTL_discord_GetTags(src->parts[i].type, &o, &ol, &c, &cl);
        total += ol + src->parts[i].length + cl;
    }

    VTL_publication_Text *result = (VTL_publication_Text *) malloc(sizeof(*result));
    if (!result) {
        return VTL_res_kMemAllocErr;
    }
    result->text = (VTL_publication_text_Symbol *) malloc(total + 1);
    if (!result->text) {
        free(result);
        return VTL_res_kMemAllocErr;
    }

    char *p = result->text;
    for (size_t i = 0; i < src->length; ++i) {
        const VTL_publication_marked_text_Part *part = &src->parts[i];
        const char *o, *c;
        size_t ol, cl;
        VTL_discord_GetTags(part->type, &o, &ol, &c, &cl);
        memcpy(p, o, ol);
        p += ol;
        memcpy(p, part->text, part->length);
        p += part->length;
        memcpy(p, c, cl);
        p += cl;
    }
    *p = '\0';
    result->length = (size_t)(p - result->text);
    *out = result;
    return VTL_res_kOk;
}
