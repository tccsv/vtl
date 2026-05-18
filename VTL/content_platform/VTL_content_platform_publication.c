#include <VTL/content_platform/VTL_content_platform_publication.h>



VTL_AppResult VTL_content_platform_PublicateMarkedTexts(const VTL_publication_marked_text_Configs* p_configs,
    const VTL_content_platform_flags flags)
{    
    if( VTL_content_platform_flags_CheckTg(flags) )
    {
        
        VTL_content_platform_tg_marked_text_SendNow(
            p_configs->file_names[VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD_INDEX]);
    }

    if (VTL_content_platform_flags_CheckW(flags))
    {
        VTL_content_platform_tg_marked_text_SendNow(
            p_configs->file_names[VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD_INDEX]);
    }
        
    return VTL_res_kOk;
}



VTL_AppResult VTL_content_platform_PublicateAudioWithMarkedText(VTL_audio_Configs* p_audio_configs,
    VTL_audio_configs_platforms_Indices *p_indices, VTL_publication_marked_text_Configs* p_text_configs,
    const VTL_content_platform_flags flags)
{


    for(size_t i = 0; i < p_indices->length; ++i)
    {
        if( VTL_content_platform_flags_CheckTg(p_indices->data[i]) )
        {
            VTL_content_platform_tg_audio_w_marked_text_SendNow(p_audio_configs->data[i].file_name, 
               p_text_configs->file_names[VTL_PUBLICATION_TEXT_TYPE_TELEGRAM_MD_INDEX] );
        }
        // else if (VTL_content_platform_flags_CheckW(p_indices->data[i]) )
        // {
        //    VTL_content_platform_w_audio_w_marked_text_SendNow(p_audio_configs->data[i].file_name, 
        //         p_text_configs->file_names{VTL_PUBLICATION_TEXT_TYPE_HTML_INDEX} );
        // }
    }

 
        
    return VTL_res_kOk;
}