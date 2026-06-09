#include <VTL/bots/vkbot/api/VTL_vk_api.h>
#include <VTL/utils/curl/VTL_utils_curl_http_client.h>

#include <parson/parson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char* VTL_vk_api_BuildMethodUrl(const char* method, const char* query,
                                const char* token, char* out, size_t cap)
{
    if (!out || cap == 0 || !method || !token) return NULL;
    int n = snprintf(out, cap, "%s/%s?%s%saccess_token=%s&v=%s",
                     VTL_VK_API_HOST, method,
                     query ? query : "", (query && *query) ? "&" : "",
                     token, VTL_VK_API_VER);
    return (n < 0 || (size_t)n >= cap) ? NULL : out;
}

JSON_Value* VTL_vk_api_GetJson(const char* url)
{
    HttpResponse resp;
    if (!url || !VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp)) return NULL;

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

    char query[128];
    snprintf(query, sizeof(query), "group_id=%s", group_id);
    char url[512];
    if (!VTL_vk_api_BuildMethodUrl("groups.getLongPollServer", query, token,
                                   url, sizeof(url)))
        return 0;

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
        body = resp.body;
        resp.body = NULL;
    }
    VTL_curl_http_client_ResponseCleanup(&resp);
    return body;
}

int VTL_vk_api_Reply(const char* token, long long peer_id, const char* text)
{
    if (!token || !text) return 0;

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

int VTL_vk_api_SendMessage(const char* token, long long peer_id, const char* text)
{
    return VTL_vk_api_Reply(token, peer_id, text);
}

/* Первый объект группы из ответа groups.getById. */
static JSON_Object* VTL_vk_api_FirstGroup(JSON_Value* root)
{
    JSON_Object* obj = json_value_get_object(root);
    if (!obj) return NULL;
    JSON_Object* r = json_object_get_object(obj, "response");
    if (r) {
        JSON_Array* gs = json_object_get_array(r, "groups");
        if (gs && json_array_get_count(gs) > 0) return json_array_get_object(gs, 0);
    }
    JSON_Array* arr = json_object_get_array(obj, "response");
    if (arr && json_array_get_count(arr) > 0) return json_array_get_object(arr, 0);
    return NULL;
}

/* Запрос groups.getById с полями. */
static JSON_Value* VTL_vk_api_GetByIdRaw(const char* token, const char* group_id,
                                         const char* fields)
{
    if (!token || !group_id) return NULL;
    char query[160];
    snprintf(query, sizeof(query), "group_id=%s%s%s", group_id,
             fields ? "&fields=" : "", fields ? fields : "");
    char url[512];
    if (!VTL_vk_api_BuildMethodUrl("groups.getById", query, token, url, sizeof(url)))
        return NULL;
    return VTL_vk_api_GetJson(url);
}

int VTL_vk_api_GetCommunityName(const char* token, const char* group_id,
                                char* out_name, size_t cap)
{
    if (!out_name || cap == 0) return 0;
    out_name[0] = '\0';

    JSON_Value* root = VTL_vk_api_GetByIdRaw(token, group_id, NULL);
    if (!root) return 0;

    JSON_Object* g = VTL_vk_api_FirstGroup(root);
    const char* name = g ? json_object_get_string(g, "name") : NULL;
    int ok = 0;
    if (name) { snprintf(out_name, cap, "%s", name); ok = 1; }
    json_value_free(root);
    return ok;
}

long VTL_vk_api_GetMembersCount(const char* token, const char* group_id)
{
    JSON_Value* root = VTL_vk_api_GetByIdRaw(token, group_id, "members_count");
    if (!root) return -1;

    JSON_Object* g = VTL_vk_api_FirstGroup(root);
    long count = -1;
    if (g && json_object_has_value(g, "members_count"))
        count = (long)json_object_get_number(g, "members_count");
    json_value_free(root);
    return count;
}

int VTL_vk_api_CheckToken(const char* token, const char* group_id)
{
    JSON_Value* root = VTL_vk_api_GetByIdRaw(token, group_id, NULL);
    if (!root) return 0;
    int ok = (VTL_vk_api_FirstGroup(root) != NULL);
    json_value_free(root);
    return ok;
}

int VTL_vk_api_SetTyping(const char* token, long long peer_id)
{
    if (!token) return 0;
    char query[96];
    snprintf(query, sizeof(query), "type=typing&peer_id=%lld", peer_id);
    char url[512];
    if (!VTL_vk_api_BuildMethodUrl("messages.setActivity", query, token, url, sizeof(url)))
        return 0;

    JSON_Value* root = VTL_vk_api_GetJson(url);
    if (!root) return 0;
    JSON_Object* obj = json_value_get_object(root);
    int ok = obj && json_object_has_value(obj, "response");
    json_value_free(root);
    return ok;
}

int VTL_vk_api_MarkAsRead(const char* token, long long peer_id)
{
    if (!token) return 0;
    char query[96];
    snprintf(query, sizeof(query), "peer_id=%lld", peer_id);
    char url[512];
    if (!VTL_vk_api_BuildMethodUrl("messages.markAsRead", query, token, url, sizeof(url)))
        return 0;

    JSON_Value* root = VTL_vk_api_GetJson(url);
    if (!root) return 0;
    JSON_Object* obj = json_value_get_object(root);
    int ok = obj && json_object_has_value(obj, "response");
    json_value_free(root);
    return ok;
}
