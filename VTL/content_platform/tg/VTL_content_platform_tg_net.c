#include <VTL/content_platform/tg/VTL_content_platform_tg_net.h>

#ifndef TG_BOT_TOKEN
#define TG_BOT_TOKEN "7810720887:AAHwKaYUpJP9stmNgMp1Di24pfhanGKxyFQ"
#endif
#ifndef TG_CHAT_ID
#define TG_CHAT_ID "-1002621373458"
#endif

#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <VTL/utils/curl/VTL_http_client.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


// Initialize API data (token, chat_id)
static void VTL_content_platform_tg_ApiDataInit(VTL_net_api_data_TG* p) {
    const char* tok = getenv("TG_BOT_TOKEN");
    if (!tok || !*tok) {
        fprintf(stderr, "[error] TG_BOT_TOKEN environment variable is not set or empty.\n");
        exit(EXIT_FAILURE);
    }
    p->token   = (tok && *tok)  ? tok : TG_BOT_TOKEN;
    const char* cid = getenv("TG_CHAT_ID");
    if (!cid || !*cid) {
        fprintf(stderr, "[error] TG_CHAT_ID environment variable is not set or empty.\n");
        exit(EXIT_FAILURE);
    }
    p->chat_id = (cid && *cid)  ? cid : TG_CHAT_ID;
    p->text = NULL;
    p->parse_mode = NULL;
}

//VTL_content_platform_tg_PrepareBaseUrl
// Build base URL "https://api.telegram.org/bot<token>/"
static int VTL_content_platform_tg_PrepareBaseUrl(VTL_net_api_data_TG* api, char* buf, size_t sz) {
    int n = snprintf(buf, sz, "https://api.telegram.org/bot%s/", api->token);
    return (n < 0 || (size_t)n >= sz) ? -1 : 0;
}

// HTTP POST JSON
static int VTL_content_platform_tg_http_post_json(const char* url, cJSON* body) {
    char *json = cJSON_PrintUnformatted(body);
    if (!json) return 0;
    HttpRequest req = {0};
    req.content_type = "application/json";
    req.body = json;
    HttpResponse resp = {0};
    int ok = VTL_curl_http_client_Request(url, HTTP_POST, &req, &resp);
    VTL_curl_http_client_ResponseCleanup(&resp);
    free(json);
    return ok;
}

// Load file content into memory
static char* VTL_content_platform_tg_LoadFile(const char* path) {
    FILE* f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char* buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

// Send text message
static int VTL_content_platform_tg_SendMessage(VTL_net_api_data_TG* api) {
    char url[512];
    if (VTL_content_platform_tg_PrepareBaseUrl(api, url, sizeof(url))) return 0;
    strcat(url, "sendMessage");
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "chat_id", api->chat_id);
    cJSON_AddStringToObject(root, "text", api->text);
    if (api->parse_mode)
        cJSON_AddStringToObject(root, "parse_mode", api->parse_mode);
    int ok = VTL_content_platform_tg_http_post_json(url, root);
    cJSON_Delete(root);
    return ok;
}

// Public API
#define INIT() VTL_content_platform_tg_ApiDataInit(&api_data)

// Text
VTL_AppResult VTL_content_platform_tg_text_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMessage(&api_data);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Marked text (MarkdownV2)
VTL_AppResult VTL_content_platform_tg_marked_text_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMessage(&api_data);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Send media (audio, photo, video, etc.)
static int VTL_content_platform_tg_SendMedia(VTL_net_api_data_TG* api, const char* method, const char* field) 
{
    char url[512];
    if (VTL_content_platform_tg_PrepareBaseUrl(api, url, sizeof(url))) {
        return 0;
    }
    strcat(url, method);

    struct curl_httppost *form = NULL, *last = NULL;

    curl_formadd(&form, &last,
                 CURLFORM_COPYNAME,     "chat_id",
                 CURLFORM_COPYCONTENTS, api->chat_id,
                 CURLFORM_END);

    if (api->text) {
        curl_formadd(&form, &last,
                     CURLFORM_COPYNAME,     "caption",
                     CURLFORM_COPYCONTENTS, api->text,
                     CURLFORM_END);
        if (api->parse_mode) {
            curl_formadd(&form, &last,
                         CURLFORM_COPYNAME,     "parse_mode",
                         CURLFORM_COPYCONTENTS, api->parse_mode,
                         CURLFORM_END);
        }
    }

    curl_formadd(&form, &last,
                 CURLFORM_COPYNAME, field,
                 CURLFORM_FILE,     api->filename,
                 CURLFORM_END);

    struct curl_slist *extra_headers = NULL;
    HttpResponse resp = {0};
    int ok = VTL_curl_http_client_RequestMultipart(url, form, extra_headers, &resp);
    VTL_curl_http_client_ResponseCleanup(&resp);
    return ok;
}

// Public API: Audio
VTL_AppResult VTL_content_platform_tg_audio_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAudio", "audio");
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_audio_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAudio", "audio");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_audio_w_marked_text_SendNow(const VTL_Filename audio_file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = audio_file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAudio", "audio");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Document
VTL_AppResult VTL_content_platform_tg_document_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendDocument", "document");
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_document_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendDocument", "document");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_document_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendDocument", "document");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Animation
VTL_AppResult VTL_content_platform_tg_animation_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAnimation", "animation");
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_animation_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAnimation", "animation");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_animation_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAnimation", "animation");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Photo
VTL_AppResult VTL_content_platform_tg_photo_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendPhoto", "photo");
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_photo_w_caption_SendNow(const VTL_Filename file_name, const VTL_Filename caption_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(caption_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendPhoto", "photo");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_photo_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendPhoto", "photo");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Video
VTL_AppResult VTL_content_platform_tg_video_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendVideo", "video");
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_video_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendVideo", "video");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

VTL_AppResult VTL_content_platform_tg_video_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendVideo", "video");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Send media group (photos, videos, etc.)
static int VTL_content_platform_tg_SendMediaGroup(
    VTL_net_api_data_TG*  api,
    const char*           media_type,
    const VTL_Filename    files[],
    size_t                count,
    const char*           caption
) {
    char url[512];
    if (VTL_content_platform_tg_PrepareBaseUrl(api, url, sizeof(url))) {
        fprintf(stderr, "[ERR] URL preparation failed\n");
        return 0;
    }
    strcat(url, "sendMediaGroup");
    fprintf(stderr, "[DBG] URL = %s\n", url);

    cJSON *arr = cJSON_CreateArray();
    if (!arr) {
        fprintf(stderr, "[ERR] Failed to create JSON array\n");
        return 0;
    }
    
    for (size_t i = 0; i < count; ++i) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "type", media_type);
        
        char tag[16], uri[32];
        snprintf(tag, sizeof(tag), "file%zu", i);
        snprintf(uri, sizeof(uri), "attach://%s", tag);
        cJSON_AddStringToObject(obj, "media", uri);
        
        if (i == 0 && caption) {
            cJSON_AddStringToObject(obj, "caption", caption);
            if (api->parse_mode) {
                cJSON_AddStringToObject(obj, "parse_mode", api->parse_mode);
            }
        }
        cJSON_AddItemToArray(arr, obj);
    }
    
    char *js = cJSON_PrintUnformatted(arr);
    if (!js) {
        cJSON_Delete(arr);
        fprintf(stderr, "[ERR] Failed to print JSON\n");
        return 0;
    }
    cJSON_Delete(arr);

    struct curl_httppost *form = NULL, *last = NULL;
    
    curl_formadd(&form, &last,
                CURLFORM_COPYNAME,     "chat_id",
                CURLFORM_COPYCONTENTS, api->chat_id,
                CURLFORM_END);
    
    curl_formadd(&form, &last,
                CURLFORM_COPYNAME,     "media",
                CURLFORM_COPYCONTENTS, js,
                CURLFORM_END);
    
    for (size_t i = 0; i < count; ++i) {
        char tag_field[16];
        snprintf(tag_field, sizeof(tag_field), "file%zu", i);
        curl_formadd(&form, &last,
                    CURLFORM_COPYNAME, tag_field,
                    CURLFORM_FILE,     files[i],
                    CURLFORM_END);
    }
    
    struct curl_slist *extra_headers = NULL;
    HttpResponse resp = {0};
    int ok = VTL_curl_http_client_RequestMultipart(url, form, extra_headers, &resp);
    VTL_curl_http_client_ResponseCleanup(&resp);
    free(js);
    return ok;
}

// Media group only photo
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_SendNow(const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "photo", file_names, file_count, NULL);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Media group only photo with caption
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "photo", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Media group only photo with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_marked_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    api_data.parse_mode = "MarkdownV2";
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    int ok = VTL_content_platform_tg_SendMediaGroup( &api_data, "photo", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Media group only video
VTL_AppResult VTL_content_platform_tg_mediagroup_video_SendNow(
    const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    return VTL_content_platform_tg_SendMediaGroup(&api_data, "video", file_names, file_count, NULL) ? VTL_res_kOk : VTL_res_kError;
}

// Media group only video with caption
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_text_SendNow(
    const VTL_Filename file_names[], size_t file_count,
    const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "video", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Media group only video with Markdown2 caption
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_marked_text_SendNow(
    const VTL_Filename file_names[], size_t file_count,
    const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    api_data.parse_mode = "MarkdownV2";
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kError;
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "video", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kError;
}

// Media group only audio
VTL_AppResult VTL_content_platform_tg_mediagroup_audio_SendNow(
    const VTL_Filename file_names[], size_t file_count)
{
    fprintf(stderr, "[debug] in mediagroup_audio_SendNow, file_count=%zu\n", file_count);
    VTL_net_api_data_TG api_data; INIT();
    return VTL_content_platform_tg_SendMediaGroup(&api_data, "audio", file_names, file_count, NULL) ? VTL_res_kOk : VTL_res_kError;
}

// Media group only document
VTL_AppResult VTL_content_platform_tg_mediagroup_document_SendNow(
    const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    return VTL_content_platform_tg_SendMediaGroup(&api_data, "document", file_names, file_count, NULL) ? VTL_res_kOk : VTL_res_kError;
}


// Media group only animation (e.g., GIF)
VTL_AppResult VTL_content_platform_tg_mediagroup_animation_SendNow(
    const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    for (size_t i = 0; i < file_count; ++i) {
    VTL_content_platform_tg_animation_SendNow(file_names[i]);
    }
}