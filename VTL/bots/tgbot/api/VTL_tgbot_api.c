#include <VTL/bots/tgbot/api/VTL_tgbot_api.h>
#include <VTL/bots/tgbot/VTL_tgbot_data.h>
#include <VTL/utils/curl/VTL_utils_curl_http_client.h>

#include <parson/parson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Собирает URL метода Bot API (free()). */
static char* VTL_tgbot_api_BuildUrl(const char* token, const char* method,
                                  const char* query)
{
    if (!token || !method) return NULL;
    size_t need = strlen(VTL_tgbot_api_BASE) + strlen(token) + strlen(method)
                + (query ? strlen(query) : 0) + 16;
    char* url = (char*)malloc(need);
    if (!url) return NULL;
    snprintf(url, need, "%s/bot%s/%s%s",
             VTL_tgbot_api_BASE, token, method, query ? query : "");
    return url;
}

int VTL_tgbot_api_GetMe(const char* token, char* out_username, size_t cap)
{
    if (!token || !out_username || cap == 0) return 0;
    out_username[0] = '\0';

    char* url = VTL_tgbot_api_BuildUrl(token, "getMe", NULL);
    if (!url) return 0;

    HttpResponse resp;
    int ok = VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp);
    free(url);
    if (!ok) return 0;

    int success = 0;
    if (resp.status_code == 200 && resp.body) {
        JSON_Value* root = json_parse_string(resp.body);
        JSON_Object* obj = root ? json_value_get_object(root) : NULL;
        const char* uname = obj ? json_object_dotget_string(obj, "result.username")
                                : NULL;
        if (uname) {
            snprintf(out_username, cap, "%s", uname);
            success = 1;
        }
        if (root) json_value_free(root);
    }
    VTL_curl_http_client_ResponseCleanup(&resp);
    return success;
}

int VTL_tgbot_api_DeleteWebhook(const char* token)
{
    if (!token) return 0;

    char* url = VTL_tgbot_api_BuildUrl(token, "deleteWebhook", NULL);
    if (!url) return 0;

    HttpResponse resp;
    int ok = VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp);
    free(url);
    if (!ok) return 0;

    int success = (resp.status_code == 200);
    if (!success) {
        fprintf(stderr, "[bot] deleteWebhook: HTTP %ld%s%s\n", resp.status_code,
                resp.body ? " — " : "", resp.body ? resp.body : "");
    }
    VTL_curl_http_client_ResponseCleanup(&resp);
    return success;
}

char* VTL_tgbot_api_GetUpdates(const char* token, long offset, int timeout_s)
{
    if (!token) return NULL;

    char query[96];
    snprintf(query, sizeof(query), "?offset=%ld&timeout=%d", offset, timeout_s);

    char* url = VTL_tgbot_api_BuildUrl(token, "getUpdates", query);
    if (!url) return NULL;

    HttpResponse resp;
    int ok = VTL_curl_http_client_Request(url, HTTP_GET, NULL, &resp);
    free(url);

    if (!ok) return NULL;
    if (resp.status_code != 200) {
        fprintf(stderr, "[bot] getUpdates: HTTP %ld%s%s\n", resp.status_code,
                resp.body ? " — " : "", resp.body ? resp.body : "");
        VTL_curl_http_client_ResponseCleanup(&resp);
        return NULL;
    }
    return resp.body;
}

VTL_AppResult VTL_tgbot_api_SendMessage(const char* token,
                                      const char* chat_id,
                                      const char* text)
{
    if (!token || !chat_id || !text) return VTL_res_kInvalidParamErr;

    JSON_Value*  body_val = json_value_init_object();
    if (!body_val) return VTL_res_kMemAllocErr;
    JSON_Object* body_obj = json_value_get_object(body_val);
    json_object_set_string(body_obj, "chat_id", chat_id);
    json_object_set_string(body_obj, "text", text);

    char* body = json_serialize_to_string(body_val);
    json_value_free(body_val);
    if (!body) return VTL_res_kMemAllocErr;

    char* url = VTL_tgbot_api_BuildUrl(token, "sendMessage", NULL);
    if (!url) { json_free_serialized_string(body); return VTL_res_kMemAllocErr; }

    HttpRequest req;
    memset(&req, 0, sizeof(req));
    req.content_type = "application/json";
    req.body = body;

    HttpResponse resp;
    int ok = VTL_curl_http_client_Request(url, HTTP_POST, &req, &resp);

    free(url);
    json_free_serialized_string(body);

    if (!ok) return VTL_res_kErr;

    VTL_AppResult result = (resp.status_code == 200) ? VTL_res_kOk : VTL_res_kErr;
    VTL_curl_http_client_ResponseCleanup(&resp);
    return result;
}
