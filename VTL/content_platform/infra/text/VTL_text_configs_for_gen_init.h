#ifndef _VTL_CONTENT_PLATFORM_TEXT_CONFIGS_H
#define _VTL_CONTENT_PLATFORM_TEXT_CONFIGS_H


#ifdef __cplusplus
extern "C"
{
#endif


#include <VTL/content_platform/VTL_content_platform_flags_check.h>
#include <VTL/VTL_content_platform_flags.h>
#include <VTL/VTL_publication_markup_text_flags.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/VTL_app_result.h>
#include <stdbool.h>


#define VTL_CONTENT_PLATFORM_HTML_MASK (VTL_CONTENT_PLATFORM_W )
#define VTL_CONTENT_PLATFORM_STANDART_MD_MASK (VTL_CONTENT_PLATFORM_R )
#define VTL_CONTENT_PLATFORM_TELEGRAM_MD_MASK (VTL_CONTENT_PLATFORM_TG )
#define VTL_CONTENT_PLATFORM_BB_MASK (VTL_CONTENT_PLATFORM_VK)
#define VTL_CONTENT_PLATFORM_REGULAR_TEXT_MASK (VTL_CONTENT_PLATFORM_TIKTOK | VTL_CONTENT_PLATFORM_X)


#define VTL_CONTENT_PLATFORM_REGULAR_TEXT_POSTFIX "reg"
#define VTL_CONTENT_PLATFORM_HTML_POSTFIX "html"
#define VTL_CONTENT_PLATFORM_STANDART_MD_POSTFIX "s_md"
#define VTL_CONTENT_PLATFORM_TELEGRAM_MD_POSTFIX "t_md"
#define VTL_CONTENT_PLATFORM_BB_POSTFIX "bb"


#define VTL_PUBLICATION_TEXT_TYPE_STANDART_MD_INDEX VTL_markup_type_kStandartMD
#define VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD_INDEX VTL_markup_type_kTelegramMD
#define VTL_PUBLICATION_TEXT_TYPE_HTML_INDEX VTL_markup_type_kHTML
#define VTL_PUBLICATION_TEXT_TYPE_BB_INDEX VTL_markup_type_kBB
#define VTL_PUBLICATION_TEXT_TYPE_REGULAR_INDEX VTL_PUBLICATION_TEXT_TYPE_REGULAR_SHIFT


#define VTL_PUBLICATION_TEXT_TYPE_MAX_NUM (VTL_PUBLICATION_TEXT_TYPE_MAX + 1)
typedef VTL_Filename VTL_publication_marked_text_configs_FileNames[VTL_PUBLICATION_TEXT_TYPE_MAX_NUM]; 

typedef struct _VTL_publication_marked_text_Configs
{
    VTL_publication_marked_text_type_Flags flags;
    VTL_publication_marked_text_configs_FileNames file_names;
} VTL_publication_marked_text_Configs;


VTL_AppResult VTL_text_configs_for_gen_Init(VTL_publication_marked_text_Configs* p_configs,
                                                const VTL_content_platform_flags platform_flags,
                                                const VTL_Filename src_file_name);



#ifdef __cplusplus
}
#endif


#endif