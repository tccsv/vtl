#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_convert.h>
#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_threads.h>
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


// ============================================================
// Параллельная сериализация: single-pass + compact
// ============================================================
//
// Идея:
//   1. Считаем upper-bound для каждого чанка частей (та же формула, что в
//      существующем part_max_size — не зависит от содержимого, только длины).
//   2. Один большой malloc на сумму upper-bound'ов.
//   3. K потоков ОДИН раз пишут свои чанки в общий буфер по непересекающимся
//      сегментам [upper_offset[i], upper_offset[i+1]). Между сегментами
//      остаются "дырки" (upper > actual).
//   4. memmove компактифицирует — выкидывает дырки, делая выход непрерывным.
//   5. realloc уменьшает буфер до фактического размера.

// сколько потоков использовать. 4 — разумный компромисс:
// больше потоков на 3 разные операции в parsing'е, тут — 4 равных чанка по объёму
#define VTL_TELEGRAM_SERIALIZER_THREADS 4

// порог переключения: меньше этого числа частей — однопоток.
// Эмпирически 32 части ≈ overhead pthread_create на 4 потока.
#define VTL_TELEGRAM_PARALLEL_SERIALIZE_MIN_PARTS 32


// Сериализует один чанк частей в dest (НЕ NULL), возвращает фактическую длину.
// dest должен иметь capacity >= upper-bound (см. VTL_telegram_chunk_upper_size).
//
// Семантика chunk'а:
//   [i_start, i_end) — индексы частей этого чанка в src->parts
//   initial_flags    — флаги "перед" первой частью чанка (равны target предыдущей
//                      части, или 0 если это первый чанк)
//   is_last          — это последний чанк? Если да — в конце дописываем закрытие
//                      всех ещё открытых тегов
static size_t VTL_telegram_chunk_serialize(const VTL_publication_MarkedText* src,
    size_t i_start, size_t i_end,
    VTL_publication_text_modification_Flags initial_flags,
    int is_last,
    char* dest)
{
    const char* dest_start = dest;
    VTL_publication_text_modification_Flags current = initial_flags;

    for (size_t i = i_start; i < i_end; ++i) {
        const VTL_publication_marked_text_Part* p = &src->parts[i];
        VTL_publication_text_modification_Flags target = p->type & TG_MOD_MASK;

        // закрыть то, что уходит, потом открыть то, что добавляется
        dest = VTL_telegram_write_closing(dest, current, target);
        dest = VTL_telegram_write_opening(dest, current, target);
        current = target;

        // сам текст — с экранированием спецсимволов
        if (p->text && p->length > 0) {
            dest = VTL_telegram_write_escaped(dest, (const char*)p->text, p->length);
        }
    }

    // финальное закрытие — только в самом последнем чанке.
    // Промежуточные чанки НЕ закрывают свои "хвостовые" теги —
    // следующий чанк начнётся с тем же current → diff=0, ничего лишнего.
    if (is_last) {
        dest = VTL_telegram_write_closing(dest, current, 0);
    }

    return (size_t)(dest - dest_start);
}


// Верхняя оценка длины сериализованного чанка.
// Совпадает с тем, что считает part_max_size в sequential-версии,
// плюс 3 байта на возможные финальные закрывающие теги.
static size_t VTL_telegram_chunk_upper_size(const VTL_publication_MarkedText* src,
                                             size_t i_start, size_t i_end,
                                             int is_last)
{
    size_t total = 0;
    for (size_t i = i_start; i < i_end; ++i) {
        total += VTL_telegram_part_max_size(&src->parts[i]);
    }
    if (is_last) total += 3u;  // *, _, ~ — три байта максимум на финальное закрытие
    return total;
}


// Аргументы для одного worker-потока (single-pass, dest всегда не-NULL).
typedef struct _VTL_telegram_SerializerTask {
    const VTL_publication_MarkedText* src;
    size_t i_start;
    size_t i_end;
    VTL_publication_text_modification_Flags initial_flags;
    int is_last;
    char* dest;            // сегмент общего буфера; upper-bound capacity
    size_t actual_size;    // out: фактически записано
} VTL_telegram_SerializerTask;

static void* VTL_telegram_serializer_worker(void* arg)
{
    VTL_telegram_SerializerTask* t = (VTL_telegram_SerializerTask*)arg;
    t->actual_size = VTL_telegram_chunk_serialize(
        t->src, t->i_start, t->i_end, t->initial_flags, t->is_last, t->dest);
    return NULL;
}


// Запустить K-задач в потоках. Те, на кого не хватило потоков, выполнятся
// в текущем потоке — это страховка от частичной нехватки ресурсов ОС.
static void VTL_telegram_run_workers(VTL_telegram_SerializerTask* tasks,
                                      vtl_tg_thread_t* threads, size_t K)
{
    size_t spawned = 0;
    for (size_t i = 0; i < K; ++i) {
        if (vtl_tg_thread_create(&threads[i],
                                 VTL_telegram_serializer_worker, &tasks[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < K; ++i) {
        VTL_telegram_serializer_worker(&tasks[i]);
    }
    for (size_t i = 0; i < spawned; ++i) {
        vtl_tg_thread_join(threads[i]);
    }
}


VTL_AppResult VTL_telegram_SerializeFromMarkedTextParallel(
    const VTL_publication_MarkedText* src,
    VTL_publication_Text** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    // короткий путь: на маленьких блоках overhead больше выгоды
    if (src->length < VTL_TELEGRAM_PARALLEL_SERIALIZE_MIN_PARTS) {
        return VTL_telegram_SerializeFromMarkedText(src, out);
    }

    const size_t K = VTL_TELEGRAM_SERIALIZER_THREADS;

    // 1) Разбиваем parts на K чанков примерно равной длины (по числу частей).
    //    Размер частей в реальном тексте отличается не сильно, дисбаланс минимален.
    size_t starts[VTL_TELEGRAM_SERIALIZER_THREADS + 1];
    starts[0] = 0;
    for (size_t i = 1; i < K; ++i) {
        starts[i] = (src->length * i) / K;
    }
    starts[K] = src->length;

    // 2) Считаем upper-bound для каждого чанка и общий upper.
    //    Это O(N), но один линейный проход без логики разметки — очень быстро.
    size_t upper_size[VTL_TELEGRAM_SERIALIZER_THREADS];
    size_t upper_offset[VTL_TELEGRAM_SERIALIZER_THREADS + 1];
    upper_offset[0] = 0;
    for (size_t i = 0; i < K; ++i) {
        upper_size[i] = VTL_telegram_chunk_upper_size(src, starts[i], starts[i + 1],
                                                      (i + 1 == K));
        upper_offset[i + 1] = upper_offset[i] + upper_size[i];
    }
    size_t upper_total = upper_offset[K];

    // 3) Одна аллокация upper-bound. +1 на финальный '\0'.
    char* buf = (char*)malloc(upper_total + 1);
    if (!buf) return VTL_res_kMemAllocErr;

    // 4) Подготовка задач: каждой известен СВОЙ сегмент в общем буфере.
    //    Начальные флаги = target предыдущей части (или 0 для первого чанка).
    VTL_telegram_SerializerTask tasks[VTL_TELEGRAM_SERIALIZER_THREADS];
    for (size_t i = 0; i < K; ++i) {
        tasks[i].src = src;
        tasks[i].i_start = starts[i];
        tasks[i].i_end = starts[i + 1];
        tasks[i].initial_flags = (starts[i] == 0)
            ? (VTL_publication_text_modification_Flags)0
            : (src->parts[starts[i] - 1].type & TG_MOD_MASK);
        tasks[i].is_last = (i + 1 == K);
        tasks[i].dest = buf + upper_offset[i];
        tasks[i].actual_size = 0;
    }

    // 5) Single-pass — параллельная запись в свои сегменты. Один batch потоков.
    vtl_tg_thread_t threads[VTL_TELEGRAM_SERIALIZER_THREADS];
    VTL_telegram_run_workers(tasks, threads, K);

    // 6) Компактификация: убираем дырки между чанками одним проходом memmove.
    //    Первый чанк остаётся на месте (его dest = buf + 0).
    //    Дальше каждый чанк двигаем вплотную к концу уже скомпактованного.
    char* compact_end = buf + tasks[0].actual_size;
    for (size_t i = 1; i < K; ++i) {
        // источники не пересекаются с предыдущим compact-куском (дырки),
        // но мы всё равно используем memmove, чтобы не зависеть от этого
        memmove(compact_end, buf + upper_offset[i], tasks[i].actual_size);
        compact_end += tasks[i].actual_size;
    }
    size_t total = (size_t)(compact_end - buf);
    buf[total] = '\0';

    // 7) Опционально: realloc до фактического размера. Если упадёт — не страшно,
    //    оставим исходный buf (он рабочий, просто чуть больше нужного).
    char* shrunk = (char*)realloc(buf, total + 1);
    if (shrunk) buf = shrunk;

    VTL_publication_Text* result =
        (VTL_publication_Text*)calloc(1, sizeof(VTL_publication_Text));
    if (!result) { free(buf); return VTL_res_kMemAllocErr; }
    result->text = (VTL_publication_text_Symbol*)buf;
    result->length = total;
    *out = result;
    return VTL_res_kOk;
}
