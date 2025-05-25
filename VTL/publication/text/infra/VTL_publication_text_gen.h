#ifndef _VTL_PUBLICATION_TEXT_GEN_H
#define _VTL_PUBLICATION_TEXT_GEN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_publication_markup_text_flags.h>

VTL_publication_app_result VTL_publication_text_gen_Init(VTL_publication_marked_text* p_marked_text);
void VTL_publication_text_gen_Free(VTL_publication_marked_text* p_marked_text);

#ifdef __cplusplus
}
#endif

#endif