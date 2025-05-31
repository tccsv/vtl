#ifndef _VTL_PUBLICATION_TEXT_OP_H
#define _VTL_PUBLICATION_TEXT_OP_H


#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/VTL_app_result.h>
#include <stdlib.h>


VTL_AppResult VTL_publication_marked_text_Init(VTL_publication_MarkedText **pp_marked_text, 
                                                const VTL_publication_Text *p_src_text, 
                                                const VTL_publication_marked_text_MarkupType src_markup_type);

VTL_AppResult VTL_publication_marked_text_TransformToRegularText(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);


VTL_AppResult VTL_publication_marked_text_TransformToStandartMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

VTL_AppResult VTL_publication_marked_text_TransformToTelegramMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

VTL_AppResult VTL_publication_marked_text_TransformToHTML(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

VTL_AppResult VTL_publication_marked_text_TransformToBB(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

#ifdef __cplusplus
}
#endif


#endif