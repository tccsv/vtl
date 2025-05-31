#include <VTL/publication/text/VTL_publication_text_op.h>


static VTL_AppResult VTL_publication_text_InitFromStandartMD(VTL_publication_MarkedText** pp_publication, 
                                                       const VTL_publication_Text* p_src_text)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromTelegramMD(VTL_publication_MarkedText** pp_publication, 
                                                        const VTL_publication_Text* p_src_text)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromHTML(VTL_publication_MarkedText** pp_publication, 
                                                    const VTL_publication_Text* p_src_text)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromBB(VTL_publication_MarkedText** pp_publication, 
                                                const VTL_publication_Text* p_src_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_Init(VTL_publication_MarkedText **pp_marked_text, 
                                                const VTL_publication_Text *p_src_text, 
                                                const VTL_publication_marked_text_MarkupType src_markup_type)
{
    if(src_markup_type == VTL_markup_type_kStandartMD)
    {        
        return VTL_publication_text_InitFromStandartMD(pp_marked_text, p_src_text);
    }
    if(src_markup_type == VTL_markup_type_kTelegramMD)
    {
        return VTL_publication_text_InitFromTelegramMD(pp_marked_text, p_src_text);
    }
    if(src_markup_type == VTL_markup_type_kHTML)
    {
        return VTL_publication_text_InitFromHTML(pp_marked_text, p_src_text);
    }
    if(src_markup_type == VTL_markup_type_kBB)
    {
        return VTL_publication_text_InitFromBB(pp_marked_text, p_src_text);
    }
    return VTL_res_kOk;
}



VTL_AppResult VTL_publication_marked_text_TransformToRegularText(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToStandartMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToTelegramMD(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToHTML(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToBB(VTL_publication_Text** pp_out_marked_text,
                                                    const VTL_publication_MarkedText* p_src_marked_text)
{
    return VTL_res_kOk;
}