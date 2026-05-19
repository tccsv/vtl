#ifndef _VTL_PUBLICATION_TEXT_OP_MARKDOWN_H
#define _VTL_PUBLICATION_TEXT_OP_MARKDOWN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_data.h>
#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_scan.h>
#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown_convert.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// Standart Markdown (CommonMark + GFM ~~strike~~) → внутренний MarkedText.
// Sequential — все сканеры в одном потоке (для отладки и маленьких текстов).
// Parallel   — каждый сканер в своём потоке (pthread / WinAPI),
//              у каждого свой буфер маркеров, поэтому без локов.
VTL_AppResult VTL_markdown_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out);

VTL_AppResult VTL_markdown_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out);


// MarkedText → Standart Markdown-текст. Один поток, потому что
// сериализация — это линейная сборка, она и так O(N) и упирается в memcpy.
VTL_AppResult VTL_markdown_SerializeText(const VTL_publication_MarkedText* src,
                                         VTL_publication_Text** out);


// Замер времени двух режимов, секунды (wall-clock, не CPU-time).
// iterations — сколько прогонов усреднить.
double VTL_markdown_BenchSequential(const VTL_publication_Text* src, size_t iterations);
double VTL_markdown_BenchParallel(const VTL_publication_Text* src, size_t iterations);


#ifdef __cplusplus
}
#endif

#endif
