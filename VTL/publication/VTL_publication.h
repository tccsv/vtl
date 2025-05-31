#ifndef _VTL_PUBLICATION_H
#define _VTL_PUBLICATION_H

#ifdef __cplusplus
extern "C"
{
#endif



#include <VTL/publication/VTL_publication_data.h>
#include <VTL/publication/text/VTL_publication_text.h>
#include <VTL/content_platform/VTL_content_platform_publication.h>
#include <VTL/media_container/VTL_media_container.h>
#include <VTL/user/VTL_user.h>

#include <VTL/VTL_app_result.h>



// VTL_AppResult VTL_SheduleMarkedText(const VTL_Filename file_name, const VTL_content_platform_flags flags,
//                                         const VTL_publication_marked_text_MarkupType markup_type, 
//                                         const VTL_Time sheduled_time);
//TO DO Добавить время в другие функции
VTL_AppResult VTL_PubicateMarkedText(const VTL_Filename file_name, const VTL_content_platform_flags flags,
                                        const VTL_publication_marked_text_MarkupType markup_type);
// VTL_AppResult VTL_SheduleText(const VTL_Filename file_name, const VTL_content_platform_flags flags);
// VTL_AppResult VTL_PubicateText(const VTL_Filename file_name, const VTL_content_platform_flags flags);


// VTL_AppResult VTL_ShedulePhoto(const VTL_Filename file_name, const VTL_content_platform_flags flags);
// VTL_AppResult VTL_PubicatePhoto(const VTL_Filename file_name, const VTL_content_platform_flags flags);
// VTL_AppResult VTL_ShedulePhotoWithMarkedText(const VTL_Filename photo_file_name, 
//                                                 const VTL_Filename text_file_name,
//                                                 const VTL_content_platform_flags flags,
//                                                 const VTL_publication_marked_text_MarkupType markup_type);
                                                
// VTL_AppResult VTL_PubicatePhotoWithMarkedText(const VTL_Filename photo_file_name, 
//                                                 const VTL_Filename text_file_name,
//                                                 const VTL_content_platform_flags flags,
//                                                 const VTL_publication_marked_text_MarkupType markup_type);

// VTL_AppResult VTL_ShedulePhotoWithText(const VTL_Filename photo_file_name, 
//                                                 const VTL_Filename text_file_name,
//                                                 const VTL_content_platform_flags flags);
                                                
// VTL_AppResult VTL_PubicatePhotoWithText(const VTL_Filename photo_file_name, 
//                                                 const VTL_Filename text_file_name,
//                                                 const VTL_content_platform_flags flags);



// VTL_AppResult VTL_SheduleVideo(const VTL_Filename file_name, const VTL_content_platform_flags flags);

// VTL_AppResult VTL_PubicateVideo(const VTL_Filename file_name, const VTL_content_platform_flags flags);                    

// VTL_AppResult VTL_SheduleVideoWithText(const VTL_Filename video_file_name, 
//                                         const VTL_Filename text_file_name,
//                                         const VTL_content_platform_flags flags);

// VTL_AppResult VTL_PubicateVideoWithText(const VTL_Filename video_file_name, 
//                                             const VTL_Filename text_file_name,
//                                             const VTL_content_platform_flags flags);

// VTL_AppResult VTL_SheduleVideoWithMarkedText(const VTL_Filename video_file_name, 
//                                                 const VTL_Filename text_file_name,
//                                                 const VTL_content_platform_flags flags,
//                                                 const VTL_publication_marked_text_MarkupType markup_type);

// VTL_AppResult VTL_PubicateVideoWithMarkedText(const VTL_Filename video_file_name, 
//                                                 const VTL_Filename text_file_name,
//                                                 const VTL_content_platform_flags flags,
//                                                 const VTL_publication_marked_text_MarkupType markup_type);

// VTL_AppResult VTL_SheduleVideoWithInnerSub(const VTL_Filename video_file_name, 
//                                                 const VTL_Filename sub_file_name,
//                                                 const VTL_content_platform_flags flags);

// VTL_AppResult VTL_PubicateVideoWithInnerSub(const VTL_Filename video_file_name, 
//                                                 const VTL_Filename sub_file_name,
//                                                 const VTL_content_platform_flags flags);


VTL_AppResult VTL_PubicateAudioWithMarkedText(const VTL_Filename audio_file_name, 
                                        const VTL_Filename text_file_name, 
                                        const VTL_publication_marked_text_MarkupType markup_type,
                                        const VTL_content_platform_flags flags);

#ifdef __cplusplus
}
#endif


#endif