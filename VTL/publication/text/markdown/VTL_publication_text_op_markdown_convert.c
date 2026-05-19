#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_convert.h>
#include <VTL/publication/text/VTL_publication_text_data.h>

#include <stdlib.h>
#include <string.h>


/* ВАЖНО: глобальные макросы VTL_TEXT_MODIFICATION_* в VTL_publication_text_data.h
 * объявлены БЕЗ скобок: "#define VTL_TEXT_MODIFICATION_BOLD 1 << 0".
 * При использовании в выражении "~VTL_TEXT_MODIFICATION_ITALIC" они
 * раскрываются как "~1 << 1" = "(~1) << 1" = 0xFFFFFFFC, что сбрасывает
 * и BOLD-бит тоже — типичная ловушка приоритета операторов.
 *
 * Локальные MD_MOD_* — те же численные значения, но в скобках. Их можно
 * безопасно инвертировать. Сами имена в data.h не трогаем. */
#define MD_MOD_BOLD   ((VTL_publication_text_modification_Flags)(1u << 0))
#define MD_MOD_ITALIC ((VTL_publication_text_modification_Flags)(1u << 1))
#define MD_MOD_STRIKE ((VTL_publication_text_modification_Flags)(1u << 2))
#define MD_MOD_MASK   (MD_MOD_BOLD | MD_MOD_ITALIC | MD_MOD_STRIKE)


// собирает одну часть для MarkedText (без выделения памяти)
static VTL_publication_marked_text_Part VTL_markdown_make_part(const char* text_start,
    size_t length, VTL_publication_text_modification_Flags flags)
{
    VTL_publication_marked_text_Part p;
    p.text = (VTL_publication_text_Symbol*)text_start;
    p.length = length;
    p.type = flags;
    return p;
}

VTL_AppResult VTL_markdown_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_markdown_MarkerList* markers,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !src->text || !markers || !out) return VTL_res_kInvalidParamErr;

    VTL_publication_MarkedText* block =
        (VTL_publication_MarkedText*)malloc(sizeof(VTL_publication_MarkedText));
    if (!block) return VTL_res_kMemAllocErr;

    // между парами маркеров — кусок текста, плюс пролог и хвост.
    // верхний предел: маркеров * 2 + 2
    size_t max_parts = markers->length * 2 + 2;
    if (max_parts == 0) max_parts = 1;
    block->parts = (VTL_publication_marked_text_Part*)malloc(
        max_parts * sizeof(VTL_publication_marked_text_Part));
    if (!block->parts) { free(block); return VTL_res_kMemAllocErr; }
    block->length = 0;

    VTL_publication_text_modification_Flags flags = 0;
    size_t cursor = 0;

    for (size_t i = 0; i < markers->length; ++i) {
        const VTL_markdown_Marker* m = &markers->items[i];

        // маркер внутри уже обработанного куска — пропускаем
        // (страховка от пересечений между потоками-сканерами)
        if (m->pos < cursor) continue;

        // кусок обычного текста перед маркером
        if (m->pos > cursor) {
            block->parts[block->length++] = VTL_markdown_make_part(
                src->text + cursor, m->pos - cursor, flags);
        }

        // переключаем флаги в зависимости от типа маркера.
        // используем локальные MD_MOD_* (в скобках)
        switch (m->kind) {
            case VTL_markdown_marker_kBoldStart:   flags |=  MD_MOD_BOLD;   break;
            case VTL_markdown_marker_kBoldEnd:     flags &= ~MD_MOD_BOLD;   break;
            case VTL_markdown_marker_kItalicStart: flags |=  MD_MOD_ITALIC; break;
            case VTL_markdown_marker_kItalicEnd:   flags &= ~MD_MOD_ITALIC; break;
            case VTL_markdown_marker_kStrikeStart: flags |=  MD_MOD_STRIKE; break;
            case VTL_markdown_marker_kStrikeEnd:   flags &= ~MD_MOD_STRIKE; break;
        }

        cursor = m->pos + m->length;
    }

    if (cursor < src->length) {
        block->parts[block->length++] = VTL_markdown_make_part(
            src->text + cursor, src->length - cursor, flags);
    }

    *out = block;
    return VTL_res_kOk;
}


// ============================================================
// обратное направление: MarkedText → Standart Markdown
// ============================================================

// какие байты экранируем в обычном тексте при сериализации.
// CommonMark позволяет экранировать любой ASCII-пунктуацию обратным слешем,
// но обязательны только сами разметочные символы и сам '\'.
static int VTL_markdown_needs_escape(char c)
{
    return c == '*' || c == '_' || c == '~' || c == '\\';
}

// верхняя оценка размера сериализации одной части.
// в худшем случае каждый символ удваивается (экранирование),
// плюс открывающие+закрывающие теги. Максимум: ** + __ + ~~ = 6 байт
// с каждой стороны → +12 в худшем случае. Берём с запасом.
static size_t VTL_markdown_part_max_size(const VTL_publication_marked_text_Part* p)
{
    return p->length * 2u + 12u;
}

// дописывает в dest закрывающие токены для флагов, которые "уходят".
// Возвращает новый указатель конца.
static char* VTL_markdown_write_closing(char* dest,
    VTL_publication_text_modification_Flags from,
    VTL_publication_text_modification_Flags to)
{
    // закрываем в обратном порядке открытия: Strike → Italic → Bold
    if ((from & MD_MOD_STRIKE) && !(to & MD_MOD_STRIKE)) {
        dest[0] = '~'; dest[1] = '~'; dest += 2;
    }
    if ((from & MD_MOD_ITALIC) && !(to & MD_MOD_ITALIC)) {
        *dest++ = '*';
    }
    if ((from & MD_MOD_BOLD) && !(to & MD_MOD_BOLD)) {
        dest[0] = '*'; dest[1] = '*'; dest += 2;
    }
    return dest;
}

// дописывает открывающие токены для флагов, которые "приходят".
// Порядок: Bold → Italic → Strike (симметрично закрывающим)
static char* VTL_markdown_write_opening(char* dest,
    VTL_publication_text_modification_Flags from,
    VTL_publication_text_modification_Flags to)
{
    if (!(from & MD_MOD_BOLD) && (to & MD_MOD_BOLD)) {
        dest[0] = '*'; dest[1] = '*'; dest += 2;
    }
    if (!(from & MD_MOD_ITALIC) && (to & MD_MOD_ITALIC)) {
        *dest++ = '*';
    }
    if (!(from & MD_MOD_STRIKE) && (to & MD_MOD_STRIKE)) {
        dest[0] = '~'; dest[1] = '~'; dest += 2;
    }
    return dest;
}

// копирует текст части в dest, экранируя спецсимволы. Возвращает новый конец.
static char* VTL_markdown_write_escaped(char* dest, const char* src, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        char c = src[i];
        if (VTL_markdown_needs_escape(c)) {
            *dest++ = '\\';
        }
        *dest++ = c;
    }
    return dest;
}


VTL_AppResult VTL_markdown_SerializeFromMarkedText(const VTL_publication_MarkedText* src,
                                                    VTL_publication_Text** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    // считаем верхнюю оценку размера одним проходом — одна аллокация
    size_t cap = 1;  // под завершающий '\0'
    for (size_t i = 0; i < src->length; ++i) {
        cap += VTL_markdown_part_max_size(&src->parts[i]);
    }

    char* buf = (char*)malloc(cap);
    if (!buf) return VTL_res_kMemAllocErr;

    char* dest = buf;
    VTL_publication_text_modification_Flags current = 0;

    for (size_t i = 0; i < src->length; ++i) {
        const VTL_publication_marked_text_Part* p = &src->parts[i];
        VTL_publication_text_modification_Flags target = p->type & MD_MOD_MASK;

        // закрыть то, что больше не нужно, потом открыть то, что добавилось
        dest = VTL_markdown_write_closing(dest, current, target);
        dest = VTL_markdown_write_opening(dest, current, target);
        current = target;

        if (p->text && p->length > 0) {
            dest = VTL_markdown_write_escaped(dest, (const char*)p->text, p->length);
        }
    }
    // закрываем всё, что осталось открытым
    dest = VTL_markdown_write_closing(dest, current, 0);

    *dest = '\0';
    size_t written = (size_t)(dest - buf);

    VTL_publication_Text* result =
        (VTL_publication_Text*)calloc(1, sizeof(VTL_publication_Text));
    if (!result) { free(buf); return VTL_res_kMemAllocErr; }
    result->text = (VTL_publication_text_Symbol*)buf;
    result->length = written;
    *out = result;
    return VTL_res_kOk;
}
