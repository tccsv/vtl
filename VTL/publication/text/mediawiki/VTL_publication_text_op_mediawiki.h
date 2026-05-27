#ifndef _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_H
#define _VTL_PUBLICATION_TEXT_OP_MEDIAWIKI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_data.h>
#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_scan.h>
#include <VTL/publication/text/mediawiki/VTL_publication_text_op_mediawiki_convert.h>


VTL_AppResult VTL_mediawiki_ParseTextSequential(const VTL_publication_Text* src,
                                                 VTL_publication_MarkedText** out);

VTL_AppResult VTL_mediawiki_ParseTextParallel(const VTL_publication_Text* src,
                                               VTL_publication_MarkedText** out);


VTL_AppResult VTL_mediawiki_EmitText(const VTL_publication_MarkedText* src,
                                      VTL_publication_Text** out);


double VTL_mediawiki_BenchSequential(const VTL_publication_Text* src, size_t iterations);
double VTL_mediawiki_BenchParallel(const VTL_publication_Text* src, size_t iterations);


#ifdef __cplusplus
}
#endif

#endif
