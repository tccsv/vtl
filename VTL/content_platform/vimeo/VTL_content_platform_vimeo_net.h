#ifndef _VTL_CONTENT_PLATFORM_VIMEO_NET_H
#define _VTL_CONTENT_PLATFORM_VIMEO_NET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <VTL/utils/VTL_file.h>
#include <VTL/utils/VTL_time.h>
#include <VTL/publication/VTL_publication_data.h>
#include <VTL/user/VTL_user_data.h>

/* ============================================================
 * Внутренняя структура API-данных (используется только внутри .c)
 * Токен берётся из env VIMEO_ACCESS_TOKEN — не передаётся снаружи.
 * ============================================================ */
typedef struct _VTL_net_api_data_vimeo {
    const char *access_token;
    const char *title;
    const char *description;
    const char *tags_csv;
    const char *video_uri;
    const char *value;
} VTL_net_api_data_vimeo;

/* ============================================================
 * Загрузка видео — SendNow
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_SendNow(
        const VTL_Filename video_file_name);

VTL_AppResult VTL_content_platform_vimeo_video_w_text_SendNow(
        const VTL_Filename video_file_name,
        const VTL_Filename text_file_name);

VTL_AppResult VTL_content_platform_vimeo_video_w_text_and_tags_SendNow(
        const VTL_Filename video_file_name,
        const VTL_Filename text_file_name,
        const VTL_Filename tags_file_name);

/* ============================================================
 * Загрузка видео — SendScheduled
 * Vimeo не имеет нативного scheduled API —
 * реализация выполняет sleep до scheduled_time, затем загружает.
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_SendScheduled(
        const VTL_Filename video_file_name,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_w_text_SendScheduled(
        const VTL_Filename video_file_name,
        const VTL_Filename text_file_name,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_w_text_and_tags_SendScheduled(
        const VTL_Filename video_file_name,
        const VTL_Filename text_file_name,
        const VTL_Filename tags_file_name,
        const VTL_Time scheduled_time);

/* ============================================================
 * Метаданные — SetNow
 * video_uri_file — текстовый файл с URI: "/videos/123456"
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetTitle(
        const VTL_Filename video_uri_file,
        const VTL_Filename title_file);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetDescription(
        const VTL_Filename video_uri_file,
        const VTL_Filename description_file);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetTags(
        const VTL_Filename video_uri_file,
        const VTL_Filename tags_csv_file);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetLanguage(
        const VTL_Filename video_uri_file,
        const VTL_Filename language_code_file);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetLicense(
        const VTL_Filename video_uri_file,
        const VTL_Filename license_code_file);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetContentRating(
        const VTL_Filename video_uri_file,
        const VTL_Filename rating_code_file);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetPassword(
        const VTL_Filename video_uri_file,
        const VTL_Filename password_file);

VTL_AppResult VTL_content_platform_vimeo_video_Delete(
        const VTL_Filename video_uri_file);

/* ============================================================
 * Метаданные — SetScheduled
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetTitleScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename title_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetDescriptionScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename description_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetTagsScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename tags_csv_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetLanguageScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename language_code_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetLicenseScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename license_code_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetContentRatingScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename rating_code_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetPasswordScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename password_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_DeleteScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Time scheduled_time);

/* ============================================================
 * Приватность — SetNow
 * ============================================================ */

/* "anybody"|"nobody"|"unlisted"|"password"|"team" */
VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetView(
        const VTL_Filename video_uri_file,
        const VTL_Filename view_code_file);

/* "public"|"private"|"whitelist" */
VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetEmbed(
        const VTL_Filename video_uri_file,
        const VTL_Filename embed_code_file);

/* "1" или "0" */
VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetDownload(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file);

/* "1" или "0" */
VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetAdd(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file);

/* "anybody"|"nobody"|"contacts" */
VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetComments(
        const VTL_Filename video_uri_file,
        const VTL_Filename comments_code_file);

/* ============================================================
 * Приватность — SetScheduled
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetViewScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename view_code_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetEmbedScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename embed_code_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetDownloadScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetAddScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetCommentsScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename comments_code_file,
        const VTL_Time scheduled_time);

/* ============================================================
 * Плеер — SetNow
 * ============================================================ */

/* hex без #: "00adef" */
VTL_AppResult VTL_content_platform_vimeo_video_player_SetColor(
        const VTL_Filename video_uri_file,
        const VTL_Filename hex_color_file);

/* "1" или "0" */
VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowTitle(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file);

/* "1" или "0" */
VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowByline(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file);

/* "1" или "0" */
VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowPortrait(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file);

/* "1" или "0" */
VTL_AppResult VTL_content_platform_vimeo_video_player_SetAutoplay(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file);

/* "1" или "0" */
VTL_AppResult VTL_content_platform_vimeo_video_player_SetLoop(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file);

/* ============================================================
 * Плеер — SetScheduled
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_player_SetColorScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename hex_color_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowTitleScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowBylineScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowPortraitScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_player_SetAutoplayScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_player_SetLoopScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename bool_file,
        const VTL_Time scheduled_time);

/* ============================================================
 * Получение информации
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_GetUploadStatus(
        const VTL_Filename video_uri_file,
        const VTL_Filename out_file);

VTL_AppResult VTL_content_platform_vimeo_video_GetEmbedCode(
        const VTL_Filename video_uri_file,
        const VTL_Filename out_file);

VTL_AppResult VTL_content_platform_vimeo_video_GetMyVideos(
        int page,
        int per_page,
        const VTL_Filename out_file);

/* ============================================================
 * Домены встраивания
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_domain_Add(
        const VTL_Filename video_uri_file,
        const VTL_Filename domain_file);

VTL_AppResult VTL_content_platform_vimeo_video_domain_Remove(
        const VTL_Filename video_uri_file,
        const VTL_Filename domain_file);

VTL_AppResult VTL_content_platform_vimeo_video_domain_AddScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename domain_file,
        const VTL_Time scheduled_time);

VTL_AppResult VTL_content_platform_vimeo_video_domain_RemoveScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename domain_file,
        const VTL_Time scheduled_time);

/* ============================================================
 * Обложка (thumbnail)
 * ============================================================ */

VTL_AppResult VTL_content_platform_vimeo_video_thumbnail_Upload(
        const VTL_Filename video_uri_file,
        const VTL_Filename image_file_name);

VTL_AppResult VTL_content_platform_vimeo_video_thumbnail_UploadScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename image_file_name,
        const VTL_Time scheduled_time);

/* ============================================================
 * Главы (chapters)
 * ============================================================ */

/* Формат файла: первая строка = время в секундах, вторая = заголовок */
VTL_AppResult VTL_content_platform_vimeo_video_chapter_Add(
        const VTL_Filename video_uri_file,
        const VTL_Filename chapter_data_file);

VTL_AppResult VTL_content_platform_vimeo_video_chapter_GetAll(
        const VTL_Filename video_uri_file,
        const VTL_Filename out_file);

VTL_AppResult VTL_content_platform_vimeo_video_chapter_AddScheduled(
        const VTL_Filename video_uri_file,
        const VTL_Filename chapter_data_file,
        const VTL_Time scheduled_time);

#ifdef __cplusplus
}
#endif

#endif /* _VTL_CONTENT_PLATFORM_VIMEO_NET_H */
