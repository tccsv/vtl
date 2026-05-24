#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/discord/VTL_publication_text_op_discord.h>
#include <VTL/publication/text/discord/VTL_publication_text_op_discord_compat.h>
#include <VTL/publication/text/discord/VTL_publication_text_op_discord_convert.h>
#include <stdlib.h>
#include <string.h>

static VTL_AppResult VTL_discord_ListPush(VTL_discord_MarkerList *l, VTL_discord_Marker m) {
    if (l->length == l->capacity) {
        size_t cap = l->capacity ? l->capacity * 2 : 32;
        VTL_discord_Marker *p = (VTL_discord_Marker *) realloc(l->items, cap * sizeof(*p));
        if (!p) return VTL_res_kMemAllocErr;
        l->items = p;
        l->capacity = cap;
    }
    l->items[l->length++] = m;
    return VTL_res_kOk;
}

static void VTL_discord_ListFree(VTL_discord_MarkerList *l) {
    free(l->items);
    l->items = NULL;
    l->length = l->capacity = 0;
}

static void *VTL_discord_ScanPair(void *arg, char sym, size_t token_len, VTL_discord_MarkerKind sk, VTL_discord_MarkerKind ek) {
    VTL_discord_ScanContext *ctx = (VTL_discord_ScanContext *) arg;
    const char *t = ctx->text;
    size_t len = ctx->text_length;
    int open = 0;

    for (size_t i = 0; i < len;) {
        if ((unsigned char) t[i] != (unsigned char) sym) {
            ++i;
            continue;
        }
        size_t run = 0;
        while (i + run < len && t[i + run] == sym) ++run;
        if (run != token_len) {
            i += run;
            continue;
        }

        char prev = i > 0 ? t[i - 1] : '\n';
        char next = i + run < len ? t[i + token_len] : '\n';

        if (!open && next != ' ' && next != '\t' && next != '\n' && next != '\0') {
            VTL_discord_Marker m = {sk, i, token_len};
            if (VTL_discord_ListPush(ctx->out, m)) return (void *) 1;
            open = 1;
        } else if (open && prev != ' ' && prev != '\t' && prev != '\n') {
            VTL_discord_Marker m = {ek, i, token_len};
            if (VTL_discord_ListPush(ctx->out, m)) return (void *) 1;
            open = 0;
        }
        i += run;
    }
    return NULL;
}

static int VTL_discord_IsWordChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static void *VTL_discord_ScanUnderscore(void *arg) {
    VTL_discord_ScanContext *ctx = (VTL_discord_ScanContext *) arg;
    const char *t = ctx->text;
    size_t len = ctx->text_length;
    int open = 0;

    for (size_t i = 0; i < len; ++i) {
        if (t[i] != '_') continue;
        if (i + 1 < len && t[i + 1] == '_') {
            ++i;
            continue;
        }

        char prev = i > 0 ? t[i - 1] : '\n';
        char next = i + 1 < len ? t[i + 1] : '\n';

        if (!open && !VTL_discord_IsWordChar(prev) &&
            next != ' ' && next != '\t' && next != '\n' && next != '\0') {
            VTL_discord_Marker m = {VTL_discord_marker_kItalicStart, i, 1};
            if (VTL_discord_ListPush(ctx->out, m)) return (void *) 1;
            open = 1;
        } else if (open && prev != ' ' && prev != '\t' && prev != '\n' &&
                   !VTL_discord_IsWordChar(next)) {
            VTL_discord_Marker m = {VTL_discord_marker_kItalicEnd, i, 1};
            if (VTL_discord_ListPush(ctx->out, m)) return (void *) 1;
            open = 0;
        }
    }
    return NULL;
}

static void *VTL_discord_ScanBoldItalic(void *a) {
    return VTL_discord_ScanPair(a, '*', 3, VTL_discord_marker_kBoldItalicStart, VTL_discord_marker_kBoldItalicEnd);
}

static void *VTL_discord_ScanBold(void *a) {
    return VTL_discord_ScanPair(a, '*', 2, VTL_discord_marker_kBoldStart, VTL_discord_marker_kBoldEnd);
}

static void *VTL_discord_ScanItalicStar(void *a) {
    return VTL_discord_ScanPair(a, '*', 1, VTL_discord_marker_kItalicStart, VTL_discord_marker_kItalicEnd);
}

static void *VTL_discord_ScanStrike(void *a) {
    return VTL_discord_ScanPair(a, '~', 2, VTL_discord_marker_kStrikeStart, VTL_discord_marker_kStrikeEnd);
}

static void *(*VTL_discord_scanners[VTL_DISCORD_SCANNER_COUNT])(void *) = {
        VTL_discord_ScanBoldItalic,
        VTL_discord_ScanBold,
        VTL_discord_ScanItalicStar,
        VTL_discord_ScanUnderscore,
        VTL_discord_ScanStrike,
};

static int VTL_discord_MarkerCmp(const void *a, const void *b) {
    const VTL_discord_Marker *ma = (const VTL_discord_Marker *) a;
    const VTL_discord_Marker *mb = (const VTL_discord_Marker *) b;
    if (ma->pos != mb->pos) {
        return ma->pos < mb->pos ? -1 : 1;
    }
    return (int) ma->kind - (int) mb->kind;
}

static VTL_AppResult VTL_discord_Merge(VTL_discord_MarkerList *parts, size_t n, VTL_discord_MarkerList *out) {
    size_t total = 0;
    for (size_t i = 0; i < n; ++i) {
        total += parts[i].length;
    }

    VTL_discord_Marker *buf = (VTL_discord_Marker *) malloc(total * sizeof(*buf));
    if (!buf && total) {
        return VTL_res_kMemAllocErr;
    }

    size_t pos = 0;
    for (size_t i = 0; i < n; ++i) {
        memcpy(buf + pos, parts[i].items, parts[i].length * sizeof(*buf));
        pos += parts[i].length;
    }
    if (total > 1) qsort(buf, total, sizeof(*buf), VTL_discord_MarkerCmp);

    out->items = buf;
    out->length = total;
    out->capacity = total;
    return VTL_res_kOk;
}

static VTL_AppResult VTL_discord_RunScanners(const VTL_publication_Text *src, VTL_discord_MarkerList *out, int parallel) {
    VTL_discord_MarkerList parts[VTL_DISCORD_SCANNER_COUNT];
    VTL_discord_ScanContext ctxs[VTL_DISCORD_SCANNER_COUNT];
    vtl_thread_t thr[VTL_DISCORD_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
        parts[i] = (VTL_discord_MarkerList) {NULL, 0, 0};
        ctxs[i] = (VTL_discord_ScanContext) {src->text, src->length, &parts[i]};
    }

    if (parallel) {
        size_t spawned = 0;
        for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
            if (vtl_thread_create(&thr[i], VTL_discord_scanners[i], &ctxs[i]) != 0) break;
            ++spawned;
        }
        for (size_t i = spawned; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
            VTL_discord_scanners[i](&ctxs[i]);
        }
        for (size_t i = 0; i < spawned; ++i) {
            vtl_thread_join(thr[i]);
        }
    } else {
        for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
            VTL_discord_scanners[i](&ctxs[i]);
        }
    }

    VTL_AppResult res = VTL_discord_Merge(parts, VTL_DISCORD_SCANNER_COUNT, out);
    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
        VTL_discord_ListFree(&parts[i]);
    }
    return res;
}

VTL_AppResult VTL_discord_ParseTextSequential(const VTL_publication_Text *src, VTL_publication_MarkedText **out) {
    if (!src || !out) {
        return VTL_res_kInvalidParamErr;
    }
    VTL_discord_MarkerList markers = {NULL, 0, 0};
    VTL_AppResult res = VTL_discord_RunScanners(src, &markers, 0);
    if (res == VTL_res_kOk) {
        res = VTL_discord_ConvertToMarkedText(src, &markers, out);
    }
    free(markers.items);
    return res;
}

VTL_AppResult VTL_discord_ParseTextParallel(const VTL_publication_Text *src, VTL_publication_MarkedText **out) {
    if (!src || !out) {
        return VTL_res_kInvalidParamErr;
    }
    VTL_discord_MarkerList markers = {NULL, 0, 0};
    VTL_AppResult res = VTL_discord_RunScanners(src, &markers, 1);
    if (res == VTL_res_kOk) {
        res = VTL_discord_ConvertToMarkedText(src, &markers, out);
    }
    free(markers.items);
    return res;
}

VTL_AppResult VTL_discord_SerializeToText(const VTL_publication_MarkedText *src, VTL_publication_Text **out) {
    return VTL_discord_ConvertFromMarkedText(src, out);
}
