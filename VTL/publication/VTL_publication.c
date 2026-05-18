#include <VTL/publication/VTL_publication.h>




VTL_AppResult VTL_PubicateMarkedText(const VTL_Filename file_name, const VTL_content_platform_flags flags,
                                        const VTL_publication_marked_text_MarkupType markup_type)
{
    VTL_AppResult app_result = VTL_res_kOk;
   
    VTL_publication_marked_text_Configs marked_text_configs = {0};
    app_result = VTL_text_configs_for_gen_Init(&marked_text_configs, flags, file_name);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    app_result = VTL_publication_marked_text_GenFiles(file_name, markup_type, &marked_text_configs);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    app_result = VTL_content_platform_PublicateMarkedTexts(&marked_text_configs, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    app_result = VTL_user_history_SaveTextPublication(file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return app_result;
}

VTL_AppResult VTL_PubicateAudioWithMarkedText(const VTL_Filename audio_file_name, 
                                        const VTL_Filename text_file_name, 
                                        const VTL_publication_marked_text_MarkupType markup_type,
                                        const VTL_content_platform_flags flags)
{
    VTL_AppResult app_result = VTL_res_kOk;
    
    VTL_publication_marked_text_Configs marked_text_configs = {0};  
    VTL_audio_Configs* p_audio_configs = {0};
    VTL_audio_configs_platforms_Indices* p_indices = {0};
    
    
    app_result = VTL_text_configs_for_gen_Init(&marked_text_configs, flags, text_file_name);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    
    app_result = VTL_publication_marked_text_GenFiles(text_file_name, markup_type, &marked_text_configs);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    
    app_result = VTL_audio_configs_for_gen_Init(&p_audio_configs, &p_indices, audio_file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    return app_result;
}