#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/VTL_app_result.h>
#include <stdlib.h>
#include <string.h>

static size_t VTL_publication_text_bbcode_CountTextParts(const VTL_publication_Text *p_src_text)
{
    size_t part_count = 1;
    bool in_tag = false;

    for (size_t i = 0; i < p_src_text->length; i++)
    {
        if (p_src_text->text[i] == '[' && !in_tag)
        {
            in_tag = true;
            part_count++;
        }
        else if (p_src_text->text[i] == ']' && in_tag)
        {
            in_tag = false;
        }
    }

    return part_count;
}

static void VTL_publication_text_bbcode_ProcessTag(const char *tag_start, const char *tag_end,
                                                   VTL_publication_text_modification_Flags *current_flags)
{
    bool is_closing = (*tag_start == '/');
    const char *tag_name = is_closing ? tag_start + 1 : tag_start;
    size_t tag_name_len = tag_end - tag_name;

    if (tag_name_len == 1)
    {
        if (*tag_name == 'b')
        {
            if (is_closing)
                *current_flags &= ~VTL_TEXT_MODIFICATION_BOLD;
            else
                *current_flags |= VTL_TEXT_MODIFICATION_BOLD;
        }
        else if (*tag_name == 'i')
        {
            if (is_closing)
                *current_flags &= ~VTL_TEXT_MODIFICATION_ITALIC;
            else
                *current_flags |= VTL_TEXT_MODIFICATION_ITALIC;
        }
        else if (*tag_name == 's')
        {
            if (is_closing)
                *current_flags &= ~VTL_TEXT_MODIFICATION_STRIKETHROUGH;
            else
                *current_flags |= VTL_TEXT_MODIFICATION_STRIKETHROUGH;
        }
    }
}

static VTL_AppResult VTL_publication_text_bbcode_AddTextPart(VTL_publication_MarkedText *block,
                                                             size_t max_parts,
                                                             const char *text_start,
                                                             size_t text_length,
                                                             VTL_publication_text_modification_Flags flags)
{
    if (block->length >= max_parts)
    {
        return VTL_res_text_io_r_kInvalidArgument;
    }

    block->parts[block->length].text = (VTL_publication_text_Symbol *)text_start;
    block->parts[block->length].length = text_length;
    block->parts[block->length].type = flags;
    block->length++;

    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_bbcode_ValidateInputParams(VTL_publication_MarkedText **pp_publication,
                                                                     const VTL_publication_Text *p_src_text)
{
    if (!pp_publication)
        return VTL_res_text_io_r_kInvalidArgument;
    if (!p_src_text)
        return VTL_res_text_io_r_kInvalidArgument;
    if (!p_src_text->text)
        return VTL_res_text_io_r_kInvalidArgument;
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_bbcode_AllocateTextBlock(VTL_publication_MarkedText **block, size_t part_count)
{
    *block = (VTL_publication_MarkedText *)malloc(sizeof(VTL_publication_MarkedText));
    if (!*block)
        return VTL_res_text_ms_w_kOutOfMemory;

    (*block)->parts = (VTL_publication_marked_text_Part *)malloc(part_count *
                                                                 sizeof(VTL_publication_marked_text_Part));
    if (!(*block)->parts)
    {
        free(*block);
        return VTL_res_text_ms_w_kOutOfMemory;
    }

    (*block)->length = 0;
    return VTL_res_kOk;
}

static VTL_AppResult VTL_publication_text_bbcode_ProcessPreTagText(VTL_publication_MarkedText *block,
                                                                   size_t part_count,
                                                                   const char *text_start,
                                                                   const char *tag_pos,
                                                                   VTL_publication_text_modification_Flags flags)
{
    size_t text_length = tag_pos - text_start;
    if (text_length > 0)
    {
        return VTL_publication_text_bbcode_AddTextPart(block, part_count, text_start, text_length, flags);
    }
    return VTL_res_kOk;
}

static void VTL_publication_text_bbcode_ProcessBBTag(const char **current_pos,
                                                     const char **text_start,
                                                     const char *text_end,
                                                     VTL_publication_text_modification_Flags *current_flags)
{
    const char *tag_start = *current_pos + 1;
    const char *tag_end = strchr(tag_start, ']');

    if (tag_end && tag_end < text_end)
    {
        VTL_publication_text_bbcode_ProcessTag(tag_start, tag_end, current_flags);
        *current_pos = tag_end + 1;
        *text_start = *current_pos;
    }
    else
    {
        (*current_pos)++;
        *text_start = *current_pos;
    }
}

static VTL_AppResult VTL_publication_text_bbcode_ProcessRemainingText(VTL_publication_MarkedText *block,
                                                                      size_t part_count,
                                                                      const char *text_start,
                                                                      const char *text_end,
                                                                      VTL_publication_text_modification_Flags flags)
{
    size_t text_length = text_end - text_start;
    if (text_length > 0)
    {
        return VTL_publication_text_bbcode_AddTextPart(block, part_count, text_start, text_length, flags);
    }
    return VTL_res_kOk;
}

static int VTL_publication_text_bbcode_NeedsBBTag(VTL_publication_text_modification_Flags type, int tag_flag)
{
    return (type & tag_flag) != 0;
}

static size_t VTL_publication_text_bbcode_GetBBTagsLength(VTL_publication_text_modification_Flags type)
{
    size_t length = 0;

    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_BOLD))
        length += 7;
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_ITALIC))
        length += 7;
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_STRIKETHROUGH))
        length += 7;

    return length;
}

static char *VTL_publication_text_bbcode_WriteOpeningTags(char *dest, VTL_publication_text_modification_Flags type)
{
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_BOLD))
    {
        strcpy(dest, "[b]");
        dest += 3;
    }
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_ITALIC))
    {
        strcpy(dest, "[i]");
        dest += 3;
    }
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_STRIKETHROUGH))
    {
        strcpy(dest, "[s]");
        dest += 3;
    }

    return dest;
}

static char *VTL_publication_text_bbcode_WriteClosingTags(char *dest, VTL_publication_text_modification_Flags type)
{
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_STRIKETHROUGH))
    {
        strcpy(dest, "[/s]");
        dest += 4;
    }
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_ITALIC))
    {
        strcpy(dest, "[/i]");
        dest += 4;
    }
    if (VTL_publication_text_bbcode_NeedsBBTag(type, VTL_TEXT_MODIFICATION_BOLD))
    {
        strcpy(dest, "[/b]");
        dest += 4;
    }

    return dest;
}

static bool VTL_publication_text_bbcode_TagsEqual(int flags1, int flags2)
{
    return (flags1 & (VTL_TEXT_MODIFICATION_BOLD |
                      VTL_TEXT_MODIFICATION_ITALIC |
                      VTL_TEXT_MODIFICATION_STRIKETHROUGH)) ==
           (flags2 & (VTL_TEXT_MODIFICATION_BOLD |
                      VTL_TEXT_MODIFICATION_ITALIC |
                      VTL_TEXT_MODIFICATION_STRIKETHROUGH));
}