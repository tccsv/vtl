#include <VTL/vkbot/VTL_vk_api.h>
#include <VTL/utils/curl/VTL_utils_curl_http_client.h>

#include <parson/parson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* RFC3986 unreserved: A-Z a-z 0-9 - _ . ~ — остальное процентим. */
char* VTL_vk_api_Escape(const char* in, char* out, size_t cap)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t o = 0;
    if (!out || cap == 0) return out;
    for (const unsigned char* p = (const unsigned char*)in; in && *p; ++p) {
        unsigned char c = *p;
        int safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                   c == '.' || c == '~';
        if (safe) {
            if (o + 1 >= cap) break;
            out[o++] = (char)c;
        } else {
            if (o + 3 >= cap) break;
            out[o++] = '%';
            out[o++] = hex[c >> 4];
            out[o++] = hex[c & 0x0F];
        }
    }
    out[o] = '\0';
    return out;
}

/* GET по url, отдаёт распарсенный JSON (или NULL). Тело чистится внутри. */
static JSON_Value* VTL_vk_api_GetJson(const char* url)
{
    HttpResponse resp;
    if (!VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp)) return NULL;

    JSON_Value* root = NULL;
    if (resp.status_code == 200 && resp.body)
        root = json_parse_string(resp.body);
    else
        fprintf(stderr, "[vk] HTTP %ld\n", resp.status_code);

    VTL_curl_http_client_ResponseCleanup(&resp);
    return root;
}

int VTL_vk_api_OpenLongPoll(const char* token, const char* group_id,
                            VTL_vk_LongPoll* lp)
{
    if (!token || !group_id || !lp) return 0;

    char url[512];
    snprintf(url, sizeof(url),
             "%s/groups.getLongPollServer?group_id=%s&access_token=%s&v=%s",
             VTL_VK_API_HOST, group_id, token, VTL_VK_API_VER);

    JSON_Value* root = VTL_vk_api_GetJson(url);
    if (!root) return 0;

    JSON_Object* obj = json_value_get_object(root);
    JSON_Object* r = obj ? json_object_get_object(obj, "response") : NULL;
    if (!r) {
        const char* em = obj ? json_object_dotget_string(obj, "error.error_msg") : NULL;
        fprintf(stderr, "[vk] getLongPollServer: %s\n", em ? em : "нет response");
        json_value_free(root);
        return 0;
    }

    const char* server = json_object_get_string(r, "server");
    const char* key    = json_object_get_string(r, "key");
    const char* ts     = json_object_get_string(r, "ts");
    if (!server || !key || !ts) { json_value_free(root); return 0; }

    snprintf(lp->server, sizeof(lp->server), "%s", server);
    snprintf(lp->key,    sizeof(lp->key),    "%s", key);
    snprintf(lp->ts,     sizeof(lp->ts),     "%s", ts);
    json_value_free(root);
    return 1;
}

char* VTL_vk_api_Check(const VTL_vk_LongPoll* lp)
{
    if (!lp) return NULL;
    char url[1024];
    snprintf(url, sizeof(url), "%s?act=a_check&key=%s&ts=%s&wait=%d",
             lp->server, lp->key, lp->ts, VTL_VK_WAIT_SEC);

    HttpResponse resp;
    if (!VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp)) return NULL;
    char* body = NULL;
    if (resp.status_code == 200 && resp.body) {
        body = resp.body;        /* отдаём владение наружу */
        resp.body = NULL;
    }
    VTL_curl_http_client_ResponseCleanup(&resp);
    return body;
}

int VTL_vk_api_Reply(const char* token, long long peer_id, const char* text)
{
    if (!token || !text) return 0;

    /* messages.send требует уникальный random_id — монотонный счётчик. */
    static long s_rnd = 1000;
    ++s_rnd;

    char enc[VTL_VK_MSG_ENC_MAX];
    VTL_vk_api_Escape(text, enc, sizeof(enc));

    char* url = (char*)malloc(strlen(VTL_VK_API_HOST) + strlen(token) +
                              strlen(enc) + 256);
    if (!url) return 0;
    sprintf(url, "%s/messages.send?peer_id=%lld&random_id=%ld&message=%s"
                 "&access_token=%s&v=%s",
            VTL_VK_API_HOST, peer_id, s_rnd, enc, token, VTL_VK_API_VER);

    JSON_Value* root = VTL_vk_api_GetJson(url);
    free(url);
    if (!root) return 0;

    JSON_Object* obj = json_value_get_object(root);
    int ok = obj && json_object_has_value(obj, "response");
    if (!ok) {
        const char* em = obj ? json_object_dotget_string(obj, "error.error_msg") : NULL;
        fprintf(stderr, "[vk] messages.send: %s\n", em ? em : "ошибка");
    }
    json_value_free(root);
    return ok;
}
