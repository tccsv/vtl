#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/html/VTL_publication_text_op_html_scan.h>
#include <VTL/publication/text/html/VTL_publication_text_op_html_threads.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


// сколько типов inline-разметки умеем — столько же потоков в Parallel
#define VTL_HTML_SCANNER_COUNT 3

// максимальное число алиасов на одно семейство (b/strong, i/em, s/del → 2)
#define VTL_HTML_MAX_ALIASES 2


// ============================================================
// мелкие хелперы
// ============================================================

// ASCII to-lower без локали — нам нужны только латинские буквы a-z в имени тега
static inline char VTL_html_to_lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

// сравнение имени тега с эталоном; имя — name_len байт начиная с name,
// эталон — null-terminated lowercase ASCII строка
static bool VTL_html_name_equals_ci(const char* name, size_t name_len,
                                    const char* lowercase_target)
{
    size_t tlen = strlen(lowercase_target);
    if (name_len != tlen) return false;
    for (size_t i = 0; i < name_len; ++i) {
        if (VTL_html_to_lower(name[i]) != lowercase_target[i]) return false;
    }
    return true;
}

// после '<' (или "</") извлекает имя тега; возвращает указатель на первый
// символ после имени (пробел / атрибут / '>') и длину имени через out_len.
// Имя — последовательность букв/цифр. Если буквы нет — name_len = 0.
static const char* VTL_html_read_tag_name(const char* p, const char* end,
                                          size_t* out_name_len)
{
    const char* start = p;
    while (p < end) {
        char c = *p;
        bool is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        bool is_digit = (c >= '0' && c <= '9');
        if (!is_alpha && !is_digit) break;
        ++p;
    }
    *out_name_len = (size_t)(p - start);
    return p;
}


// ============================================================
// общий сканер парных HTML-тегов
// ============================================================
//
// Семантика:
//   - Открывающий тег: <name> или <name [attrs]>
//   - Закрывающий тег: </name> (атрибуты в закрывающем не допускаются HTML5)
//   - Имя тега case-insensitive
//   - Атрибуты — всё что между именем и '>'; пропускаем грубо, до '>' или конца
//
// Ограничения нашей минимальной реализации:
//   - '>' внутри кавычек атрибута воспримется как конец тега
//     (для тегов b/i/s/em/del/strong атрибуты редко содержат '>', и
//     для текстовой разметки это допустимо)
//   - не обрабатываем HTML-комментарии <!-- ... -->
//   - не обрабатываем CDATA / <script> / <style> (они и не должны
//     лезть в текстовый поток для разметки)

static void* VTL_html_scan_pair_family(VTL_html_ScanContext* ctx,
                                       const char* const aliases[VTL_HTML_MAX_ALIASES],
                                       size_t alias_count,
                                       VTL_html_MarkerKind start_kind,
                                       VTL_html_MarkerKind end_kind)
{
    const char* text = ctx->text;
    const char* end  = ctx->text + ctx->text_length;
    bool open = false;

    for (const char* p = text; p < end; ++p) {
        if (*p != '<') continue;

        const char* tag_open_pos = p;
        const char* q = p + 1;
        if (q >= end) break;

        bool is_closing = (*q == '/');
        if (is_closing) ++q;

        size_t name_len = 0;
        const char* after_name = VTL_html_read_tag_name(q, end, &name_len);
        if (name_len == 0) continue;  // не тег, просто '<' в тексте

        // проверяем что имя совпадает с одним из алиасов семейства
        bool matched = false;
        for (size_t a = 0; a < alias_count; ++a) {
            if (VTL_html_name_equals_ci(q, name_len, aliases[a])) {
                matched = true;
                break;
            }
        }
        if (!matched) continue;  // чужой тег — не наша забота

        // ищем '>' до конца текста; всё что между after_name и '>' — атрибуты
        const char* gt = after_name;
        while (gt < end && *gt != '>') ++gt;
        if (gt >= end) continue;  // незакрытый тег — игнорируем

        size_t tag_length = (size_t)(gt + 1 - tag_open_pos);

        VTL_html_Marker m;
        m.pos = (size_t)(tag_open_pos - text);
        m.length = tag_length;

        // парность: чередуем open/close, как в telegram
        if (!is_closing && !open) {
            m.kind = start_kind;
            if (VTL_html_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                return (void*)1;
            }
            open = true;
        } else if (is_closing && open) {
            m.kind = end_kind;
            if (VTL_html_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                return (void*)1;
            }
            open = false;
        }
        // иначе (двойной open или close без open) — пропускаем,
        // даём шанс корректным следующим парам

        p = gt;  // прыжок на '>' — следующая итерация цикла p++ перешагнёт его
    }
    return NULL;
}


// ============================================================
// конкретные сканеры: параметризуют scan_pair_family списком алиасов
// ============================================================

void* VTL_html_ScanBold(void* arg)
{
    VTL_html_ScanContext* ctx = (VTL_html_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    static const char* const aliases[VTL_HTML_MAX_ALIASES] = { "b", "strong" };
    return VTL_html_scan_pair_family(ctx, aliases, 2,
        VTL_html_marker_kBoldStart, VTL_html_marker_kBoldEnd);
}

void* VTL_html_ScanItalic(void* arg)
{
    VTL_html_ScanContext* ctx = (VTL_html_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    static const char* const aliases[VTL_HTML_MAX_ALIASES] = { "i", "em" };
    return VTL_html_scan_pair_family(ctx, aliases, 2,
        VTL_html_marker_kItalicStart, VTL_html_marker_kItalicEnd);
}

void* VTL_html_ScanStrike(void* arg)
{
    VTL_html_ScanContext* ctx = (VTL_html_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    static const char* const aliases[VTL_HTML_MAX_ALIASES] = { "s", "del" };
    return VTL_html_scan_pair_family(ctx, aliases, 2,
        VTL_html_marker_kStrikeStart, VTL_html_marker_kStrikeEnd);
}


// ============================================================
// слияние частичных списков (только для Parallel) + сортировка
// ============================================================

// qsort comparator: сначала по позиции, при равной — по типу
static int VTL_html_marker_cmp(const void* a, const void* b)
{
    const VTL_html_Marker* ma = (const VTL_html_Marker*)a;
    const VTL_html_Marker* mb = (const VTL_html_Marker*)b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return  1;
    if (ma->kind < mb->kind) return -1;
    if (ma->kind > mb->kind) return  1;
    return 0;
}

static VTL_AppResult VTL_html_sort_markers(VTL_html_MarkerList* list)
{
    if (!list) return VTL_res_kInvalidParamErr;
    if (list->length > 1) {
        qsort(list->items, list->length, sizeof(VTL_html_Marker),
              VTL_html_marker_cmp);
    }
    return VTL_res_kOk;
}

static VTL_AppResult VTL_html_merge_markers(const VTL_html_MarkerList* parts,
                                            size_t parts_count,
                                            VTL_html_MarkerList* out)
{
    if (!parts || !out) return VTL_res_kInvalidParamErr;

    // считаем общий размер заранее — один reserve вместо N realloc'ов
    size_t total = 0;
    for (size_t i = 0; i < parts_count; ++i) {
        total += parts[i].length;
    }
    VTL_AppResult res = VTL_html_MarkerListReserve(out, total);
    if (res != VTL_res_kOk) return res;

    for (size_t i = 0; i < parts_count; ++i) {
        for (size_t j = 0; j < parts[i].length; ++j) {
            res = VTL_html_MarkerListPush(out, parts[i].items[j]);
            if (res != VTL_res_kOk) return res;
        }
    }
    return VTL_html_sort_markers(out);
}


// ============================================================
// оркестрация: Sequential vs Parallel
// ============================================================

typedef void* (*VTL_html_scanner_fn)(void*);

// массив всех сканеров — добавишь новый, дописываешь сюда
static const VTL_html_scanner_fn VTL_html_all_scanners[VTL_HTML_SCANNER_COUNT] = {
    VTL_html_ScanBold,
    VTL_html_ScanItalic,
    VTL_html_ScanStrike
};


VTL_AppResult VTL_html_ParseDocumentSequential(const VTL_publication_Text* src,
                                                VTL_html_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_html_ScanContext ctx;
    ctx.text = src->text;
    ctx.text_length = src->length;
    ctx.out = out;

    for (size_t i = 0; i < VTL_HTML_SCANNER_COUNT; ++i) {
        if (VTL_html_all_scanners[i](&ctx) != NULL) {
            return VTL_res_kErr;
        }
    }
    return VTL_html_sort_markers(out);
}


VTL_AppResult VTL_html_ParseDocumentParallel(const VTL_publication_Text* src,
                                              VTL_html_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    // у каждого сканера свой буфер и свой контекст — поэтому без локов
    VTL_html_MarkerList partial_lists[VTL_HTML_SCANNER_COUNT];
    VTL_html_ScanContext contexts[VTL_HTML_SCANNER_COUNT];
    vtl_html_thread_t threads[VTL_HTML_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_HTML_SCANNER_COUNT; ++i) {
        VTL_html_MarkerListInit(&partial_lists[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial_lists[i];
    }

    // если создание потока упадёт на полпути — оставшиеся гоним последовательно
    size_t spawned = 0;
    for (size_t i = 0; i < VTL_HTML_SCANNER_COUNT; ++i) {
        if (vtl_html_thread_create(&threads[i],
                                   VTL_html_all_scanners[i], &contexts[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < VTL_HTML_SCANNER_COUNT; ++i) {
        VTL_html_all_scanners[i](&contexts[i]);
    }

    // ждём все запущенные потоки, собираем их коды возврата
    void* worst = NULL;
    for (size_t i = 0; i < spawned; ++i) {
        void* res = vtl_html_thread_join(threads[i]);
        if (res != NULL) worst = res;
    }

    // сливаем все буферы в один и сортируем
    VTL_AppResult merge_res = VTL_html_merge_markers(partial_lists,
                                                     VTL_HTML_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_HTML_SCANNER_COUNT; ++i) {
        VTL_html_MarkerListFree(&partial_lists[i]);
    }

    if (worst != NULL) return VTL_res_kErr;
    return merge_res;
}
