#include <VTL/publication/text/VTL_publication_text_op.h>

static VTL_AppResult VTL_publication_text_InitFromStandartMD(VTL_publication_MarkedText **pp_publication,
                                                             const VTL_publication_Text *p_src_text)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromTelegramMD(VTL_publication_MarkedText **pp_publication,
                                                             const VTL_publication_Text *p_src_text)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromHTML(VTL_publication_MarkedText **pp_publication,
                                                       const VTL_publication_Text *p_src_text)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_InitFromBB(VTL_publication_MarkedText **pp_publication,
                                                     const VTL_publication_Text *p_src_text)
{
    VTL_AppResult result = VTL_publication_text_bbcode_ValidateInputParams(pp_publication, p_src_text);
    if (result != VTL_res_kOk)
        return result;

    size_t part_count = VTL_publication_text_bbcode_CountTextParts(p_src_text);

    VTL_publication_MarkedText *block = NULL;
    result = VTL_publication_text_bbcode_AllocateTextBlock(&block, part_count);
    if (result != VTL_res_kOk)
        return result;

    const char *text_start = p_src_text->text;
    const char *text_end = p_src_text->text + p_src_text->length;
    const char *current_pos = text_start;
    VTL_publication_text_modification_Flags current_flags = 0;

    while (current_pos < text_end && result == VTL_res_kOk)
    {
        if (*current_pos == '[')
        {
            result = VTL_publication_text_bbcode_ProcessPreTagText(block, part_count, text_start, current_pos, current_flags);
            if (result == VTL_res_kOk)
            {
                VTL_publication_text_bbcode_ProcessBBTag(&current_pos, &text_start, text_end, &current_flags);
            }
        }
        else
        {
            current_pos++;
        }
    }

    if (result == VTL_res_kOk)
    {
        result = VTL_publication_text_bbcode_ProcessRemainingText(block, part_count, text_start, text_end, current_flags);
    }

    if (result != VTL_res_kOk)
    {
        free(block->parts);
        free(block);
        return result;
    }

    *pp_publication = block;
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_Init(VTL_publication_MarkedText **pp_marked_text,
                                               const VTL_publication_Text *p_src_text,
                                               const VTL_publication_marked_text_MarkupType src_markup_type)
{
    if (src_markup_type == VTL_markup_type_kStandartMD)
    {
        return VTL_publication_text_InitFromStandartMD(pp_marked_text, p_src_text);
    }
    if (src_markup_type == VTL_markup_type_kTelegramMD)
    {
        return VTL_publication_text_InitFromTelegramMD(pp_marked_text, p_src_text);
    }
    if (src_markup_type == VTL_markup_type_kHTML)
    {
        return VTL_publication_text_InitFromHTML(pp_marked_text, p_src_text);
    }
    if (src_markup_type == VTL_markup_type_kBB)
    {
        return VTL_publication_text_InitFromBB(pp_marked_text, p_src_text);
    }
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToRegularText(VTL_publication_Text **pp_out_marked_text,
                                                                 const VTL_publication_MarkedText *p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToStandartMD(VTL_publication_Text **pp_out_marked_text,
                                                                const VTL_publication_MarkedText *p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToTelegramMD(VTL_publication_Text **pp_out_marked_text,
                                                                const VTL_publication_MarkedText *p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToHTML(VTL_publication_Text **pp_out_marked_text,
                                                          const VTL_publication_MarkedText *p_src_marked_text)
{
    return VTL_res_kOk;
}

VTL_AppResult VTL_publication_marked_text_TransformToBB(VTL_publication_Text **pp_out_marked_text,
                                                        const VTL_publication_MarkedText *p_src_marked_text)
{
    if (pp_out_marked_text == NULL || p_src_marked_text == NULL)
    {
        return VTL_res_text_io_r_kInvalidArgument;
    }

    *pp_out_marked_text = NULL;

    size_t total_length = 0;
    for (size_t i = 0; i < p_src_marked_text->length; i++)
    {
        const VTL_publication_marked_text_Part *part = &p_src_marked_text->parts[i];
        total_length += part->length + VTL_publication_text_bbcode_GetBBTagsLength(part->type);

        if (i > 0)
        {
            const VTL_publication_marked_text_Part *prev_part = &p_src_marked_text->parts[i - 1];
            if (prev_part->type == part->type)
            {
                total_length -= 7;
            }
        }
    }

    VTL_publication_Text *output = (VTL_publication_Text *)malloc(sizeof(VTL_publication_Text));
    if (output == NULL)
    {
        return VTL_res_text_io_r_kInvalidArgument;
    }

    output->text = (VTL_publication_text_Symbol *)malloc(total_length + 1);
    if (output->text == NULL)
    {
        free(output);
        return VTL_res_text_io_r_kInvalidArgument;
    }
    output->length = total_length;

    char *current_pos = output->text;
    for (size_t i = 0; i < p_src_marked_text->length; i++)
    {
        const VTL_publication_marked_text_Part *part = &p_src_marked_text->parts[i];

        if (i == 0)
        {
            current_pos = VTL_publication_text_bbcode_WriteOpeningTags(current_pos, part->type);
        }
        else
        {
            const VTL_publication_marked_text_Part *prev_part = &p_src_marked_text->parts[i - 1];

            if (part->type != prev_part->type)
            {
                current_pos = VTL_publication_text_bbcode_WriteClosingTags(current_pos, prev_part->type);
                current_pos = VTL_publication_text_bbcode_WriteOpeningTags(current_pos, part->type);
            }
        }

        memcpy(current_pos, part->text, part->length);
        current_pos += part->length;
    }

    // Закрываем теги для последней части
    if (p_src_marked_text->length > 0)
    {
        const VTL_publication_marked_text_Part *last_part = &p_src_marked_text->parts[p_src_marked_text->length - 1];
        current_pos = VTL_publication_text_bbcode_WriteClosingTags(current_pos, last_part->type);
    }

    *current_pos = '\0';
    *pp_out_marked_text = output;

    return VTL_res_kOk;
}