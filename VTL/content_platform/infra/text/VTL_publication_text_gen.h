#ifndef _VTL_PUBLICATION_TEXT_GEN_H
#define _VTL_PUBLICATION_TEXT_GEN_H

#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/publication/text/VTL_publication_text.h>
#include <VTL/content_platform/infra/text/VTL_text_configs_for_gen_init.h>




VTL_AppResult VTL_publication_marked_text_GenFiles(const VTL_Filename src_file_name, 
                                                    const VTL_publication_marked_text_MarkupType src_markup_type,
                                                    const VTL_publication_marked_text_Configs* p_configs);                   


#ifdef __cplusplus
}
#endif


#endif