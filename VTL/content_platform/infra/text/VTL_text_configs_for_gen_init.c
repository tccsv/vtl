#include <VTL/content_platform/infra/text/VTL_text_configs_for_gen_init.h>

static bool VTL_content_platform_flags_CheckRegularText(const VTL_content_platform_flags content_platform_flags)
{
    return VTL_CONTENT_PLATFORM_REGULAR_TEXT_MASK & content_platform_flags != 0;
}

static bool VTL_content_platform_flags_CheckHTML(const VTL_content_platform_flags content_platform_flags)
{
    return VTL_CONTENT_PLATFORM_HTML_MASK & content_platform_flags != 0;
}

static bool VTL_content_platform_flags_CheckStandartMD(const VTL_content_platform_flags content_platform_flags)
{
    return VTL_CONTENT_PLATFORM_STANDART_MD_MASK & content_platform_flags != 0;
}

static bool VTL_content_platform_flags_CheckTelegramMD(const VTL_content_platform_flags content_platform_flags)
{
    return VTL_CONTENT_PLATFORM_TELEGRAM_MD_MASK & content_platform_flags != 0;
}

static bool VTL_content_platform_flags_CheckBB(const VTL_content_platform_flags content_platform_flags)
{
    return VTL_CONTENT_PLATFORM_BB_MASK & content_platform_flags != 0;
}

static void VTL_content_platform_text_InitFlags(VTL_publication_marked_text_type_Flags* p_text_type_flags,
                                                            const VTL_content_platform_flags content_platform_flags)
{
    *p_text_type_flags = VTL_PUBLICATION_TEXT_TYPE_STANDART_MD * VTL_content_platform_flags_CheckStandartMD(content_platform_flags)
                        | VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD * VTL_content_platform_flags_CheckTelegramMD(content_platform_flags)
                        | VTL_PUBLICATION_TEXT_TYPE_REGULAR * VTL_content_platform_flags_CheckHTML(content_platform_flags)
                        | VTL_PUBLICATION_TEXT_TYPE_HTML * VTL_content_platform_flags_CheckHTML(content_platform_flags)
                        | VTL_PUBLICATION_TEXT_TYPE_BB * VTL_content_platform_flags_CheckHTML(content_platform_flags);
}

static VTL_AppResult VTL_content_platform_text_InitStandartMDFileName(VTL_Filename out_file_name, 
                                                                                const VTL_Filename src_file_name)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_content_platform_text_InitTelegramMDFileName(VTL_Filename out_file_name, 
                                                                                const VTL_Filename src_file_name)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_content_platform_text_InitRegularTextFileName(VTL_Filename out_file_name, 
                                                                                const VTL_Filename src_file_name)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_content_platform_text_InitHTMLFileName(VTL_Filename out_file_name, 
                                                                                const VTL_Filename src_file_name)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_content_platform_text_InitBBFileName(VTL_Filename out_file_name, 
                                                                                const VTL_Filename src_file_name)
{
    return VTL_res_kOk;
}

static VTL_AppResult VTL_content_platform_text_InitFileNames(
                                                VTL_publication_marked_text_configs_FileNames arr_file_names,
                                                const VTL_content_platform_flags content_platform_flags,
                                                const VTL_Filename src_file_name)
{
    VTL_AppResult app_result = VTL_res_kOk;
    if(VTL_content_platform_flags_CheckRegularText(content_platform_flags))
    {
        app_result = VTL_content_platform_text_InitRegularTextFileName(
            arr_file_names[VTL_PUBLICATION_TEXT_TYPE_REGULAR_INDEX], src_file_name);
    }
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    if(VTL_content_platform_flags_CheckStandartMD(content_platform_flags))
    {
        app_result = VTL_content_platform_text_InitStandartMDFileName(
            arr_file_names[VTL_PUBLICATION_TEXT_TYPE_STANDART_MD_INDEX], src_file_name);
    }
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    if(VTL_content_platform_flags_CheckTelegramMD(content_platform_flags))
    {
        app_result = VTL_content_platform_text_InitTelegramMDFileName(
            arr_file_names[VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD_INDEX], src_file_name);
    }
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    
    if(VTL_content_platform_flags_CheckHTML(content_platform_flags))
    {
        app_result = VTL_content_platform_text_InitHTMLFileName(
            arr_file_names[VTL_PUBLICATION_TEXT_TYPE_HTML_INDEX], src_file_name);
    }
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    
    if(VTL_content_platform_flags_CheckBB(content_platform_flags))
    {
    app_result = VTL_content_platform_text_InitBBFileName(
            arr_file_names[VTL_PUBLICATION_TEXT_TYPE_BB_INDEX], src_file_name);
    }
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    } 
    return app_result;
}

VTL_AppResult VTL_text_configs_for_gen_Init(VTL_publication_marked_text_Configs* p_configs,
                                                const VTL_content_platform_flags platform_flags,
                                                const VTL_Filename src_file_name)
{
    VTL_content_platform_text_InitFlags(&p_configs->flags, platform_flags);
    VTL_content_platform_text_InitFileNames(p_configs->file_names, platform_flags, src_file_name);
    return VTL_res_kOk;
}