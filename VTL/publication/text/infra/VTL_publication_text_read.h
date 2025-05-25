#ifndef _VTL_PUBLICATION_TEXT_READ_H
#define _VTL_PUBLICATION_TEXT_READ_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>
#include <VTL/VTL_publication_markup_text_flags.h>

VTL_publication_app_result VTL_publication_text_read_Init(VTL_publication_text* p_text);
void VTL_publication_text_read_Free(VTL_publication_text* p_text);

#ifdef __cplusplus
}
#endif


#endif