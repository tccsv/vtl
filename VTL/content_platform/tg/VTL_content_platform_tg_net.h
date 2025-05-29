#ifndef _VTL_CONTENT_PLATFORM_TG_NET_H
#define _VTL_CONTENT_PLATFORM_TG_NET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/VTL_publication_data.h>
#include <VTL/user/VTL_user_data.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/VTL_app_result.h>


typedef struct {
    const char* token;       
    const char* chat_id;     
    const char* text;        
    const char* parse_mode;
} VTL_net_api_data_TG;

//Text
VTL_AppResult VTL_content_platform_tg_text_SendNow(const VTL_Filename file_name);
//Text with marked text
VTL_AppResult VTL_content_platform_tg_marked_text_SendNow(const VTL_Filename file_name);
//Audio
VTL_AppResult VTL_content_platform_tg_audio_SendNow(const VTL_Filename file_name);
//Audio with text
VTL_AppResult VTL_content_platform_tg_audio_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Audio with marked text
VTL_AppResult VTL_content_platform_tg_audio_w_marked_text_SendNow(const VTL_Filename audio_file_name, const VTL_Filename text_file_name);
//Document
VTL_AppResult VTL_content_platform_tg_document_SendNow(const VTL_Filename file_name);
//Document with text
VTL_AppResult VTL_content_platform_tg_document_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Document with marked text
VTL_AppResult VTL_content_platform_tg_document_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Animation
VTL_AppResult VTL_content_platform_tg_animation_SendNow(const VTL_Filename file_name);
//Animation with text
VTL_AppResult VTL_content_platform_tg_animation_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Animation with marked text
VTL_AppResult VTL_content_platform_tg_animation_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Photo
VTL_AppResult VTL_content_platform_tg_photo_SendNow(const VTL_Filename file_name);
//Photo with caption
VTL_AppResult VTL_content_platform_tg_photo_w_caption_SendNow(const VTL_Filename file_name, const VTL_Filename caption_file_name);
//Photo with marked text
VTL_AppResult VTL_content_platform_tg_photo_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Video
VTL_AppResult VTL_content_platform_tg_video_SendNow(const VTL_Filename file_name);
//Video with text
VTL_AppResult VTL_content_platform_tg_video_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Video with marked text
VTL_AppResult VTL_content_platform_tg_video_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name);
//Mediagroup only photo
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_SendNow(const VTL_Filename file_names[], size_t file_count);
//Mediagroup only photo with text
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mediagroup onlly photo with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_marked_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mediagroup only video
VTL_AppResult VTL_content_platform_tg_mediagroup_video_SendNow(const VTL_Filename file_names[], size_t file_count);
//Mediagroup only video with caption
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mediagroup only video with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_marked_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);



//Mediagroup only audio
VTL_AppResult VTL_content_platform_tg_mediagroup_audio_SendNow(const VTL_Filename file_names[], size_t file_count);
//Mediagroup audio with text
VTL_AppResult VTL_content_platform_tg_mediagroup_audio_w_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mediagroup audio with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_audio_w_marked_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mideagroup only document
VTL_AppResult VTL_content_platform_tg_mediagroup_document_SendNow(const VTL_Filename file_names[], size_t file_count);
//Mediagroup only document with text
VTL_AppResult VTL_content_platform_tg_mediagroup_document_w_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mediagroup only document with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_document_w_marked_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mediagroup only animation
VTL_AppResult VTL_content_platform_tg_mediagroup_animation_SendNow(const VTL_Filename file_names[], size_t file_count);
//Mediagroup only animation with text
VTL_AppResult VTL_content_platform_tg_mediagroup_animation_w_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Mediagroup only animation with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_animation_w_marked_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name);
//Meiagroup photo and video
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_and_video_SendNow(const VTL_Filename photo_files[], size_t photo_count, const VTL_Filename video_files[], size_t video_count);
//Mediagroup photo and video with caption
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_and_video_w_text_SendNow(const VTL_Filename photo_files[], size_t photo_count, const VTL_Filename video_files[], size_t video_count, 
                                                                                const VTL_Filename text_file_name);
//Mediagroup photo and video with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_and_video_w_marked_text_SendNow(const VTL_Filename photo_files[], size_t photo_count, const VTL_Filename video_files[], size_t video_count, 
                                                                                                    const VTL_Filename text_file_name);





VTL_AppResult VTL_content_platform_tg_text_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_marked_text_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_audio_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_audio_w_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_audio_w_marked_text_SendScheduled(const VTL_Filename audio_file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_document_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_document_w_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_document_w_marked_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_animation_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_animation_w_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name,const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_animation_w_marked_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_photo_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_photo_w_caption_SendScheduled(const VTL_Filename file_name, const VTL_Filename caption_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_photo_w_marked_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_video_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_video_w_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_video_w_marked_text_SendScheduled(const VTL_Filename file_name, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_text_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_marked_text_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_video_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_text_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_marked_text_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_audio_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_document_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_animation_SendScheduled(const VTL_Filename file_names[], size_t file_count, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_and_video_SendScheduled(const VTL_Filename photo_files[], size_t photo_count, const VTL_Filename video_files[], size_t video_count, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_and_video_w_text_SendScheduled(const VTL_Filename photo_files[], size_t photo_count, const VTL_Filename video_files[], size_t video_count, 
                                                                                          const VTL_Filename text_file_name, const VTL_Time sheduled_time);
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_and_video_w_marked_text_SendScheduled(const VTL_Filename photo_files[], size_t photo_count, const VTL_Filename video_files[], size_t video_count, 
                                                                                                          const VTL_Filename text_file_name, const VTL_Time sheduled_time);


// VTL_AppResult VTL_content_platform_tg_audio_w_marked_text_SendNow(const VTL_Filename audio_file_name, const VTL_Filename text_file_name);

// VTL_AppResult VTL_content_platform_tg_marked_text_SendScheduled(const VTL_Filename file_name, const VTL_Time sheduled_time);

// VTL_AppResult VTL_content_platform_tg_marked_text_SendNow(const VTL_Filename file_name);


#ifdef __cplusplus
}
#endif


#endif