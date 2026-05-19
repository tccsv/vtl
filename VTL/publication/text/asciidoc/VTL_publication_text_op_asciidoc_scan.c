#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_scan.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_convert.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_compat.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


// сколько типов разметки умеем — столько же потоков в параллельном режиме
#define VTL_ASCIIDOC_SCANNER_COUNT 15


// открывающая последовательность зачёркивания
static const char VTL_ASCIIDOC_STRIKE_OPEN[] = "[line-through]#";
#define VTL_ASCIIDOC_STRIKE_OPEN_LEN (sizeof(VTL_ASCIIDOC_STRIKE_OPEN) - 1)


// ============================================================
// мелкие хелперы
// ============================================================

// буква/цифра/_ — часть слова. нужно чтобы отличить *bold* от 1*2*3
static bool VTL_asciidoc_is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

// pos — начало строки (самое начало текста или сразу после \n)
static bool VTL_asciidoc_is_at_line_start(const char* text, size_t pos)
{
    if (pos == 0) {
        return true;
    }
    return text[pos - 1] == '\n';
}

// позиция '\n' от from до конца текста (или text_len если \n не нашли)
static size_t VTL_asciidoc_line_end(const char* text, size_t text_len, size_t from)
{
    size_t i = from;
    while (i < text_len && text[i] != '\n') {
        ++i;
    }
    return i;
}


// ============================================================
// inline парные токены: * _ ` ~ ^
// один и тот же символ — и открывающий, и закрывающий
// ============================================================

// общая функция для одиночных парных символов
// возвращает (void*)1 если push в список упал, иначе NULL
static void* VTL_asciidoc_scan_inline_pair(VTL_asciidoc_ScanContext* ctx,
                                            char delim,
                                            VTL_asciidoc_MarkerKind start_kind,
                                            VTL_asciidoc_MarkerKind end_kind)
{
    bool open = false;

    for (size_t i = 0; i < ctx->text_length; ++i) {
        if (ctx->text[i] != delim) {
            continue;
        }

        // если соседа нет — считаем что там \n
        char prev = (i > 0) ? ctx->text[i - 1] : '\n';
        char next = (i + 1 < ctx->text_length) ? ctx->text[i + 1] : '\n';

        if (!open) {
            // открывающий: перед ним не буква (не середина слова),
            // после — не пробел (значит дальше идёт текст разметки)
            bool prev_ok = !VTL_asciidoc_is_word_char(prev);
            bool next_ok = next != ' ' && next != '\t' && next != '\n' && next != '\0';
            if (prev_ok && next_ok) {
                VTL_asciidoc_Marker m = { start_kind, i, 1, 0 };
                if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = true;
            }
        } else {
            // закрывающий: перед ним не пробел, после — не буква
            bool prev_ok = prev != ' ' && prev != '\t' && prev != '\n';
            bool next_ok = !VTL_asciidoc_is_word_char(next);
            if (prev_ok && next_ok) {
                VTL_asciidoc_Marker m = { end_kind, i, 1, 0 };
                if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = false;
            }
        }
    }
    return NULL;
}


void* VTL_asciidoc_ScanBold(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '*',
        VTL_asciidoc_marker_kBoldStart, VTL_asciidoc_marker_kBoldEnd);
}

void* VTL_asciidoc_ScanItalic(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '_',
        VTL_asciidoc_marker_kItalicStart, VTL_asciidoc_marker_kItalicEnd);
}

void* VTL_asciidoc_ScanMono(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '`',
        VTL_asciidoc_marker_kMonoStart, VTL_asciidoc_marker_kMonoEnd);
}

void* VTL_asciidoc_ScanSubscript(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '~',
        VTL_asciidoc_marker_kSubscriptStart, VTL_asciidoc_marker_kSubscriptEnd);
}

void* VTL_asciidoc_ScanSuperscript(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '^',
        VTL_asciidoc_marker_kSuperscriptStart, VTL_asciidoc_marker_kSuperscriptEnd);
}


// ============================================================
// зачёркивание [line-through]#текст#
// не парный одиночный символ — поэтому отдельная функция
// ============================================================

void* VTL_asciidoc_ScanStrikethrough(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + VTL_ASCIIDOC_STRIKE_OPEN_LEN <= ctx->text_length) {
        // ищем "[line-through]#"
        if (memcmp(ctx->text + i, VTL_ASCIIDOC_STRIKE_OPEN,
                   VTL_ASCIIDOC_STRIKE_OPEN_LEN) != 0) {
            ++i;
            continue;
        }

        // нашли — теперь первое # в этой же строке
        size_t close_pos = (size_t)-1;
        for (size_t j = i + VTL_ASCIIDOC_STRIKE_OPEN_LEN; j < ctx->text_length; ++j) {
            if (ctx->text[j] == '#') { close_pos = j; break; }
            if (ctx->text[j] == '\n') break;
        }
        if (close_pos == (size_t)-1) {
            ++i;
            continue;
        }

        // start длинный (15 байт), end — один #
        VTL_asciidoc_Marker start_m = {
            VTL_asciidoc_marker_kStrikeStart, i, VTL_ASCIIDOC_STRIKE_OPEN_LEN, 0
        };
        VTL_asciidoc_Marker end_m = {
            VTL_asciidoc_marker_kStrikeEnd, close_pos, 1, 0
        };
        if (VTL_asciidoc_MarkerListPush(ctx->out, start_m) != VTL_res_kOk) return (void*)1;
        if (VTL_asciidoc_MarkerListPush(ctx->out, end_m) != VTL_res_kOk) return (void*)1;
        i = close_pos + 1;
    }
    return NULL;
}


// ============================================================
// ссылки
// link:url[текст] — обычная
// <<id>> или <<id,текст>> — на якорь
// ============================================================

void* VTL_asciidoc_ScanLink(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    static const char prefix[] = "link:";
    const size_t prefix_len = sizeof(prefix) - 1;

    size_t i = 0;
    while (i + prefix_len <= ctx->text_length) {
        if (memcmp(ctx->text + i, prefix, prefix_len) != 0) {
            ++i;
            continue;
        }
        // после "link:" — url до '[', потом ']'
        size_t bracket_open = (size_t)-1;
        for (size_t j = i + prefix_len; j < ctx->text_length; ++j) {
            if (ctx->text[j] == '[') { bracket_open = j; break; }
            if (ctx->text[j] == ' ' || ctx->text[j] == '\n') break;
        }
        if (bracket_open == (size_t)-1) { ++i; continue; }

        size_t bracket_close = (size_t)-1;
        for (size_t j = bracket_open + 1; j < ctx->text_length; ++j) {
            if (ctx->text[j] == ']') { bracket_close = j; break; }
            if (ctx->text[j] == '\n') break;
        }
        if (bracket_close == (size_t)-1) { ++i; continue; }

        // маркер охватывает всё "link:url[текст]"
        VTL_asciidoc_Marker m = {
            VTL_asciidoc_marker_kLink, i, bracket_close - i + 1, 0
        };
        if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = bracket_close + 1;
    }
    return NULL;
}

void* VTL_asciidoc_ScanCrossReference(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + 1 < ctx->text_length) {
        if (ctx->text[i] != '<' || ctx->text[i + 1] != '<') {
            ++i;
            continue;
        }
        // ищем >>
        size_t close_pos = (size_t)-1;
        for (size_t j = i + 2; j + 1 < ctx->text_length; ++j) {
            if (ctx->text[j] == '>' && ctx->text[j + 1] == '>') {
                close_pos = j + 1;
                break;
            }
            if (ctx->text[j] == '\n') break;
        }
        if (close_pos == (size_t)-1) { ++i; continue; }

        VTL_asciidoc_Marker m = {
            VTL_asciidoc_marker_kCrossReference, i, close_pos - i + 1, 0
        };
        if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = close_pos + 1;
    }
    return NULL;
}


// ============================================================
// блочные сканеры (по строкам)
// ============================================================

void* VTL_asciidoc_ScanHeading(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        // считаем подряд идущие '=' — это уровень
        size_t level = 0;
        while (i + level < ctx->text_length && ctx->text[i + level] == '=' && level < 6) {
            ++level;
        }
        // после '=' должен идти пробел, иначе не заголовок
        if (level > 0 && i + level < ctx->text_length && ctx->text[i + level] == ' ') {
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kHeading, i, level + 1, (int)level
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}

void* VTL_asciidoc_ScanList(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        // * (маркированный) или . (нумерованный). повторы = уровень вложенности
        char bullet = ctx->text[i];
        if (bullet == '*' || bullet == '.') {
            size_t level = 0;
            while (i + level < ctx->text_length && ctx->text[i + level] == bullet && level < 5) {
                ++level;
            }
            if (level > 0 && i + level < ctx->text_length && ctx->text[i + level] == ' ') {
                VTL_asciidoc_Marker m = {
                    VTL_asciidoc_marker_kListItem, i, level + 1, (int)level
                };
                if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            }
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}


// общая функция для блоков с одинаковыми открывающей и закрывающей строками
// (---- ... ----   и   ____ ... ____)
static void* VTL_asciidoc_scan_fenced_block(VTL_asciidoc_ScanContext* ctx,
                                             const char* fence, size_t fence_len,
                                             VTL_asciidoc_MarkerKind kind_start,
                                             VTL_asciidoc_MarkerKind kind_end)
{
    size_t i = 0;
    bool open = false;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t line_end = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i);
        size_t line_len = line_end - i;
        if (line_len == fence_len && memcmp(ctx->text + i, fence, fence_len) == 0) {
            VTL_asciidoc_Marker m = {
                open ? kind_end : kind_start,
                i, fence_len, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            open = !open;
        }
        i = line_end + 1;
    }
    return NULL;
}

void* VTL_asciidoc_ScanCodeBlock(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_fenced_block(ctx, "----", 4,
        VTL_asciidoc_marker_kCodeBlockStart, VTL_asciidoc_marker_kCodeBlockEnd);
}

void* VTL_asciidoc_ScanQuotedBlock(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_fenced_block(ctx, "____", 4,
        VTL_asciidoc_marker_kQuotedBlockStart, VTL_asciidoc_marker_kQuotedBlockEnd);
}


void* VTL_asciidoc_ScanComment(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        if (i + 1 < ctx->text_length && ctx->text[i] == '/' && ctx->text[i + 1] == '/') {
            size_t line_end = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i);
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kComment, i, line_end - i, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            i = line_end + 1;
            continue;
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}


// ============================================================
// уровень документа
// :ключ: значение  — атрибут
// NOTE:/TIP:/WARNING:/IMPORTANT:/CAUTION: — admonition
// ============================================================

void* VTL_asciidoc_ScanAttribute(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        // строка должна начинаться с ':' и содержать второе ':' до конца строки
        if (ctx->text[i] != ':') {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t line_end = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i);
        size_t second_colon = (size_t)-1;
        for (size_t j = i + 1; j < line_end; ++j) {
            if (ctx->text[j] == ':') { second_colon = j; break; }
        }
        // между двумя ':' хотя бы один символ — иначе имя пустое
        if (second_colon != (size_t)-1 && second_colon > i + 1) {
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kAttribute, i, line_end - i, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = line_end + 1;
    }
    return NULL;
}

// поддерживаемые метки admonition
static bool VTL_asciidoc_is_admonition_label(const char* text, size_t pos, size_t text_len)
{
    static const char* labels[] = {
        "NOTE: ", "TIP: ", "IMPORTANT: ", "WARNING: ", "CAUTION: "
    };
    static const size_t lens[] = { 6, 5, 11, 9, 9 };
    const size_t count = sizeof(labels) / sizeof(labels[0]);

    for (size_t k = 0; k < count; ++k) {
        if (pos + lens[k] <= text_len &&
            memcmp(text + pos, labels[k], lens[k]) == 0) {
            return true;
        }
    }
    return false;
}

void* VTL_asciidoc_ScanAdmonition(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        if (VTL_asciidoc_is_admonition_label(ctx->text, i, ctx->text_length)) {
            // длина маркера — до ':' с пробелом, чтобы при конвертации не печатать "NOTE: "
            size_t colon = i;
            while (colon < ctx->text_length && ctx->text[colon] != ':') ++colon;
            size_t marker_len = (colon + 2 <= ctx->text_length) ? (colon - i + 2) : (colon - i + 1);
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kAdmonition, i, marker_len, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}


// ============================================================
// оркестрация: Sequential vs Parallel
// ============================================================

typedef void* (*VTL_asciidoc_scanner_fn)(void*);

// все 15 сканеров в одном массиве. добавляешь новый — допиши сюда
static const VTL_asciidoc_scanner_fn VTL_asciidoc_all_scanners[VTL_ASCIIDOC_SCANNER_COUNT] = {
    VTL_asciidoc_ScanBold,
    VTL_asciidoc_ScanItalic,
    VTL_asciidoc_ScanMono,
    VTL_asciidoc_ScanSubscript,
    VTL_asciidoc_ScanSuperscript,
    VTL_asciidoc_ScanStrikethrough,
    VTL_asciidoc_ScanLink,
    VTL_asciidoc_ScanCrossReference,
    VTL_asciidoc_ScanHeading,
    VTL_asciidoc_ScanList,
    VTL_asciidoc_ScanCodeBlock,
    VTL_asciidoc_ScanQuotedBlock,
    VTL_asciidoc_ScanComment,
    VTL_asciidoc_ScanAttribute,
    VTL_asciidoc_ScanAdmonition
};


VTL_AppResult VTL_asciidoc_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_asciidoc_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_asciidoc_ScanContext ctx = { src->text, src->length, out };
    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        if (VTL_asciidoc_all_scanners[i](&ctx) != NULL) {
            return VTL_res_kErr;
        }
    }
    return VTL_asciidoc_SortMarkers(out);
}


VTL_AppResult VTL_asciidoc_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_asciidoc_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    // у каждого сканера свой буфер и свой контекст — ради этого и нет локов
    VTL_asciidoc_MarkerList partial_lists[VTL_ASCIIDOC_SCANNER_COUNT];
    VTL_asciidoc_ScanContext contexts[VTL_ASCIIDOC_SCANNER_COUNT];
    vtl_thread_t threads[VTL_ASCIIDOC_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        VTL_asciidoc_MarkerListInit(&partial_lists[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial_lists[i];
    }

    // если создание потока упадёт на полпути — оставшиеся гоним последовательно
    size_t spawned = 0;
    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        if (vtl_thread_create(&threads[i],
                              VTL_asciidoc_all_scanners[i], &contexts[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        VTL_asciidoc_all_scanners[i](&contexts[i]);
    }

    // ждём все запущенные потоки, собираем их коды возврата
    void* worst = NULL;
    for (size_t i = 0; i < spawned; ++i) {
        void* res = vtl_thread_join(threads[i]);
        if (res != NULL) worst = res;
    }

    // сливаем все буферы в один и сортируем
    VTL_AppResult merge_res = VTL_asciidoc_MergeMarkers(partial_lists,
                                                       VTL_ASCIIDOC_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        VTL_asciidoc_MarkerListFree(&partial_lists[i]);
    }

    if (worst != NULL) return VTL_res_kErr;
    return merge_res;
}
