#include <VTL/publication/text/VTL_publication_text_op.h>
#include <VTL/publication/text/telegram/VTL_publication_text_op_telegram.h>
#include <VTL/publication/text/markdown/VTL_publication_text_op_markdown.h>
#include <stdlib.h>
#include <string.h>

static VTL_AppResult vtl_marked_text_alloc_single_part(VTL_publication_MarkedText** pp_out,
                                                        const VTL_publication_Text* p_src,
                                                        VTL_publication_text_modification_Flags type)
{
    if (!pp_out || !p_src) return VTL_res_kErr;
    *pp_out = (VTL_publication_MarkedText*)calloc(1, sizeof(VTL_publication_MarkedText));
    if (!*pp_out) return VTL_res_kErr;
    (*pp_out)->parts = (VTL_publication_marked_text_Part*)calloc(1, sizeof(VTL_publication_marked_text_Part));
    if (!(*pp_out)->parts) { free(*pp_out); return VTL_res_kErr; }
    (*pp_out)->length = 1;
    (*pp_out)->parts[0].text = p_src->text;
    (*pp_out)->parts[0].length = p_src->length;
    (*pp_out)->parts[0].type = type;
    return VTL_res_kOk;
}

static VTL_AppResult vtl_text_alloc_from_marked(VTL_publication_Text** pp_out,
                                                 const VTL_publication_MarkedText* p_src)
{
    if (!pp_out || !p_src) return VTL_res_kErr;
    size_t total = 0;
    for (size_t i = 0; i < p_src->length; ++i) total += p_src->parts[i].length;
    *pp_out = (VTL_publication_Text*)calloc(1, sizeof(VTL_publication_Text));
    if (!*pp_out) return VTL_res_kErr;
    (*pp_out)->text = (VTL_publication_text_Symbol*)malloc(total + 1);
    if (!(*pp_out)->text) { free(*pp_out); return VTL_res_kErr; }
    size_t pos = 0;
    for (size_t i = 0; i < p_src->length; ++i) {
        memcpy((*pp_out)->text + pos, p_src->parts[i].text, p_src->parts[i].length);
        pos += p_src->parts[i].length;
    }
    (*pp_out)->text[total] = 0;
    (*pp_out)->length = total;
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_InitFromStandartMD(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{
    // парсим Standart Markdown (CommonMark + GFM ~~strike~~) в параллельном режиме —
    // три сканера (bold/italic/strike) бегут каждый в своём потоке со своим
    // буфером маркеров, без локов. Sequential доступен отдельно через VTL_markdown_*.
    return VTL_markdown_ParseTextParallel(p_src_text, pp_marked_text);
}

VTL_AppResult VTL_publication_marked_text_InitFromTelegramMD(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{
    // парсим Telegram MarkdownV2 в параллельном режиме —
    // у каждого из трёх сканеров (bold/italic/strike) свой буфер маркеров,
    // что даёт ускорение на больших текстах. Sequential доступен отдельно.
    return VTL_telegram_ParseTextParallel(p_src_text, pp_marked_text);
}

VTL_AppResult VTL_publication_marked_text_InitFromHTML(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{ return vtl_marked_text_alloc_single_part(pp_marked_text, p_src_text, 0); }

VTL_AppResult VTL_publication_marked_text_InitFromBB(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{ return vtl_marked_text_alloc_single_part(pp_marked_text, p_src_text, 0); }

VTL_AppResult VTL_publication_marked_text_InitFromRegularText(
    VTL_publication_MarkedText** pp_marked_text, const VTL_publication_Text* p_src_text)
{ return vtl_marked_text_alloc_single_part(pp_marked_text, p_src_text, 0); }

VTL_AppResult VTL_publication_marked_text_Init(
    VTL_publication_MarkedText** pp_marked_text,
    const VTL_publication_Text* p_src_text,
    const VTL_publication_marked_text_MarkupType src_markup_type)
{
    switch(src_markup_type) {
        case VTL_markup_type_kStandartMD:
            return VTL_publication_marked_text_InitFromStandartMD(pp_marked_text, p_src_text);
        case VTL_markup_type_kTelegramMD:
            return VTL_publication_marked_text_InitFromTelegramMD(pp_marked_text, p_src_text);
        case VTL_markup_type_kHTML:
            return VTL_publication_marked_text_InitFromHTML(pp_marked_text, p_src_text);
        case VTL_markup_type_kBB:
            return VTL_publication_marked_text_InitFromBB(pp_marked_text, p_src_text);
        default:
            return VTL_publication_marked_text_InitFromRegularText(pp_marked_text, p_src_text);
    }
}

VTL_AppResult VTL_publication_marked_text_TransformToStandartMD(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{
    // сериализация выдаёт канонический CommonMark-синтаксис:
    // ** для bold, * для italic, ~~ для strike (GFM).
    // Линейная по символам, расспараллеливать смысла нет — упирается в memcpy.
    return VTL_markdown_SerializeText(p_marked_text, pp_text);
}

VTL_AppResult VTL_publication_marked_text_TransformToTelegramMD(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{
    // сериализация — линейная по символам, расспараллеливать смысла нет
    // (упрётся в memcpy и пропускную способность памяти)
    return VTL_telegram_SerializeText(p_marked_text, pp_text);
}

VTL_AppResult VTL_publication_marked_text_TransformToHTML(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{ return vtl_text_alloc_from_marked(pp_text, p_marked_text); }

VTL_AppResult VTL_publication_marked_text_TransformToBB(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{ return vtl_text_alloc_from_marked(pp_text, p_marked_text); }

VTL_AppResult VTL_publication_marked_text_TransformToRegularText(
    VTL_publication_Text** pp_text, const VTL_publication_MarkedText* p_marked_text)
{ return vtl_text_alloc_from_marked(pp_text, p_marked_text); }
