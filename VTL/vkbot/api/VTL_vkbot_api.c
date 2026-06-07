#include <VTL/vkbot/api/VTL_vkbot_api.h>
#include <VTL/vkbot/VTL_vkbot_data.h>
#include <VTL/utils/curl/VTL_utils_curl_http_client.h>

#include <parson/parson.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Percent-encode строки для тела x-www-form-urlencoded. Возвращает
 * malloc-нутую строку; вызывающий free(). NULL — ошибка/нет входа. */
static char* VTL_vkbot_api_UrlEncode(const char* s)
{
    if (!s) return NULL;
    static const char hex[] = "0123456789ABCDEF";
    size_t len = strlen(s);
    char* out = (char*)malloc(len * 3 + 1);
    if (!out) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out[j++] = (char)c;
        } else {
            out[j++] = '%';
            out[j++] = hex[c >> 4];
            out[j++] = hex[c & 0x0F];
        }
    }
    out[j] = '\0';
    return out;
}

/* Уникальный random_id для messages.send (защита VK от дублей сообщений). */
static long VTL_vkbot_api_RandomId(void)
{
    static long counter = 0;
    return (long)time(NULL) * 1000 + (++counter % 1000);
}

int VTL_vkbot_api_GetLongPollServer(const char* token, const char* group_id,
                                    char* out_server, size_t server_cap,
                                    char* out_key,    size_t key_cap,
                                    char* out_ts,     size_t ts_cap)
{
    if (!token || !group_id || !out_server || !out_key || !out_ts) return 0;
    out_server[0] = out_key[0] = out_ts[0] = '\0';

    char url[1024];
    snprintf(url, sizeof(url),
             "%s/groups.getLongPollServer?group_id=%s&access_token=%s&v=%s",
             VTL_VKBOT_API_BASE, group_id, token, VTL_VKBOT_API_VERSION);

    HttpResponse resp;
    int ok = VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp);
    if (!ok) return 0;

    int success = 0;
    if (resp.status_code == 200 && resp.body) {
        JSON_Value*  root = json_parse_string(resp.body);
        JSON_Object* obj  = root ? json_value_get_object(root) : NULL;
        JSON_Object* r    = obj  ? json_object_get_object(obj, "response") : NULL;
        const char* server = r ? json_object_get_string(r, "server") : NULL;
        const char* key    = r ? json_object_get_string(r, "key")    : NULL;
        const char* ts     = r ? json_object_get_string(r, "ts")     : NULL;
        if (server && key && ts) {
            snprintf(out_server, server_cap, "%s", server);
            snprintf(out_key,    key_cap,    "%s", key);
            snprintf(out_ts,     ts_cap,     "%s", ts);
            success = 1;
        } else if (obj && json_object_get_object(obj, "error")) {
            JSON_Object* err = json_object_get_object(obj, "error");
            fprintf(stderr, "[vkbot] getLongPollServer: VK error %d — %s\n",
                    (int)json_object_get_number(err, "error_code"),
                    json_object_get_string(err, "error_msg"));
        }
        if (root) json_value_free(root);
    } else {
        fprintf(stderr, "[vkbot] getLongPollServer: HTTP %ld\n", resp.status_code);
    }
    VTL_curl_http_client_ResponseCleanup(&resp);
    return success;
}

char* VTL_vkbot_api_GetUpdates(const char* server, const char* key,
                               const char* ts, int wait_s)
{
    if (!server || !key || !ts) return NULL;

    /* groups.getLongPollServer обычно возвращает server уже с https://,
     * но на всякий случай добавляем схему, если её нет. */
    char url[1280];
    if (strncmp(server, "http", 4) == 0) {
        snprintf(url, sizeof(url), "%s?act=a_check&key=%s&ts=%s&wait=%d",
                 server, key, ts, wait_s);
    } else {
        snprintf(url, sizeof(url), "https://%s?act=a_check&key=%s&ts=%s&wait=%d",
                 server, key, ts, wait_s);
    }

    HttpResponse resp;
    int ok = VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp);
    if (!ok) return NULL;
    if (resp.status_code != 200) {
        fprintf(stderr, "[vkbot] a_check: HTTP %ld\n", resp.status_code);
        VTL_curl_http_client_ResponseCleanup(&resp);
        return NULL;
    }
    /* Передаём владение телом наружу — cleanup не зовём. */
    return resp.body;
}

VTL_AppResult VTL_vkbot_api_SendMessage(const char* token, const char* peer_id,
                                        const char* text)
{
    if (!token || !peer_id || !text) return VTL_res_kInvalidParamErr;

    char* enc = VTL_vkbot_api_UrlEncode(text);
    if (!enc) return VTL_res_kMemAllocErr;

    /* Тело x-www-form-urlencoded: peer_id/random_id/токен безопасны как есть,
     * экранируем только пользовательский текст. */
    size_t need = strlen(enc) + strlen(token) + strlen(peer_id) + 128;
    char*  body = (char*)malloc(need);
    if (!body) { free(enc); return VTL_res_kMemAllocErr; }
    snprintf(body, need,
             "peer_id=%s&message=%s&random_id=%ld&access_token=%s&v=%s",
             peer_id, enc, VTL_vkbot_api_RandomId(), token, VTL_VKBOT_API_VERSION);
    free(enc);

    char url[256];
    snprintf(url, sizeof(url), "%s/messages.send", VTL_VKBOT_API_BASE);

    HttpRequest req;
    memset(&req, 0, sizeof(req));
    req.content_type = "application/x-www-form-urlencoded";
    req.body = body;

    HttpResponse resp;
    int ok = VTL_curl_http_client_Request(url, HTTP_POST, &req, &resp);
    free(body);
    if (!ok) return VTL_res_kErr;

    /* VK отвечает 200 даже на логические ошибки — проверяем поле "error". */
    VTL_AppResult result = VTL_res_kOk;
    if (resp.status_code != 200) {
        result = VTL_res_kErr;
    } else if (resp.body && strstr(resp.body, "\"error\"")) {
        fprintf(stderr, "[vkbot] messages.send: %s\n", resp.body);
        result = VTL_res_kErr;
    }
    VTL_curl_http_client_ResponseCleanup(&resp);
    return result;
}
