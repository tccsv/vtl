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

VTL_AppResult VTL_SheduleMarkedText(const VTL_Filename file_name, const VTL_content_platform_flags flags,
                                        const VTL_publication_marked_text_MarkupType markup_type, 
                                        const VTL_Time sheduled_time)
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

    VTL_UserHistory history = {0};
    app_result = VTL_user_history_text_pubication_InitSheduled(&history, NULL, file_name, flags, sheduled_time);
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

VTL_AppResult VTL_SheduleText(const VTL_Filename file_name, const VTL_content_platform_flags flags)
{
    return VTL_SheduleMarkedText(file_name, flags, VTL_markup_type_kRegular, VTL_user_history_GetCurrentTime());
}

VTL_AppResult VTL_PubicateText(const VTL_Filename file_name, const VTL_content_platform_flags flags)
{
    return VTL_PubicateMarkedText(file_name, flags, VTL_markup_type_kRegular);
}

VTL_AppResult VTL_ShedulePhoto(const VTL_Filename file_name, const VTL_content_platform_flags flags)
{
    VTL_AppResult app_result = VTL_res_kOk;
    
    VTL_publication_Image image = {0};
    memcpy(image.link, file_name, VTL_FILENAME_MAX_LENGTH);
    
    app_result = VTL_content_platform_PublicateImage(&image, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    app_result = VTL_user_history_SaveMediaWTextPublication(NULL, file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return app_result;
}

VTL_AppResult VTL_PubicatePhoto(const VTL_Filename file_name, const VTL_content_platform_flags flags)
{
    return VTL_ShedulePhoto(file_name, flags);
}

VTL_AppResult VTL_ShedulePhotoWithMarkedText(const VTL_Filename photo_file_name, 
                                                const VTL_Filename text_file_name,
                                                const VTL_content_platform_flags flags,
                                                const VTL_publication_marked_text_MarkupType markup_type)
{
    VTL_AppResult app_result = VTL_res_kOk;
    
    VTL_publication_ImageWithMarkedText image_with_text = {0};
    memcpy(image_with_text.image.link, photo_file_name, VTL_FILENAME_MAX_LENGTH);
    
    VTL_publication_marked_text_Configs marked_text_configs = {0};
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

    app_result = VTL_content_platform_PublicateImageWithMarkedText(&image_with_text, &marked_text_configs, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    app_result = VTL_user_history_SaveMediaWTextPublication(text_file_name, photo_file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return app_result;
}

VTL_AppResult VTL_PubicatePhotoWithMarkedText(const VTL_Filename photo_file_name, 
                                                const VTL_Filename text_file_name,
                                                const VTL_content_platform_flags flags,
                                                const VTL_publication_marked_text_MarkupType markup_type)
{
    return VTL_ShedulePhotoWithMarkedText(photo_file_name, text_file_name, flags, markup_type);
}

VTL_AppResult VTL_ShedulePhotoWithText(const VTL_Filename photo_file_name, 
                                                const VTL_Filename text_file_name,
                                                const VTL_content_platform_flags flags)
{
    return VTL_ShedulePhotoWithMarkedText(photo_file_name, text_file_name, flags, VTL_markup_type_kRegular);
}

VTL_AppResult VTL_PubicatePhotoWithText(const VTL_Filename photo_file_name, 
                                                const VTL_Filename text_file_name,
                                                const VTL_content_platform_flags flags)
{
    return VTL_PubicatePhotoWithMarkedText(photo_file_name, text_file_name, flags, VTL_markup_type_kRegular);
}

VTL_AppResult VTL_SheduleVideo(const VTL_Filename file_name, const VTL_content_platform_flags flags)
{
    VTL_AppResult app_result = VTL_res_kOk;
    
    VTL_publication_Video video = {0};
    memcpy(video.link, file_name, VTL_FILENAME_MAX_LENGTH);
    
    app_result = VTL_content_platform_PublicateVideo(&video, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    app_result = VTL_user_history_SaveMediaWTextPublication(NULL, file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return app_result;
}

VTL_AppResult VTL_PubicateVideo(const VTL_Filename file_name, const VTL_content_platform_flags flags)
{
    return VTL_SheduleVideo(file_name, flags);
}

VTL_AppResult VTL_SheduleVideoWithText(const VTL_Filename video_file_name, 
                                        const VTL_Filename text_file_name,
                                        const VTL_content_platform_flags flags)
{
    return VTL_SheduleVideoWithMarkedText(video_file_name, text_file_name, flags, VTL_markup_type_kRegular);
}

VTL_AppResult VTL_PubicateVideoWithText(const VTL_Filename video_file_name, 
                                            const VTL_Filename text_file_name,
                                            const VTL_content_platform_flags flags)
{
    return VTL_PubicateVideoWithMarkedText(video_file_name, text_file_name, flags, VTL_markup_type_kRegular);
}

VTL_AppResult VTL_SheduleVideoWithMarkedText(const VTL_Filename video_file_name, 
                                                const VTL_Filename text_file_name,
                                                const VTL_content_platform_flags flags,
                                                const VTL_publication_marked_text_MarkupType markup_type)
{
    VTL_AppResult app_result = VTL_res_kOk;
    
    VTL_publication_VideoWithMarkedText video_with_text = {0};
    memcpy(video_with_text.video.link, video_file_name, VTL_FILENAME_MAX_LENGTH);
    
    VTL_publication_marked_text_Configs marked_text_configs = {0};
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

    app_result = VTL_content_platform_PublicateVideoWithMarkedText(&video_with_text, &marked_text_configs, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    app_result = VTL_user_history_SaveMediaWTextPublication(text_file_name, video_file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return app_result;
}

VTL_AppResult VTL_PubicateVideoWithMarkedText(const VTL_Filename video_file_name, 
                                                const VTL_Filename text_file_name,
                                                const VTL_content_platform_flags flags,
                                                const VTL_publication_marked_text_MarkupType markup_type)
{
    return VTL_SheduleVideoWithMarkedText(video_file_name, text_file_name, flags, markup_type);
}

VTL_AppResult VTL_SheduleVideoWithInnerSub(const VTL_Filename video_file_name, 
                                                const VTL_Filename sub_file_name,
                                                const VTL_content_platform_flags flags)
{
    VTL_AppResult app_result = VTL_res_kOk;
    
    app_result = VTL_content_platform_PublicateVideoWithInnerSub(video_file_name, sub_file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }

    app_result = VTL_user_history_SaveMediaWTextPublication(sub_file_name, video_file_name, flags);
    if(app_result != VTL_res_kOk)
    {
        return app_result;
    }
    return app_result;
}

VTL_AppResult VTL_PubicateVideoWithInnerSub(const VTL_Filename video_file_name, 
                                                const VTL_Filename sub_file_name,
                                                const VTL_content_platform_flags flags)
{
    return VTL_SheduleVideoWithInnerSub(video_file_name, sub_file_name, flags);
}