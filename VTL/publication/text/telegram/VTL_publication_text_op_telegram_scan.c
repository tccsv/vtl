#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_scan.h>
#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_threads.h>

#include <stdbool.h>
#include <stdlib.h>


// сколько типов inline-разметки умеем — столько же потоков в Parallel
#define VTL_TELEGRAM_SCANNER_COUNT 3


// ============================================================
// мелкие хелперы
// ============================================================

// буква/цифра/_ — часть слова. Нужно чтобы отличить *bold* (разметка)
// от 1*2*3 или snake_case (не разметка).
static bool VTL_telegram_is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

// символ экранирован, если перед ним нечётное число обратных слешей.
// Telegram MarkdownV2 требует "\*" для буквенной звёздочки, и т.п.
static bool VTL_telegram_is_escaped(const char* text, size_t pos)
{
    size_t backslashes = 0;
    while (pos > backslashes && text[pos - 1 - backslashes] == '\\') {
        ++backslashes;
    }
    return (backslashes & 1u) != 0;
}


// ============================================================
// общий сканер парного inline-токена (*, _, ~)
// один и тот же символ — и открывающий, и закрывающий
// ============================================================

// возвращает (void*)1 если push в список упал (OOM), иначе NULL
static void* VTL_telegram_scan_inline_pair(VTL_telegram_ScanContext* ctx,
                                            char delim,
                                            VTL_telegram_MarkerKind start_kind,
                                            VTL_telegram_MarkerKind end_kind)
{
    bool open = false;

    for (size_t i = 0; i < ctx->text_length; ++i) {
        if (ctx->text[i] != delim) {
            continue;
        }
        // экранированный символ — буквенный, не разметка
        if (VTL_telegram_is_escaped(ctx->text, i)) {
            continue;
        }

        // если соседа нет — считаем что там \n
        char prev = (i > 0) ? ctx->text[i - 1] : '\n';
        char next = (i + 1 < ctx->text_length) ? ctx->text[i + 1] : '\n';

        if (!open) {
            // открывающий: перед ним не буква/цифра (иначе это середина слова),
            // после — не пробельный символ (иначе это не разметка, а текст)
            bool prev_ok = !VTL_telegram_is_word_char(prev);
            bool next_ok = next != ' ' && next != '\t' &&
                           next != '\n' && next != '\r' && next != '\0';
            if (prev_ok && next_ok) {
                VTL_telegram_Marker m;
                m.kind = start_kind;
                m.pos = i;
                m.length = 1;
                if (VTL_telegram_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = true;
            }
        } else {
            // закрывающий: перед ним не пробельный (иначе разметка не закрывается),
            // после — не буква/цифра (иначе разметка прилипает к следующему слову)
            bool prev_ok = prev != ' ' && prev != '\t' &&
                           prev != '\n' && prev != '\r';
            bool next_ok = !VTL_telegram_is_word_char(next);
            if (prev_ok && next_ok) {
                VTL_telegram_Marker m;
                m.kind = end_kind;
                m.pos = i;
                m.length = 1;
                if (VTL_telegram_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = false;
            }
        }
    }
    return NULL;
}


// ============================================================
// конкретные сканеры — просто параметризуют scan_inline_pair
// ============================================================

void* VTL_telegram_ScanBold(void* arg)
{
    VTL_telegram_ScanContext* ctx = (VTL_telegram_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_telegram_scan_inline_pair(ctx, '*',
        VTL_telegram_marker_kBoldStart, VTL_telegram_marker_kBoldEnd);
}

void* VTL_telegram_ScanItalic(void* arg)
{
    VTL_telegram_ScanContext* ctx = (VTL_telegram_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_telegram_scan_inline_pair(ctx, '_',
        VTL_telegram_marker_kItalicStart, VTL_telegram_marker_kItalicEnd);
}

void* VTL_telegram_ScanStrike(void* arg)
{
    VTL_telegram_ScanContext* ctx = (VTL_telegram_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_telegram_scan_inline_pair(ctx, '~',
        VTL_telegram_marker_kStrikeStart, VTL_telegram_marker_kStrikeEnd);
}


// ============================================================
// слияние частичных списков (только для Parallel) + сортировка
// ============================================================

// qsort comparator: сначала по позиции, при равной — по типу
static int VTL_telegram_marker_cmp(const void* a, const void* b)
{
    const VTL_telegram_Marker* ma = (const VTL_telegram_Marker*)a;
    const VTL_telegram_Marker* mb = (const VTL_telegram_Marker*)b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return  1;
    if (ma->kind < mb->kind) return -1;
    if (ma->kind > mb->kind) return  1;
    return 0;
}

static VTL_AppResult VTL_telegram_sort_markers(VTL_telegram_MarkerList* list)
{
    if (!list) return VTL_res_kInvalidParamErr;
    if (list->length > 1) {
        qsort(list->items, list->length, sizeof(VTL_telegram_Marker),
              VTL_telegram_marker_cmp);
    }
    return VTL_res_kOk;
}

static VTL_AppResult VTL_telegram_merge_markers(const VTL_telegram_MarkerList* parts,
                                                size_t parts_count,
                                                VTL_telegram_MarkerList* out)
{
    if (!parts || !out) return VTL_res_kInvalidParamErr;

    // считаем общий размер заранее — один reserve вместо N realloc'ов
    size_t total = 0;
    for (size_t i = 0; i < parts_count; ++i) {
        total += parts[i].length;
    }
    VTL_AppResult res = VTL_telegram_MarkerListReserve(out, total);
    if (res != VTL_res_kOk) return res;

    for (size_t i = 0; i < parts_count; ++i) {
        for (size_t j = 0; j < parts[i].length; ++j) {
            res = VTL_telegram_MarkerListPush(out, parts[i].items[j]);
            if (res != VTL_res_kOk) return res;
        }
    }
    return VTL_telegram_sort_markers(out);
}


// ============================================================
// оркестрация: Sequential vs Parallel
// ============================================================

typedef void* (*VTL_telegram_scanner_fn)(void*);

// массив всех сканеров — добавляешь новый, дописываешь сюда
static const VTL_telegram_scanner_fn VTL_telegram_all_scanners[VTL_TELEGRAM_SCANNER_COUNT] = {
    VTL_telegram_ScanBold,
    VTL_telegram_ScanItalic,
    VTL_telegram_ScanStrike
};


VTL_AppResult VTL_telegram_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_telegram_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_telegram_ScanContext ctx;
    ctx.text = src->text;
    ctx.text_length = src->length;
    ctx.out = out;

    for (size_t i = 0; i < VTL_TELEGRAM_SCANNER_COUNT; ++i) {
        if (VTL_telegram_all_scanners[i](&ctx) != NULL) {
            return VTL_res_kErr;
        }
    }
    return VTL_telegram_sort_markers(out);
}


VTL_AppResult VTL_telegram_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_telegram_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    // у каждого сканера свой буфер и свой контекст — поэтому без локов
    VTL_telegram_MarkerList partial_lists[VTL_TELEGRAM_SCANNER_COUNT];
    VTL_telegram_ScanContext contexts[VTL_TELEGRAM_SCANNER_COUNT];
    vtl_tg_thread_t threads[VTL_TELEGRAM_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_TELEGRAM_SCANNER_COUNT; ++i) {
        VTL_telegram_MarkerListInit(&partial_lists[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial_lists[i];
    }

    // если создание потока упадёт на полпути — оставшиеся гоним последовательно
    size_t spawned = 0;
    for (size_t i = 0; i < VTL_TELEGRAM_SCANNER_COUNT; ++i) {
        if (vtl_tg_thread_create(&threads[i],
                                 VTL_telegram_all_scanners[i], &contexts[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < VTL_TELEGRAM_SCANNER_COUNT; ++i) {
        VTL_telegram_all_scanners[i](&contexts[i]);
    }

    // ждём все запущенные потоки, собираем их коды возврата
    void* worst = NULL;
    for (size_t i = 0; i < spawned; ++i) {
        void* res = vtl_tg_thread_join(threads[i]);
        if (res != NULL) worst = res;
    }

    // сливаем все буферы в один и сортируем
    VTL_AppResult merge_res = VTL_telegram_merge_markers(partial_lists,
                                                         VTL_TELEGRAM_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_TELEGRAM_SCANNER_COUNT; ++i) {
        VTL_telegram_MarkerListFree(&partial_lists[i]);
    }

    if (worst != NULL) return VTL_res_kErr;
    return merge_res;
}
