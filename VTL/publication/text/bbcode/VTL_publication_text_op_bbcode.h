#ifndef _VTL_PUBLICATION_TEXT_OP_BBCODE_H
#define _VTL_PUBLICATION_TEXT_OP_BBCODE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/VTL_app_result.h>
#include <stdlib.h>
#include <string.h>

    size_t VTL_publication_text_bbcode_CountTextParts(const VTL_publication_Text *p_src_text);

    void VTL_publication_text_bbcode_ProcessTag(const char *tag_start, const char *tag_end,
                                                VTL_publication_text_modification_Flags *current_flags);

    VTL_AppResult VTL_publication_text_bbcode_AddTextPart(VTL_publication_MarkedText *block,
                                                          size_t max_parts,
                                                          const char *text_start,
                                                          size_t text_length,
                                                          VTL_publication_text_modification_Flags flags);

    VTL_AppResult VTL_publication_text_bbcode_ValidateInputParams(VTL_publication_MarkedText **pp_publication,
                                                                  const VTL_publication_Text *p_src_text);

    VTL_AppResult VTL_publication_text_bbcode_AllocateTextBlock(VTL_publication_MarkedText **block, size_t part_count);

    VTL_AppResult VTL_publication_text_bbcode_ProcessPreTagText(VTL_publication_MarkedText *block,
                                                                size_t part_count,
                                                                const char *text_start,
                                                                const char *tag_pos,
                                                                VTL_publication_text_modification_Flags flags);

    void VTL_publication_text_bbcode_ProcessBBTag(const char **current_pos,
                                                  const char **text_start,
                                                  const char *text_end,
                                                  VTL_publication_text_modification_Flags *current_flags);

    VTL_AppResult VTL_publication_text_bbcode_ProcessRemainingText(VTL_publication_MarkedText *block,
                                                                   size_t part_count,
                                                                   const char *text_start,
                                                                   const char *text_end,
                                                                   VTL_publication_text_modification_Flags flags);

    int VTL_publication_text_bbcode_NeedsBBTag(VTL_publication_text_modification_Flags type, int tag_flag);

    size_t VTL_publication_text_bbcode_GetBBTagsLength(VTL_publication_text_modification_Flags type);

    char *VTL_publication_text_bbcode_WriteOpeningTags(char *dest, VTL_publication_text_modification_Flags type);

    char *VTL_publication_text_bbcode_WriteClosingTags(char *dest, VTL_publication_text_modification_Flags type);

    bool VTL_publication_text_bbcode_TagsEqual(int flags1, int flags2);

#ifdef __cplusplus
}
#endif

#endif 