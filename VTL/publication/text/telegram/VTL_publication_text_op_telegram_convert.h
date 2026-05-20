#ifndef _VTL_PUBLICATION_TEXT_OP_TELEGRAM_CONVERT_H
#define _VTL_PUBLICATION_TEXT_OP_TELEGRAM_CONVERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// Telegram MD → внутренний формат.
// Проходит по отсортированным маркерам и нарезает текст на части с
// флагами BOLD/ITALIC/STRIKETHROUGH. Сами токены разметки (*, _, ~)
// из выходного текста выкидываются.
VTL_AppResult VTL_telegram_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_telegram_MarkerList* markers,
                                                VTL_publication_MarkedText** out);

// Внутренний формат → Telegram MD (однопоточная версия).
// Для каждой части смотрит разницу флагов с предыдущей и дописывает
// соответствующие закрывающие/открывающие токены.
// Символы '*' '_' '~' и backslash внутри текста экранируются обратным слешем.
VTL_AppResult VTL_telegram_SerializeFromMarkedText(const VTL_publication_MarkedText* src,
                                                    VTL_publication_Text** out);


// То же, но в параллельном режиме (single-pass + compact).
// Архитектурно:
//   1. Считаем upper-bound длину выхода для каждого чанка частей за O(K)
//      (формула та же, что у уже существующей part_max_size).
//   2. Аллоцируем один общий буфер upper_total — один malloc, без per-chunk
//      аллокаций.
//   3. Каждый поток ОДИН раз пишет свой чанк в свой сегмент общего буфера
//      (стартовая позиция = prefix sum upper-bound'ов). Сегменты не пересекаются,
//      между ними остаются "дырки" — потому что upper-bound > actual size.
//   4. Главный поток последовательно memmove-ит чанки вплотную и делает
//      realloc до фактического размера.
// Каждый поток знает начальные флаги через target предыдущей части
// (вычисляется однократно в главном потоке за O(K), где K — число потоков).
//
// На малых текстах (parts->length < ~32) автоматически делегирует sequential —
// overhead pthread_create перевешивает выгоду.
// Результат побайтно идентичен sequential-версии.
VTL_AppResult VTL_telegram_SerializeFromMarkedTextParallel(
    const VTL_publication_MarkedText* src,
    VTL_publication_Text** out);


#ifdef __cplusplus
}
#endif

#endif
