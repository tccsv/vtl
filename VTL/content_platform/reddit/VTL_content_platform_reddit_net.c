#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <VTL/content_platform/reddit/VTL_content_platform_reddit_net.h>
#include <VTL/utils/curl/VTL_utils_curl_http_client_simple.h>

static char VTL_access_token[512] = "";

VTL_AppResult VTL_reddit_get_access_token(const char *username, const char *password, const char *client_id, const char *client_secret)
{
  char url[] = "https://www.reddit.com/api/v1/access_token";
  char auth_header[256];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Basic %s", client_id);
  char payload[512];
  snprintf(payload, sizeof(payload), "grant_type=password&username=%s&password=%s", username, password);
  const char *headers[] = {
      "Content-Type: application/x-www-form-urlencoded",
      auth_header};
  return http_post_with_headers(url, payload, headers, 2) == 0 ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_reddit_get_refresh_token(const char *username, const char *password, const char *client_id, const char *client_secret)
{
  char url[] = "https://www.reddit.com/api/v1/access_token";
  char auth_header[256];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Basic %s", client_id);
  char payload[512];
  snprintf(payload, sizeof(payload), "grant_type=password&username=%s&password=%s", username, password);
  const char *headers[] = {
      "Content-Type: application/x-www-form-urlencoded",
      auth_header};
  return http_post_with_headers(url, payload, headers, 2) == 0 ? VTL_res_kOk : VTL_res_kErr;
}

void VTL_ensure_access_token()
{
  if (strlen(VTL_access_token) == 0)
  {
    const char *username = "Necessary_Drink6378";
    const char *password = "Vlad0204";
    const char *client_id = "xjsF5XgLuKcMmQwx-TA0rA=";
    const char *client_secret = "bjeOpXl5HhEpCwern4QjUX7HOgCM6Q";
    if (VTL_reddit_get_access_token(username, password, client_id, client_secret) == VTL_res_kOk)
    {
      snprintf(VTL_access_token, sizeof(VTL_access_token), "mock_access_token");
    }
    else
    {
      fprintf(stderr, "Failed to obtain access token\n");
      exit(1);
    }
  }
}

static bool VTL_is_markdown_file(const char *file_path)
{
  const char *dot = strrchr(file_path, '.');
  return dot && strcmp(dot, ".md") == 0;
}

static int VTL_read_markdown_file(const char *file_path, char *title, size_t title_size, char *content, size_t content_size)
{
  FILE *file = fopen(file_path, "r");
  if (!file)
  {
    perror("Failed to open markdown file");
    return -1;
  }
  if (fgets(title, title_size, file))
  {
    title[strcspn(title, "\n")] = 0;
  }
  else
  {
    fclose(file);
    return -1;
  }
  size_t content_len = 0;
  char line[256];
  while (fgets(line, sizeof(line), file))
  {
    size_t line_len = strlen(line);
    if (content_len + line_len < content_size)
    {
      strcpy(content + content_len, line);
      content_len += line_len;
    }
  }
  fclose(file);
  return 0;
}

VTL_AppResult VTL_wrapped_reddit_send_text(const char *subreddit, const char *file_path)
{
  if (!VTL_is_markdown_file(file_path))
  {
    fprintf(stderr, "Error: File is not a markdown file\n");
    return VTL_res_kErr;
  }
  char title[256];
  char content[2048];
  if (VTL_read_markdown_file(file_path, title, sizeof(title), content, sizeof(content)) != 0)
  {
    fprintf(stderr, "Error: Failed to read markdown file\n");
    return VTL_res_kErr;
  }
  VTL_ensure_access_token();
  return VTL_reddit_send_text(subreddit, title, content, VTL_access_token);
}

VTL_AppResult VTL_wrapped_reddit_get_new_posts(const char *subreddit)
{
  VTL_ensure_access_token();
  return VTL_reddit_get_new_posts(subreddit, VTL_access_token);
}

VTL_AppResult VTL_wrapped_reddit_upload_photo(const char *subreddit, const char *file_path, const char *md_file_path)
{
  if (!VTL_is_markdown_file(md_file_path))
  {
    fprintf(stderr, "File is not a markdown file: %s\n", md_file_path);
    return VTL_res_kErr;
  }
  char title[256];
  char caption[2048];
  VTL_read_markdown_file(md_file_path, title, sizeof(title), caption, sizeof(caption));
  VTL_ensure_access_token();
  return VTL_reddit_upload_photo(subreddit, file_path, VTL_access_token);
}

VTL_AppResult VTL_wrapped_reddit_upload_video(const char *subreddit, const char *file_path, const char *md_file_path)
{
  if (!VTL_is_markdown_file(md_file_path))
  {
    fprintf(stderr, "File is not a markdown file: %s\n", md_file_path);
    return VTL_res_kErr;
  }
  char title[256];
  char caption[2048];
  VTL_read_markdown_file(md_file_path, title, sizeof(title), caption, sizeof(caption));
  VTL_ensure_access_token();
  return VTL_reddit_upload_video(subreddit, file_path, md_file_path);
}

VTL_AppResult VTL_reddit_send_text(const char *subreddit, const char *title, const char *text, const char *access_token)
{
  char url[256];
  snprintf(url, sizeof(url), "https://oauth.reddit.com/api/submit");
  char payload[4096];
  snprintf(payload, sizeof(payload), "sr=%s&title=%s&text=%s&kind=self", subreddit, title, text);
  char auth_header[256];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", access_token);
  const char *headers[] = {
      auth_header,
      "User-Agent: reddit-c-bot/0.1"};
  return http_post_with_headers(url, payload, headers, 2) == 0 ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_reddit_get_new_posts(const char *subreddit, const char *access_token)
{
  char url[256];
  snprintf(url, sizeof(url), "https://oauth.reddit.com/r/%s/new/", subreddit);
  char auth_header[256];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", access_token);
  const char *headers[] = {
      auth_header,
      "User-Agent: reddit-c-bot/0.1"};
  return http_get_with_headers(url, headers, 2) == 0 ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_reddit_upload_photo(const char *subreddit, const char *file_path, const char *access_token)
{
  char url[256];
  snprintf(url, sizeof(url), "https://oauth.reddit.com/api/submit");
  char auth_header[256];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", access_token);
  const char *headers[] = {
      auth_header,
      "User-Agent: reddit-c-bot/0.1"};
  return http_post_multipart(url, "file", file_path, subreddit, NULL) == 0 ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_reddit_upload_video(const char *subreddit, const char *file_path, const char *md_file_path)
{
  if (!VTL_is_markdown_file(md_file_path))
  {
    fprintf(stderr, "File is not a markdown file: %s\n", md_file_path);
    return VTL_res_kErr;
  }
  char title[256];
  char caption[2048];
  if (VTL_read_markdown_file(md_file_path, title, sizeof(title), caption, sizeof(caption)) != 0)
  {
    fprintf(stderr, "Failed to read markdown file: %s\n", md_file_path);
    return VTL_res_kErr;
  }
  VTL_ensure_access_token();
  char url[256];
  snprintf(url, sizeof(url), "https://oauth.reddit.com/api/submit");
  char auth_header[256];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", VTL_access_token);
  return http_post_multipart(url, "file", file_path, subreddit, NULL) == 0 ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_reddit_get_latest_posts(const char *subreddit)
{
  VTL_ensure_access_token();
  char url[256];
  snprintf(url, sizeof(url), "https://oauth.reddit.com/r/%s/new/", subreddit);
  char auth_header[256];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", VTL_access_token);
  const char *headers[] = {
      auth_header,
      "User-Agent: reddit-c-bot/0.1"};
  return http_get_with_headers(url, headers, 2) == 0 ? VTL_res_kOk : VTL_res_kErr;
}

#include <VTL/content_platform/reddit/VTL_content_platform_reddit_net.h>

static void VTL_content_platform_reddit_net_ApiDataInit(VTL_net_api_data_reddit *p_api_data)
{
  // p_api_data-> = ...
  return;
}

VTL_AppResult VTL_content_platform_reddit_text_SendNow(const VTL_Filename file_name)
{
  VTL_net_api_data_reddit api_data;
  VTL_content_platform_reddit_net_ApiDataInit(&api_data);

  return VTL_res_kOk;
}
