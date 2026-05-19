#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_convert.h>
#include <VTL/publication/text/VTL_publication_text_data.h>

#include <stdlib.h>
#include <string.h>


/* ВАЖНО: глобальные макросы VTL_TEXT_MODIFICATION_* в VTL_publication_text_data.h
 * объявлены БЕЗ скобок: "#define VTL_TEXT_MODIFICATION_BOLD 1 << 0".
 * При использовании в выражении "~VTL_TEXT_MODIFICATION_ITALIC" они
 * раскрываются как "~1 << 1" = "(~1) << 1" = 0xFFFFFFFC, что сбрасывает
 * и BOLD-бит тоже — типичная ловушка приоритета операторов.
 *
 * Локальные TG_MOD_* — те же численные значения, но в скобках. Их можно
 * безопасно инвертировать. Сами имена в data.h не трогаем (по требованию). */
#define TG_MOD_BOLD   ((VTL_publication_text_modification_Flags)(1u << 0))
#define TG_MOD_ITALIC ((VTL_publication_text_modification_Flags)(1u << 1))
#define TG_MOD_STRIKE ((VTL_publication_text_modification_Flags)(1u << 2))
#define TG_MOD_MASK   (TG_MOD_BOLD | TG_MOD_ITALIC | TG_MOD_STRIKE)


// собирает одну часть для MarkedText (без выделения памяти — просто заполняет поля)
static VTL_publication_marked_text_Part VTL_telegram_make_part(const char* text_start,
    size_t length, VTL_publication_text_modification_Flags flags)
{
    VTL_publication_marked_text_Part p;
    p.text = (VTL_publication_text_Symbol*)text_start;
    p.length = length;
    p.type = flags;
    return p;
}

VTL_AppResult VTL_telegram_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_telegram_MarkerList* markers,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !src->text || !markers || !out) return VTL_res_kInvalidParamErr;

    VTL_publication_MarkedText* block =
        (VTL_publication_MarkedText*)malloc(sizeof(VTL_publication_MarkedText));
    if (!block) return VTL_res_kMemAllocErr;

    // между парами маркеров — кусок текста, плюс пролог и хвост.
    // верхний предел: маркеров * 2 (текст до + после каждого) + 2 (граничные)
    size_t max_parts = markers->length * 2 + 2;
    if (max_parts == 0) max_parts = 1;
    block->parts = (VTL_publication_marked_text_Part*)malloc(
        max_parts * sizeof(VTL_publication_marked_text_Part));
    if (!block->parts) { free(block); return VTL_res_kMemAllocErr; }
    block->length = 0;

    VTL_publication_text_modification_Flags flags = 0;
    size_t cursor = 0;  // докуда уже записали

    for (size_t i = 0; i < markers->length; ++i) {
        const VTL_telegram_Marker* m = &markers->items[i];

        // маркер внутри уже обработанного куска — пропускаем
        // (страховка от пересечений между потоками-сканерами)
        if (m->pos < cursor) continue;

        // кусок обычного текста перед маркером
        if (m->pos > cursor) {
            block->parts[block->length++] = VTL_telegram_make_part(
                src->text + cursor, m->pos - cursor, flags);
        }

        // переключаем флаги в зависимости от типа маркера.
        // используем локальные TG_MOD_* (в скобках) — см. примечание сверху файла
        switch (m->kind) {
            case VTL_telegram_marker_kBoldStart:   flags |=  TG_MOD_BOLD;   break;
            case VTL_telegram_marker_kBoldEnd:     flags &= ~TG_MOD_BOLD;   break;
            case VTL_telegram_marker_kItalicStart: flags |=  TG_MOD_ITALIC; break;
            case VTL_telegram_marker_kItalicEnd:   flags &= ~TG_MOD_ITALIC; break;
            case VTL_telegram_marker_kStrikeStart: flags |=  TG_MOD_STRIKE; break;
            case VTL_telegram_marker_kStrikeEnd:   flags &= ~TG_MOD_STRIKE; break;
        }

        cursor = m->pos + m->length;
    }

    // хвост после последнего маркера
    if (cursor < src->length) {
        block->parts[block->length++] = VTL_telegram_make_part(
            src->text + cursor, src->length - cursor, flags);
    }

    *out = block;
    return VTL_res_kOk;
}


// ============================================================
// обратное направление: MarkedText → Telegram MD
// ============================================================

// нужно ли экранировать символ внутри текста Telegram MD.
// Минимально: разметочные * _ ~ и сам backslash. Иначе получатель
// интерпретирует их как разметку и пользователь увидит не то.
static int VTL_telegram_needs_escape(char c)
{
    return c == '*' || c == '_' || c == '~' || c == '\\';
}

// сколько байт нужно на сериализацию одной части (с учётом экранирования и тегов).
// Верхняя оценка — она же используется для одной аллокации без realloc'ов.
static size_t VTL_telegram_part_max_size(const VTL_publication_marked_text_Part* p)
{
    // в худшем случае каждый символ удваивается (экранирование),
    // плюс открывающие и закрывающие теги (3 символа максимум каждого направления)
    return p->length * 2u + 6u;
}

// дописывает в dest закрывающие токены для флагов, которые "уходят".
// Возвращает новый указатель конца.
static char* VTL_telegram_write_closing(char* dest,
    VTL_publication_text_modification_Flags from,
    VTL_publication_text_modification_Flags to)
{
    // закрываем в обратном порядке открытия: Strike → Italic → Bold
    if ((from & TG_MOD_STRIKE) && !(to & TG_MOD_STRIKE)) *dest++ = '~';
    if ((from & TG_MOD_ITALIC) && !(to & TG_MOD_ITALIC)) *dest++ = '_';
    if ((from & TG_MOD_BOLD)   && !(to & TG_MOD_BOLD))   *dest++ = '*';
    return dest;
}

// дописывает открывающие токены для флагов, которые "приходят".
// Порядок: Bold → Italic → Strike (симметрично закрывающим)
static char* VTL_telegram_write_opening(char* dest,
    VTL_publication_text_modification_Flags from,
    VTL_publication_text_modification_Flags to)
{
    if (!(from & TG_MOD_BOLD)   && (to & TG_MOD_BOLD))   *dest++ = '*';
    if (!(from & TG_MOD_ITALIC) && (to & TG_MOD_ITALIC)) *dest++ = '_';
    if (!(from & TG_MOD_STRIKE) && (to & TG_MOD_STRIKE)) *dest++ = '~';
    return dest;
}

// копирует текст части в dest, экранируя спецсимволы. Возвращает новый конец.
static char* VTL_telegram_write_escaped(char* dest, const char* src, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        char c = src[i];
        if (VTL_telegram_needs_escape(c)) {
            *dest++ = '\\';
        }
        *dest++ = c;
    }
    return dest;
}


VTL_AppResult VTL_telegram_SerializeFromMarkedText(const VTL_publication_MarkedText* src,
                                                    VTL_publication_Text** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    // считаем верхнюю оценку размера одним проходом — одна аллокация
    size_t cap = 1;  // под завершающий '\0'
    for (size_t i = 0; i < src->length; ++i) {
        cap += VTL_telegram_part_max_size(&src->parts[i]);
    }

    char* buf = (char*)malloc(cap);
    if (!buf) return VTL_res_kMemAllocErr;

    char* dest = buf;
    VTL_publication_text_modification_Flags current = 0;

    for (size_t i = 0; i < src->length; ++i) {
        const VTL_publication_marked_text_Part* p = &src->parts[i];
        VTL_publication_text_modification_Flags target = p->type & TG_MOD_MASK;

        // закрыть то, что больше не нужно, потом открыть то, что добавилось
        dest = VTL_telegram_write_closing(dest, current, target);
        dest = VTL_telegram_write_opening(dest, current, target);
        current = target;

        // сам текст — с экранированием спецсимволов
        if (p->text && p->length > 0) {
            dest = VTL_telegram_write_escaped(dest, (const char*)p->text, p->length);
        }
    }
    // закрываем всё, что осталось открытым
    dest = VTL_telegram_write_closing(dest, current, 0);

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
