#ifndef _VTL_PUBLICATION_TEXT_OP_H
#define _VTL_PUBLICATION_TEXT_OP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/publication/text/infra/VTL_publication_text_gen.h>

VTL_publication_app_result VTL_publication_TextOpInit(VTL_publication_text* p_text);
void VTL_publication_TextOpFree(VTL_publication_text* p_text);

VTL_publication_app_result VTL_publication_MarkedTextInit(VTL_publication_MarkedText **pp_marked_text, 
                                                const VTL_publication_Text *p_src_text, 
                                                const VTL_publication_marked_text_MarkupType src_markup_type);

VTL_publication_app_result VTL_publication_MarkedTextTransformToRegularText(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

VTL_publication_app_result VTL_publication_MarkedTextTransformToStandartMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

VTL_publication_app_result VTL_publication_MarkedTextTransformToTelegramMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

VTL_publication_app_result VTL_publication_MarkedTextTransformToHTML(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

VTL_publication_app_result VTL_publication_MarkedTextTransformToBB(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text);

#ifdef __cplusplus
}
#endif

#endif // _VTL_PUBLICATION_TEXT_OP_H