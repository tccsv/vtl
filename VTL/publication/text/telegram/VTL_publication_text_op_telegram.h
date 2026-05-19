#ifndef _VTL_PUBLICATION_TEXT_OP_TELEGRAM_H
#define _VTL_PUBLICATION_TEXT_OP_TELEGRAM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_data.h>
#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_scan.h>
#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram_convert.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// Telegram MarkdownV2 → внутренний MarkedText.
// Sequential — все сканеры в одном потоке (для отладки и маленьких текстов).
// Parallel   — каждый сканер в своём потоке (pthread / WinAPI),
//              у каждого свой буфер маркеров, поэтому без локов.
VTL_AppResult VTL_telegram_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out);

VTL_AppResult VTL_telegram_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out);


// MarkedText → Telegram MarkdownV2-текст. Один поток, потому что
// сериализация — это линейная сборка, она и так O(N) и упирается в memcpy.
VTL_AppResult VTL_telegram_SerializeText(const VTL_publication_MarkedText* src,
                                         VTL_publication_Text** out);


// Замер времени двух режимов, секунды (wall-clock, не CPU-time).
// iterations — сколько прогонов усреднить.
double VTL_telegram_BenchSequential(const VTL_publication_Text* src, size_t iterations);
double VTL_telegram_BenchParallel(const VTL_publication_Text* src, size_t iterations);


#ifdef __cplusplus
}
#endif

#endif
