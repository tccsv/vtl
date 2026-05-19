#ifndef _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_H
#define _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_data.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_scan.h>
#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_convert.h>
#include <VTL/publication/text/asciidoc/infra/VTL_publication_text_op_asciidoc_load.h>



// разбираем 15 видов разметки AsciiDoc и кладём в MarkedText
// Sequential — все сканеры в одном потоке
// Parallel — каждый сканер в своём потоке (pthread / WinAPI)
//           буферы у всех свои, поэтому локов нет
VTL_AppResult VTL_asciidoc_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out);

VTL_AppResult VTL_asciidoc_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out);


// замер времени двух режимов, секунды (wall-clock через CLOCK_MONOTONIC)
// iterations — сколько прогонов усреднить
double VTL_asciidoc_BenchSequential(const VTL_publication_Text* src, size_t iterations);
double VTL_asciidoc_BenchParallel(const VTL_publication_Text* src, size_t iterations);


#ifdef __cplusplus
}
#endif

#endif
