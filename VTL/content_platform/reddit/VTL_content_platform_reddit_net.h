#ifndef _VTL_CONTENT_PLATFORM_reddit_NET_H
#define _VTL_CONTENT_PLATFORM_reddit_NET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/VTL_publication_data.h>
#include <VTL/user/VTL_user_data.h>
#include <VTL/utils/VTL_utils_time.h>
#include <VTL/VTL_app_result.h>

  typedef struct _VTL_net_api_data_reddit
  {
    int x;
  } VTL_net_api_data_reddit;

  VTL_AppResult VTL_reddit_get_access_token(const char *username, const char *password, const char *client_id, const char *client_secret);
  VTL_AppResult VTL_reddit_get_refresh_token(const char *username, const char *password, const char *client_id, const char *client_secret);
  VTL_AppResult VTL_wrapped_reddit_send_text(const char *subreddit, const char *file_path);
  VTL_AppResult VTL_wrapped_reddit_get_new_posts(const char *subreddit);
  VTL_AppResult VTL_wrapped_reddit_upload_photo(const char *subreddit, const char *file_path, const char *md_file_path);

  VTL_AppResult VTL_reddit_send_text(const char *subreddit, const char *title, const char *text, const char *access_token);

  VTL_AppResult VTL_reddit_get_new_posts(const char *subreddit, const char *access_token);
  VTL_AppResult VTL_reddit_get_latest_posts(const char *subreddit);

  VTL_AppResult VTL_reddit_upload_photo(const char *subreddit, const char *file_path, const char *access_token);

  VTL_AppResult VTL_reddit_upload_video(const char *subreddit, const char *file_path, const char *md_file_path);

#ifdef __cplusplus
}
#endif

#endif