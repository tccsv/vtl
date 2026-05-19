#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_scan.h>
#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_threads.h>

#include <stdbool.h>
#include <stdlib.h>


// сколько типов inline-разметки умеем — столько же потоков в Parallel
#define VTL_MARKDOWN_SCANNER_COUNT 3


// ============================================================
// мелкие хелперы
// ============================================================

// буква/цифра/_ — часть слова. Нужно чтобы отличить *bold* (разметка)
// от 1*2*3 (числа). Внимание: '_' в Markdown — сам по себе разделитель,
// но внутри слов он не разделяет (snake_case), поэтому для проверки
// "это слово?" мы тоже считаем '_' буквенным.
static bool VTL_markdown_is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

// символ экранирован, если перед ним нечётное число обратных слешей.
// CommonMark: "\*" — литеральная звёздочка, а не разметка.
static bool VTL_markdown_is_escaped(const char* text, size_t pos)
{
    size_t backslashes = 0;
    while (pos > backslashes && text[pos - 1 - backslashes] == '\\') {
        ++backslashes;
    }
    return (backslashes & 1u) != 0;
}

// безопасно прочитать символ по индексу. Если выход за границы — '\n'.
// Так получаем "виртуальный перевод строки" по краям текста, и правила
// flanking-определителей работают одинаково в середине и на границе.
static char VTL_markdown_char_at(const char* text, size_t pos, size_t len)
{
    return (pos < len) ? text[pos] : '\n';
}

// предыдущий символ перед позицией pos (если pos==0, считаем что был '\n')
static char VTL_markdown_char_before(const char* text, size_t pos)
{
    return (pos > 0) ? text[pos - 1] : '\n';
}

// проверка "точно ли это часть пары" — нужна italic-сканеру,
// чтобы пропустить одиночный * который на самом деле часть **
static bool VTL_markdown_is_part_of_pair(const char* text, size_t pos, size_t len, char delim)
{
    char prev = VTL_markdown_char_before(text, pos);
    char next = VTL_markdown_char_at(text, pos + 1, len);
    return prev == delim || next == delim;
}


// ============================================================
// общий сканер парных однобайтовых токенов: * или _ (italic)
// ============================================================

// возвращает (void*)1 если push в список упал, иначе NULL
static void* VTL_markdown_scan_italic_for_delim(VTL_markdown_ScanContext* ctx, char delim)
{
    bool open = false;

    for (size_t i = 0; i < ctx->text_length; ++i) {
        if (ctx->text[i] != delim) {
            continue;
        }
        // экранированный символ — литерал
        if (VTL_markdown_is_escaped(ctx->text, i)) {
            continue;
        }
        // если это часть пары (**, __) — не italic-токен, пропускаем
        if (VTL_markdown_is_part_of_pair(ctx->text, i, ctx->text_length, delim)) {
            continue;
        }

        char prev = VTL_markdown_char_before(ctx->text, i);
        char next = VTL_markdown_char_at(ctx->text, i + 1, ctx->text_length);

        if (!open) {
            // открывающий: перед ним не буква/цифра, после — не пробел
            bool prev_ok = !VTL_markdown_is_word_char(prev);
            bool next_ok = next != ' ' && next != '\t' &&
                           next != '\n' && next != '\r' && next != '\0';
            if (prev_ok && next_ok) {
                VTL_markdown_Marker m;
                m.kind = VTL_markdown_marker_kItalicStart;
                m.pos = i;
                m.length = 1;
                if (VTL_markdown_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = true;
            }
        } else {
            // закрывающий: перед ним не пробел, после — не буква/цифра
            bool prev_ok = prev != ' ' && prev != '\t' &&
                           prev != '\n' && prev != '\r';
            bool next_ok = !VTL_markdown_is_word_char(next);
            if (prev_ok && next_ok) {
                VTL_markdown_Marker m;
                m.kind = VTL_markdown_marker_kItalicEnd;
                m.pos = i;
                m.length = 1;
                if (VTL_markdown_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = false;
            }
        }
    }
    return NULL;
}


// ============================================================
// общий сканер парных двухбайтовых токенов: ** или __ (bold)
// и ~~ (strike) — общая логика, разные kind'ы
// ============================================================

static void* VTL_markdown_scan_double(VTL_markdown_ScanContext* ctx,
                                       char delim,
                                       VTL_markdown_MarkerKind start_kind,
                                       VTL_markdown_MarkerKind end_kind)
{
    bool open = false;
    size_t i = 0;
    // i + 1 чтобы безопасно посмотреть text[i+1] на пару
    while (i + 1 < ctx->text_length) {
        if (ctx->text[i] != delim || ctx->text[i + 1] != delim) {
            ++i;
            continue;
        }
        // экранированный первый символ — литерал
        if (VTL_markdown_is_escaped(ctx->text, i)) {
            ++i;
            continue;
        }

        char prev = VTL_markdown_char_before(ctx->text, i);
        // символ после ПАРЫ (за два индекса)
        char next = VTL_markdown_char_at(ctx->text, i + 2, ctx->text_length);

        if (!open) {
            // открывающий: перед парой не буква/цифра, после — не пробел
            bool prev_ok = !VTL_markdown_is_word_char(prev);
            bool next_ok = next != ' ' && next != '\t' &&
                           next != '\n' && next != '\r' && next != '\0';
            if (prev_ok && next_ok) {
                VTL_markdown_Marker m;
                m.kind = start_kind;
                m.pos = i;
                m.length = 2;
                if (VTL_markdown_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = true;
                i += 2;
                continue;
            }
        } else {
            // закрывающий: перед парой не пробел, после — не буква/цифра
            bool prev_ok = prev != ' ' && prev != '\t' &&
                           prev != '\n' && prev != '\r';
            bool next_ok = !VTL_markdown_is_word_char(next);
            if (prev_ok && next_ok) {
                VTL_markdown_Marker m;
                m.kind = end_kind;
                m.pos = i;
                m.length = 2;
                if (VTL_markdown_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = false;
                i += 2;
                continue;
            }
        }
        ++i;
    }
    return NULL;
}


// ============================================================
// конкретные сканеры
// ============================================================

void* VTL_markdown_ScanBold(void* arg)
{
    VTL_markdown_ScanContext* ctx = (VTL_markdown_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    // CommonMark разрешает оба синтаксиса. Делаем два прохода —
    // открывающий ** не должен закрываться __, и наоборот. Состояния
    // open у двух проходов независимы, поэтому всё корректно.
    void* r1 = VTL_markdown_scan_double(ctx, '*',
        VTL_markdown_marker_kBoldStart, VTL_markdown_marker_kBoldEnd);
    if (r1) return r1;
    return VTL_markdown_scan_double(ctx, '_',
        VTL_markdown_marker_kBoldStart, VTL_markdown_marker_kBoldEnd);
}

void* VTL_markdown_ScanItalic(void* arg)
{
    VTL_markdown_ScanContext* ctx = (VTL_markdown_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    // два прохода для * и _, независимые состояния open
    void* r1 = VTL_markdown_scan_italic_for_delim(ctx, '*');
    if (r1) return r1;
    return VTL_markdown_scan_italic_for_delim(ctx, '_');
}

void* VTL_markdown_ScanStrike(void* arg)
{
    VTL_markdown_ScanContext* ctx = (VTL_markdown_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_markdown_scan_double(ctx, '~',
        VTL_markdown_marker_kStrikeStart, VTL_markdown_marker_kStrikeEnd);
}


// ============================================================
// слияние частичных списков (только для Parallel) + сортировка
// ============================================================

// qsort comparator: сначала по позиции, при равной — по типу
static int VTL_markdown_marker_cmp(const void* a, const void* b)
{
    const VTL_markdown_Marker* ma = (const VTL_markdown_Marker*)a;
    const VTL_markdown_Marker* mb = (const VTL_markdown_Marker*)b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return  1;
    if (ma->kind < mb->kind) return -1;
    if (ma->kind > mb->kind) return  1;
    return 0;
}

static VTL_AppResult VTL_markdown_sort_markers(VTL_markdown_MarkerList* list)
{
    if (!list) return VTL_res_kInvalidParamErr;
    if (list->length > 1) {
        qsort(list->items, list->length, sizeof(VTL_markdown_Marker),
              VTL_markdown_marker_cmp);
    }
    return VTL_res_kOk;
}

static VTL_AppResult VTL_markdown_merge_markers(const VTL_markdown_MarkerList* parts,
                                                size_t parts_count,
                                                VTL_markdown_MarkerList* out)
{
    if (!parts || !out) return VTL_res_kInvalidParamErr;

    // считаем общий размер заранее — один reserve вместо N realloc'ов
    size_t total = 0;
    for (size_t i = 0; i < parts_count; ++i) {
        total += parts[i].length;
    }
    VTL_AppResult res = VTL_markdown_MarkerListReserve(out, total);
    if (res != VTL_res_kOk) return res;

    for (size_t i = 0; i < parts_count; ++i) {
        for (size_t j = 0; j < parts[i].length; ++j) {
            res = VTL_markdown_MarkerListPush(out, parts[i].items[j]);
            if (res != VTL_res_kOk) return res;
        }
    }
    return VTL_markdown_sort_markers(out);
}


// ============================================================
// оркестрация: Sequential vs Parallel
// ============================================================

typedef void* (*VTL_markdown_scanner_fn)(void*);

static const VTL_markdown_scanner_fn VTL_markdown_all_scanners[VTL_MARKDOWN_SCANNER_COUNT] = {
    VTL_markdown_ScanBold,
    VTL_markdown_ScanItalic,
    VTL_markdown_ScanStrike
};


VTL_AppResult VTL_markdown_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_markdown_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_markdown_ScanContext ctx;
    ctx.text = src->text;
    ctx.text_length = src->length;
    ctx.out = out;

    for (size_t i = 0; i < VTL_MARKDOWN_SCANNER_COUNT; ++i) {
        if (VTL_markdown_all_scanners[i](&ctx) != NULL) {
            return VTL_res_kErr;
        }
    }
    return VTL_markdown_sort_markers(out);
}


VTL_AppResult VTL_markdown_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_markdown_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    // у каждого сканера свой буфер и свой контекст — поэтому без локов
    VTL_markdown_MarkerList partial_lists[VTL_MARKDOWN_SCANNER_COUNT];
    VTL_markdown_ScanContext contexts[VTL_MARKDOWN_SCANNER_COUNT];
    vtl_md_thread_t threads[VTL_MARKDOWN_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_MARKDOWN_SCANNER_COUNT; ++i) {
        VTL_markdown_MarkerListInit(&partial_lists[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial_lists[i];
    }

    // если создание потока упадёт на полпути — оставшиеся гоним последовательно
    size_t spawned = 0;
    for (size_t i = 0; i < VTL_MARKDOWN_SCANNER_COUNT; ++i) {
        if (vtl_md_thread_create(&threads[i],
                                 VTL_markdown_all_scanners[i], &contexts[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < VTL_MARKDOWN_SCANNER_COUNT; ++i) {
        VTL_markdown_all_scanners[i](&contexts[i]);
    }

    void* worst = NULL;
    for (size_t i = 0; i < spawned; ++i) {
        void* res = vtl_md_thread_join(threads[i]);
        if (res != NULL) worst = res;
    }

    VTL_AppResult merge_res = VTL_markdown_merge_markers(partial_lists,
                                                         VTL_MARKDOWN_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_MARKDOWN_SCANNER_COUNT; ++i) {
        VTL_markdown_MarkerListFree(&partial_lists[i]);
    }

    if (worst != NULL) return VTL_res_kErr;
    return merge_res;
}
