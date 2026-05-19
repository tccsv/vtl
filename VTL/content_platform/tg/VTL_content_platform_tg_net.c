#include <VTL/content_platform/tg/VTL_content_platform_tg_net.h>

#ifndef TG_BOT_TOKEN
#define TG_BOT_TOKEN "8517247017:AAFJWJWaE3Tb5VfDYm93M9819mTNVXgoM_E"
#endif
#ifndef TG_CHAT_ID
#define TG_CHAT_ID "639267858"
#endif

#include <curl/curl.h>
#include <parson/parson.h>
#include <VTL/utils/curl/VTL_http_client.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


// Initialize API data (token, chat_id). Env vars override the compile-time defaults.
static void VTL_content_platform_tg_ApiDataInit(VTL_net_api_data_TG* p) {
    const char* tok = getenv("TG_BOT_TOKEN");
    p->token = (tok && *tok) ? tok : TG_BOT_TOKEN;
    const char* cid = getenv("TG_CHAT_ID");
    p->chat_id = (cid && *cid) ? cid : TG_CHAT_ID;
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
static int VTL_content_platform_tg_http_post_json(const char* url, JSON_Value* body) {
    char *json = json_serialize_to_string(body);
    if (!json) return 0;
    HttpRequest req = {0};
    req.content_type = "application/json";
    req.body = json;
    HttpResponse resp = {0};
    int ok = VTL_curl_http_client_Request(url, HTTP_POST, &req, &resp);
    VTL_curl_http_client_ResponseCleanup(&resp);
    json_free_serialized_string(json);
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

// Send a single text message (no length checks; up to Telegram's 4096-char hard limit).
static int VTL_content_platform_tg_SendMessageRaw(VTL_net_api_data_TG* api, const char* text, size_t len) {
    char url[512];
    if (VTL_content_platform_tg_PrepareBaseUrl(api, url, sizeof(url))) return 0;
    strcat(url, "sendMessage");
    char* tmp = (char*)malloc(len + 1);
    if (!tmp) return 0;
    memcpy(tmp, text, len);
    tmp[len] = '\0';
    JSON_Value* root_val = json_value_init_object();
    JSON_Object* root = json_value_get_object(root_val);
    json_object_set_string(root, "chat_id", api->chat_id);
    json_object_set_string(root, "text", tmp);
    if (api->parse_mode)
        json_object_set_string(root, "parse_mode", api->parse_mode);
    int ok = VTL_content_platform_tg_http_post_json(url, root_val);
    json_value_free(root_val);
    free(tmp);
    return ok;
}

// Telegram sendMessage limit is 4096 UTF-16 code units; conservatively cap at 3500 bytes
// and split on the nearest newline / whitespace, never inside a UTF-8 multibyte sequence
// or an HTML tag. When chunking, also carry any open <b>/<i>/<s> tags across boundaries.
#define VTL_TG_TEXT_CHUNK_MAX 3500

// Update the open-tag depth counters for the bytes p[..p+n]. Naive scanner that
// looks for the supported <b>, <i>, <s> and their closing forms; everything else
// (escaped &lt;/&gt; or attributes) is ignored.
static void vtl_tg_track_tags(const char* p, size_t n, int* bold, int* italic, int* strike) {
    size_t i = 0;
    while (i < n) {
        if (p[i] != '<') { ++i; continue; }
        if (i + 2 < n && p[i + 1] == '/' && p[i + 3] == '>') {
            char c = p[i + 2];
            if      (c == 'b' && *bold   > 0) (*bold)--;
            else if (c == 'i' && *italic > 0) (*italic)--;
            else if (c == 's' && *strike > 0) (*strike)--;
            i += 4; continue;
        }
        if (i + 2 < n && p[i + 2] == '>') {
            char c = p[i + 1];
            if      (c == 'b') (*bold)++;
            else if (c == 'i') (*italic)++;
            else if (c == 's') (*strike)++;
            i += 3; continue;
        }
        ++i;
    }
}

static int VTL_content_platform_tg_SendMessage(VTL_net_api_data_TG* api) {
    if (!api->text) return 0;
    const char* p = api->text;
    size_t total = strlen(p);
    if (total <= VTL_TG_TEXT_CHUNK_MAX) {
        return VTL_content_platform_tg_SendMessageRaw(api, p, total);
    }
    size_t off = 0;
    int ok = 1;
    int bold = 0, italic = 0, strike = 0;
    while (off < total) {
        size_t remain = total - off;
        size_t take = remain > VTL_TG_TEXT_CHUNK_MAX ? VTL_TG_TEXT_CHUNK_MAX : remain;
        if (take < remain) {
            size_t lo = take - take / 4;
            size_t cut = take;
            for (size_t i = take; i > lo; --i) {
                if (p[off + i - 1] == '\n') { cut = i; break; }
            }
            if (cut == take) {
                for (size_t i = take; i > lo; --i) {
                    if (p[off + i - 1] == ' ') { cut = i; break; }
                }
            }
            // Never split inside a UTF-8 multibyte sequence.
            while (cut > 1 && ((unsigned char)p[off + cut] & 0xC0) == 0x80) --cut;
            // Never split inside an HTML tag: back up past any unmatched '<'.
            for (size_t i = cut; i > 0; --i) {
                char c = p[off + i - 1];
                if (c == '>') break;
                if (c == '<') { cut = i - 1; break; }
            }
            // Never split inside an <a>...</a>. Walk the slice and if the last
            // <a opened before `cut` has its </a> after `cut`, move `cut` to
            // just before that <a.
            {
                size_t last_open = (size_t)-1;
                int inside = 0;
                for (size_t i = 0; i < cut; ++i) {
                    if (p[off + i] != '<') continue;
                    if (i + 2 < cut && p[off + i + 1] == 'a' &&
                        (p[off + i + 2] == ' ' || p[off + i + 2] == '>')) {
                        last_open = i; inside = 1;
                    } else if (i + 3 < cut && p[off + i + 1] == '/' &&
                               p[off + i + 2] == 'a' && p[off + i + 3] == '>') {
                        inside = 0;
                    }
                }
                if (inside && last_open != (size_t)-1) cut = last_open;
            }
            take = cut;
        }
        // Build chunk: open tags inherited from previous + body + close-all at end.
        size_t prefix_len = (size_t)(bold * 3 + italic * 3 + strike * 3);
        // Update open-tag state with bytes in this chunk.
        int b2 = bold, i2 = italic, s2 = strike;
        vtl_tg_track_tags(p + off, take, &b2, &i2, &s2);
        size_t suffix_len = (size_t)(b2 * 4 + i2 * 4 + s2 * 4);
        char* buf = (char*)malloc(prefix_len + take + suffix_len + 1);
        if (!buf) { ok = 0; break; }
        size_t bp = 0;
        for (int k = 0; k < bold;   ++k) { memcpy(buf + bp, "<b>", 3); bp += 3; }
        for (int k = 0; k < italic; ++k) { memcpy(buf + bp, "<i>", 3); bp += 3; }
        for (int k = 0; k < strike; ++k) { memcpy(buf + bp, "<s>", 3); bp += 3; }
        memcpy(buf + bp, p + off, take); bp += take;
        // Close in reverse open order.
        for (int k = 0; k < s2; ++k) { memcpy(buf + bp, "</s>", 4); bp += 4; }
        for (int k = 0; k < i2; ++k) { memcpy(buf + bp, "</i>", 4); bp += 4; }
        for (int k = 0; k < b2; ++k) { memcpy(buf + bp, "</b>", 4); bp += 4; }
        if (!VTL_content_platform_tg_SendMessageRaw(api, buf, bp)) ok = 0;
        free(buf);
        bold = b2; italic = i2; strike = s2;
        off += take;
        while (off < total && (p[off] == '\n' || p[off] == ' ')) ++off;
    }
    return ok;
}

// Public API
#define INIT() VTL_content_platform_tg_ApiDataInit(&api_data)

// Text
VTL_AppResult VTL_content_platform_tg_text_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMessage(&api_data);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Marked text (HTML)
VTL_AppResult VTL_content_platform_tg_marked_text_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = "HTML";
    int ok = VTL_content_platform_tg_SendMessage(&api_data);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Send media (audio, photo, video, etc.)
static int VTL_content_platform_tg_SendMedia(VTL_net_api_data_TG* api, const char* method, const char* field)
{
    char url[512];
    if (VTL_content_platform_tg_PrepareBaseUrl(api, url, sizeof(url))) return 0;
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
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_audio_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAudio", "audio");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_audio_w_marked_text_SendNow(const VTL_Filename audio_file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = audio_file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAudio", "audio");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Document
VTL_AppResult VTL_content_platform_tg_document_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendDocument", "document");
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_document_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendDocument", "document");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_document_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendDocument", "document");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Animation
VTL_AppResult VTL_content_platform_tg_animation_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAnimation", "animation");
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_animation_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAnimation", "animation");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_animation_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendAnimation", "animation");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Photo
VTL_AppResult VTL_content_platform_tg_photo_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendPhoto", "photo");
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_photo_w_caption_SendNow(const VTL_Filename file_name, const VTL_Filename caption_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(caption_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendPhoto", "photo");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_photo_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendPhoto", "photo");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Video
VTL_AppResult VTL_content_platform_tg_video_SendNow(const VTL_Filename file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    api_data.text = NULL;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendVideo", "video");
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_video_w_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = NULL;
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendVideo", "video");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_content_platform_tg_video_w_marked_text_SendNow(const VTL_Filename file_name, const VTL_Filename text_file_name) {
    VTL_net_api_data_TG api_data; INIT();
    api_data.filename = file_name;
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    api_data.text = txt;
    api_data.parse_mode = "MarkdownV2";
    int ok = VTL_content_platform_tg_SendMedia(&api_data, "sendVideo", "video");
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
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
    if (VTL_content_platform_tg_PrepareBaseUrl(api, url, sizeof(url))) return 0;
    strcat(url, "sendMediaGroup");

    JSON_Value *arr_val = json_value_init_array();
    JSON_Array *arr = json_value_get_array(arr_val);
    if (!arr) return 0;

    for (size_t i = 0; i < count; ++i) {
        JSON_Value *obj_val = json_value_init_object();
        JSON_Object *obj = json_value_get_object(obj_val);
        json_object_set_string(obj, "type", media_type);

        char tag[16], uri[32];
        snprintf(tag, sizeof(tag), "file%zu", i);
        snprintf(uri, sizeof(uri), "attach://%s", tag);
        json_object_set_string(obj, "media", uri);

        if (i == 0 && caption) {
            json_object_set_string(obj, "caption", caption);
            if (api->parse_mode) {
                json_object_set_string(obj, "parse_mode", api->parse_mode);
            }
        }
        json_array_append_value(arr, obj_val);
    }

    char *js = json_serialize_to_string(arr_val);
    if (!js) {
        json_value_free(arr_val);
        return 0;
    }
    json_value_free(arr_val);

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
    json_free_serialized_string(js);
    return ok;
}

// Media group only photo
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_SendNow(const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "photo", file_names, file_count, NULL);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Media group only photo with caption
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "photo", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Media group only photo with marked text
VTL_AppResult VTL_content_platform_tg_mediagroup_photo_w_marked_text_SendNow(const VTL_Filename file_names[], size_t file_count, const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    api_data.parse_mode = "MarkdownV2";
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    int ok = VTL_content_platform_tg_SendMediaGroup( &api_data, "photo", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Media group only video
VTL_AppResult VTL_content_platform_tg_mediagroup_video_SendNow(
    const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    return VTL_content_platform_tg_SendMediaGroup(&api_data, "video", file_names, file_count, NULL) ? VTL_res_kOk : VTL_res_kErr;
}

// Media group only video with caption
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_text_SendNow(
    const VTL_Filename file_names[], size_t file_count,
    const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "video", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Media group only video with Markdown2 caption
VTL_AppResult VTL_content_platform_tg_mediagroup_video_w_marked_text_SendNow(
    const VTL_Filename file_names[], size_t file_count,
    const VTL_Filename text_file_name)
{
    VTL_net_api_data_TG api_data; INIT();
    api_data.parse_mode = "MarkdownV2";
    char* txt = VTL_content_platform_tg_LoadFile(text_file_name);
    if (!txt) return VTL_res_kErr;
    int ok = VTL_content_platform_tg_SendMediaGroup(&api_data, "video", file_names, file_count, txt);
    free(txt);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

// Media group only audio
VTL_AppResult VTL_content_platform_tg_mediagroup_audio_SendNow(
    const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    return VTL_content_platform_tg_SendMediaGroup(&api_data, "audio", file_names, file_count, NULL) ? VTL_res_kOk : VTL_res_kErr;
}

// Media group only document
VTL_AppResult VTL_content_platform_tg_mediagroup_document_SendNow(
    const VTL_Filename file_names[], size_t file_count)
{
    VTL_net_api_data_TG api_data; INIT();
    return VTL_content_platform_tg_SendMediaGroup(&api_data, "document", file_names, file_count, NULL) ? VTL_res_kOk : VTL_res_kErr;
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