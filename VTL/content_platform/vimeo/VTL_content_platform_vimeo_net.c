#define _POSIX_C_SOURCE 200809L

#include <VTL/content_platform/vimeo/VTL_content_platform_vimeo_net.h>

#include <curl/curl.h>
#include <parson/parson.h>
#include <VTL/utils/curl/VTL_http_client.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define VTL_VIMEO_URL_BUF              512
#define VTL_VIMEO_UPLOAD_LINK_BUF      2048
#define VTL_VIMEO_PIC_URI_BUF          256
#define VTL_VIMEO_AUTH_HDR_BUF         512
#define VTL_VIMEO_CHAPTER_TITLE_BUF    256
#define VTL_VIMEO_CONTENT_LENGTH_BUF   64
#define VTL_VIMEO_API_BASE             "https://api.vimeo.com"
#define VTL_VIMEO_ACCEPT_HDR           "Accept: application/vnd.vimeo.*+json;version=3.4"
#define VTL_VIMEO_TUS_TIMEOUT_SEC      300L
#define VTL_VIMEO_THUMB_TIMEOUT_SEC    60L

/* MSVC CRT не имеет POSIX fmemopen / strtok_r:
 *   strtok_r → strtok_s (Annex K, бинарно совместима по сигнатуре);
 *   fmemopen → tmpfile() с предзаписью буфера (libcurl читает FILE* через fread,
 *              поведение для CURLOPT_READDATA идентично). */
#ifdef _MSC_VER
#define strtok_r strtok_s
static FILE *VTL_content_platform_vimeo_FmemopenCompat(
    const void *buf, size_t size, const char *mode)
{
    (void) mode;
    FILE *f = tmpfile();
    if (!f) return NULL;
    if (fwrite(buf, 1, size, f) != size) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    return f;
}
#define fmemopen(b, s, m) VTL_content_platform_vimeo_FmemopenCompat((b), (s), (m))
#endif

/* ============================================================
 * Внутренние вспомогательные функции
 * (паттерн идентичен TG: static, с префиксом модуля)
 * ============================================================ */

/* Внутренняя структура API-данных. Токен берётся из env VIMEO_ACCESS_TOKEN. */
typedef struct {
    const char *access_token;
    const char *title;
    const char *description;
    const char *tags_csv;
} VTL_net_api_data_vimeo;

/* Инициализация API-данных из env. */
static VTL_AppResult VTL_content_platform_vimeo_ApiDataInit(
        VTL_net_api_data_vimeo *p) {
    const char *tok = getenv("VIMEO_ACCESS_TOKEN");
    if (!tok || !*tok) return VTL_res_vimeo_kMissingToken;
    p->access_token = tok;
    p->title = NULL;
    p->description = NULL;
    p->tags_csv = NULL;
    return VTL_res_kOk;
}

/* Читает файл целиком в malloc-буфер. Caller обязан free(). */
static char *VTL_content_platform_vimeo_LoadFile(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char *buf = malloc((size_t) sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t) sz, f);
    fclose(f);
    if (n != (size_t) sz) { free(buf); return NULL; }
    buf[sz] = '\0';
    /* Обрезаем trailing whitespace/newlines */
    for (long i = sz - 1; i >= 0 && (buf[i] == '\n' || buf[i] == '\r'
                                     || buf[i] == ' '); i--)
        buf[i] = '\0';
    return buf;
}

/* Получить размер файла */
static long VTL_content_platform_vimeo_GetFileSize(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1L;
    return (long) st.st_size;
}

/* Записать строку в файл */
static VTL_AppResult VTL_content_platform_vimeo_WriteFile(const char *path,
                                                          const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return VTL_res_kFileWriteErr;
    fputs(content, f);
    fclose(f);
    return VTL_res_kOk;
}

/* Сборка URL c проверкой truncation: возвращает kOk если уложились,
 * kErr если snprintf обрезал результат. */
static VTL_AppResult VTL_content_platform_vimeo_BuildUrl(
        char *out, size_t out_size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, out_size, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t) n >= out_size) return VTL_res_kErr;
    return VTL_res_kOk;
}

/* No-op write callback — libcurl без него пишет в stdout. */
static size_t VTL_content_platform_vimeo_DiscardWriteCb(
        void *ptr, size_t sz, size_t nmemb, void *up) {
    (void) ptr;
    (void) up;
    return sz * nmemb;
}

/* Явные дефолты SSL для каждого curl_easy_init — поведение не должно
 * зависеть от опций сборки libcurl. */
static void VTL_content_platform_vimeo_ApplyCurlSecurity(CURL *curl) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
}

/* ============================================================
 * HTTP-примитивы через VTL_curl_http_client (как в TG модуле)
 * ============================================================ */

/* Буфер для тела ответа PATCH/POST */
typedef struct {
    char *data;
    size_t len;
} VTL_vimeo_RespBuf;

static size_t VTL_content_platform_vimeo_RespWriteCb(
        void *ptr, size_t sz, size_t nmemb, void *up) {
    size_t total = sz * nmemb;
    VTL_vimeo_RespBuf *b = (VTL_vimeo_RespBuf *) up;
    char *tmp = realloc(b->data, b->len + total + 1);
    if (!tmp) return 0;
    b->data = tmp;
    memcpy(b->data + b->len, ptr, total);
    b->len += total;
    b->data[b->len] = '\0';
    return total;
}

/* Универсальный POST/PATCH JSON — caller free() результата или NULL.
 * method — "POST" или "PATCH" (PATCH через CUSTOMREQUEST,
 * VTL_curl_http_client его не поддерживает). */
static char *VTL_content_platform_vimeo_HttpJson(const char *method,
                                                 const char *url,
                                                 const char *body,
                                                 const char *token) {
    char auth_hdr[VTL_VIMEO_AUTH_HDR_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            auth_hdr, sizeof(auth_hdr),
            "Authorization: bearer %s", token) != VTL_res_kOk)
        return NULL;

    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    hdrs = curl_slist_append(hdrs, VTL_VIMEO_ACCEPT_HDR);
    hdrs = curl_slist_append(hdrs, auth_hdr);

    VTL_vimeo_RespBuf b = {malloc(1), 0};
    if (b.data) b.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    }
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     VTL_content_platform_vimeo_RespWriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    VTL_content_platform_vimeo_ApplyCurlSecurity(curl);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(b.data);
        return NULL;
    }
    return b.data;
}

/* GET — возвращает тело ответа или NULL */
static char *VTL_content_platform_vimeo_HttpGet(const char *url,
                                                const char *token) {
    char auth_hdr[VTL_VIMEO_AUTH_HDR_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            auth_hdr, sizeof(auth_hdr),
            "Authorization: bearer %s", token) != VTL_res_kOk)
        return NULL;

    HttpRequest req = {0};
    req.content_type = NULL;
    req.body = NULL;
    VTL_curl_http_client_AddHeader(&req, auth_hdr);
    VTL_curl_http_client_AddHeader(&req, VTL_VIMEO_ACCEPT_HDR);

    HttpResponse resp = {0};
    int ok = VTL_curl_http_client_Request(url, HTTP_GET, &req, &resp);
    curl_slist_free_all(req.headers);

    if (!ok || !resp.body) {
        VTL_curl_http_client_ResponseCleanup(&resp);
        return NULL;
    }
    char *body = strdup(resp.body);
    VTL_curl_http_client_ResponseCleanup(&resp);
    return body;
}

/* DELETE */
static VTL_AppResult VTL_content_platform_vimeo_HttpDelete(const char *url,
                                                           const char *token) {
    char auth_hdr[VTL_VIMEO_AUTH_HDR_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            auth_hdr, sizeof(auth_hdr),
            "Authorization: bearer %s", token) != VTL_res_kOk)
        return VTL_res_kErr;

    HttpRequest req = {0};
    VTL_curl_http_client_AddHeader(&req, auth_hdr);

    HttpResponse resp = {0};
    int ok = VTL_curl_http_client_Request(url, HTTP_DELETE, &req, &resp);
    curl_slist_free_all(req.headers);
    VTL_curl_http_client_ResponseCleanup(&resp);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

/* PUT (для доменов) */
static VTL_AppResult VTL_content_platform_vimeo_HttpPut(const char *url,
                                                        const char *token) {
    char auth_hdr[VTL_VIMEO_AUTH_HDR_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            auth_hdr, sizeof(auth_hdr),
            "Authorization: bearer %s", token) != VTL_res_kOk)
        return VTL_res_kErr;

    HttpRequest req = {0};
    req.body = "";
    VTL_curl_http_client_AddHeader(&req, auth_hdr);
    VTL_curl_http_client_AddHeader(&req, "Content-Length: 0");

    HttpResponse resp = {0};
    int ok = VTL_curl_http_client_Request(url, HTTP_PUT, &req, &resp);
    curl_slist_free_all(req.headers);
    VTL_curl_http_client_ResponseCleanup(&resp);
    return ok ? VTL_res_kOk : VTL_res_kErr;
}

/* ============================================================
 * TUS-загрузка файла
 * Токен — в URL (?token=...), Authorization НЕ нужен.
 * ============================================================ */
static VTL_AppResult VTL_content_platform_vimeo_TusUpload(
        const char *upload_link, const char *file_path, long file_size) {
    FILE *fp = fopen(file_path, "rb");
    if (!fp) return VTL_res_kFileOpenErr;

    char *buf = malloc((size_t) file_size);
    if (!buf) {
        fclose(fp);
        return VTL_res_kMemAllocErr;
    }
    size_t n = fread(buf, 1, (size_t) file_size, fp);
    fclose(fp);
    if ((long) n != file_size) {
        free(buf);
        return VTL_res_kFileReadErr;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        free(buf);
        return VTL_res_vimeo_kTusUploadError;
    }

    char cl_hdr[VTL_VIMEO_CONTENT_LENGTH_BUF];
    snprintf(cl_hdr, sizeof(cl_hdr), "Content-Length: %ld", file_size);

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Tus-Resumable: 1.0.0");
    hdrs = curl_slist_append(hdrs, "Upload-Offset: 0");
    hdrs = curl_slist_append(hdrs, "Content-Type: application/offset+octet-stream");
    hdrs = curl_slist_append(hdrs, cl_hdr);

    FILE *mem = fmemopen(buf, (size_t) file_size, "rb");
    if (!mem) {
        free(buf);
        curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
        return VTL_res_vimeo_kTusUploadError;
    }

    curl_easy_setopt(curl, CURLOPT_URL, upload_link);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, mem);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) file_size);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     VTL_content_platform_vimeo_DiscardWriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, VTL_VIMEO_TUS_TIMEOUT_SEC);
    VTL_content_platform_vimeo_ApplyCurlSecurity(curl);

    CURLcode res = curl_easy_perform(curl);
    fclose(mem);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    free(buf);

    if (res != CURLE_OK || http_code != 204) {
        fprintf(stderr, "[VTL/vimeo] TUS error: %s (HTTP %ld)\n",
                curl_easy_strerror(res), http_code);
        return VTL_res_vimeo_kTusUploadError;
    }
    return VTL_res_kOk;
}

/* ============================================================
 * Внутренняя функция загрузки — ядро
 * ============================================================ */
static VTL_AppResult VTL_content_platform_vimeo_DoUpload(
        VTL_net_api_data_vimeo *api, const char *file_path) {
    long file_size = VTL_content_platform_vimeo_GetFileSize(file_path);
    if (file_size < 0) {
        fprintf(stderr, "[VTL/vimeo] File not found: %s\n", file_path);
        return VTL_res_kFileOpenErr;
    }

    /* Строим JSON для POST /me/videos */
    JSON_Value *root_val = json_value_init_object();
    JSON_Object *root_obj = json_value_get_object(root_val);
    JSON_Value *up_val = json_value_init_object();
    JSON_Object *up_obj = json_value_get_object(up_val);
    json_object_set_string(up_obj, "approach", "tus");
    json_object_set_number(up_obj, "size", (double) file_size);
    json_object_set_value(root_obj, "upload", up_val);

    if (api->title)
        json_object_set_string(root_obj, "name", api->title);
    if (api->description)
        json_object_set_string(root_obj, "description", api->description);
    if (api->tags_csv) {
        JSON_Value *ta = json_value_init_array();
        JSON_Array *arr = json_value_get_array(ta);
        char *copy = strdup(api->tags_csv), *sp = NULL;
        if (copy) {
            for (char *t = strtok_r(copy, ",", &sp); t;
                 t = strtok_r(NULL, ",", &sp))
                json_array_append_string(arr, t);
            free(copy);
        }
        json_object_set_value(root_obj, "tags", ta);
    }

    char *body = json_serialize_to_string(root_val);
    json_value_free(root_val);
    if (!body) return VTL_res_kMemAllocErr;

    char *resp = VTL_content_platform_vimeo_HttpJson(
            "POST", VTL_VIMEO_API_BASE "/me/videos", body, api->access_token);
    json_free_serialized_string(body);
    if (!resp) return VTL_res_vimeo_kHttpPostError;

    JSON_Value *rv = json_parse_string(resp);
    free(resp);
    if (!rv) return VTL_res_vimeo_kJsonParseError;

    JSON_Object *ro = json_value_get_object(rv);
    JSON_Object *upro = json_object_get_object(ro, "upload");
    const char *link = upro ? json_object_get_string(upro, "upload_link") : NULL;

    if (!link) {
        fprintf(stderr, "[VTL/vimeo] Missing upload_link in response\n");
        json_value_free(rv);
        return VTL_res_vimeo_kMissingUploadLink;
    }

    /* Копия upload_link нужна потому, что json_value_free инвалидирует строку
     * parson'а. Буфер 2048 — ссылка ~1400 байт. */
    char link_copy[VTL_VIMEO_UPLOAD_LINK_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(link_copy, sizeof(link_copy),
                                            "%s", link) != VTL_res_kOk) {
        json_value_free(rv);
        return VTL_res_vimeo_kMissingUploadLink;
    }
    json_value_free(rv);

    printf("[VTL/vimeo] Uploading %s (%ld bytes)...\n", file_path, file_size);
    VTL_AppResult r = VTL_content_platform_vimeo_TusUpload(
            link_copy, file_path, file_size);
    if (r == VTL_res_kOk)
        printf("[VTL/vimeo] Upload complete.\n");
    return r;
}

/* ============================================================
 * PATCH-helper'ы — собирают JSON, шлют PATCH, освобождают
 * ============================================================ */

/* PATCH одного поля верхнего уровня */
static VTL_AppResult VTL_content_platform_vimeo_PatchField(
        const char *video_uri, const char *token,
        const char *field, const char *value) {
    JSON_Value *v = json_value_init_object();
    json_object_set_string(json_value_get_object(v), field, value);
    char *body = json_serialize_to_string(v);
    json_value_free(v);
    if (!body) return VTL_res_kMemAllocErr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                            VTL_VIMEO_API_BASE "%s",
                                            video_uri) != VTL_res_kOk) {
        json_free_serialized_string(body);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpJson("PATCH", url, body, token);
    json_free_serialized_string(body);
    if (!resp) return VTL_res_vimeo_kHttpPatchError;
    free(resp);
    return VTL_res_kOk;
}

/* PATCH вложенного privacy.<field> = string */
static VTL_AppResult VTL_content_platform_vimeo_PatchPrivacy(
        const char *video_uri, const char *token,
        const char *field, const char *value) {
    JSON_Value *v = json_value_init_object();
    JSON_Value *priv = json_value_init_object();
    json_object_set_string(json_value_get_object(priv), field, value);
    json_object_set_value(json_value_get_object(v), "privacy", priv);
    char *body = json_serialize_to_string(v);
    json_value_free(v);
    if (!body) return VTL_res_kMemAllocErr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                            VTL_VIMEO_API_BASE "%s",
                                            video_uri) != VTL_res_kOk) {
        json_free_serialized_string(body);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpJson("PATCH", url, body, token);
    json_free_serialized_string(body);
    if (!resp) return VTL_res_vimeo_kHttpPatchError;
    free(resp);
    return VTL_res_kOk;
}

/* PATCH вложенного privacy.<field> = bool */
static VTL_AppResult VTL_content_platform_vimeo_PatchPrivacyBool(
        const char *video_uri, const char *token,
        const char *field, int value) {
    JSON_Value *v = json_value_init_object();
    JSON_Value *priv = json_value_init_object();
    json_object_set_boolean(json_value_get_object(priv), field, value);
    json_object_set_value(json_value_get_object(v), "privacy", priv);
    char *body = json_serialize_to_string(v);
    json_value_free(v);
    if (!body) return VTL_res_kMemAllocErr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                            VTL_VIMEO_API_BASE "%s",
                                            video_uri) != VTL_res_kOk) {
        json_free_serialized_string(body);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpJson("PATCH", url, body, token);
    json_free_serialized_string(body);
    if (!resp) return VTL_res_vimeo_kHttpPatchError;
    free(resp);
    return VTL_res_kOk;
}

/* PATCH embed.title.<field> = string */
static VTL_AppResult VTL_content_platform_vimeo_PatchEmbedTitle(
        const char *video_uri, const char *token,
        const char *field, const char *value) {
    JSON_Value *v = json_value_init_object();
    JSON_Value *emb = json_value_init_object();
    JSON_Value *title = json_value_init_object();
    json_object_set_string(json_value_get_object(title), field, value);
    json_object_set_value(json_value_get_object(emb), "title", title);
    json_object_set_value(json_value_get_object(v), "embed", emb);
    char *body = json_serialize_to_string(v);
    json_value_free(v);
    if (!body) return VTL_res_kMemAllocErr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                            VTL_VIMEO_API_BASE "%s",
                                            video_uri) != VTL_res_kOk) {
        json_free_serialized_string(body);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpJson("PATCH", url, body, token);
    json_free_serialized_string(body);
    if (!resp) return VTL_res_vimeo_kHttpPatchError;
    free(resp);
    return VTL_res_kOk;
}

/* PATCH embed.<field> = bool */
static VTL_AppResult VTL_content_platform_vimeo_PatchEmbedBool(
        const char *video_uri, const char *token,
        const char *field, int value) {
    JSON_Value *v = json_value_init_object();
    JSON_Value *emb = json_value_init_object();
    json_object_set_boolean(json_value_get_object(emb), field, value);
    json_object_set_value(json_value_get_object(v), "embed", emb);
    char *body = json_serialize_to_string(v);
    json_value_free(v);
    if (!body) return VTL_res_kMemAllocErr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                            VTL_VIMEO_API_BASE "%s",
                                            video_uri) != VTL_res_kOk) {
        json_free_serialized_string(body);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpJson("PATCH", url, body, token);
    json_free_serialized_string(body);
    if (!resp) return VTL_res_vimeo_kHttpPatchError;
    free(resp);
    return VTL_res_kOk;
}

/* PATCH embed.<field> = string */
static VTL_AppResult VTL_content_platform_vimeo_PatchEmbedString(
        const char *video_uri, const char *token,
        const char *field, const char *value) {
    JSON_Value *v = json_value_init_object();
    JSON_Value *emb = json_value_init_object();
    json_object_set_string(json_value_get_object(emb), field, value);
    json_object_set_value(json_value_get_object(v), "embed", emb);
    char *body = json_serialize_to_string(v);
    json_value_free(v);
    if (!body) return VTL_res_kMemAllocErr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                            VTL_VIMEO_API_BASE "%s",
                                            video_uri) != VTL_res_kOk) {
        json_free_serialized_string(body);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpJson("PATCH", url, body, token);
    json_free_serialized_string(body);
    if (!resp) return VTL_res_vimeo_kHttpPatchError;
    free(resp);
    return VTL_res_kOk;
}

/* Загрузить URI из файла. На ошибку — out не трогается. */
static VTL_AppResult VTL_content_platform_vimeo_LoadUri(
        const char *uri_file, char **out) {
    char *u = VTL_content_platform_vimeo_LoadFile(uri_file);
    if (!u) return VTL_res_kFileReadErr;
    *out = u;
    return VTL_res_kOk;
}

/* Загрузить URI и значение. На ошибку оба out = NULL и ничего не выделено. */
static VTL_AppResult VTL_content_platform_vimeo_LoadUriAndValue(
        const char *uri_file, const char *val_file,
        char **uri_out, char **val_out) {
    *uri_out = NULL;
    *val_out = NULL;
    char *u = VTL_content_platform_vimeo_LoadFile(uri_file);
    if (!u) return VTL_res_kFileReadErr;
    char *v = VTL_content_platform_vimeo_LoadFile(val_file);
    if (!v) {
        free(u);
        return VTL_res_kFileReadErr;
    }
    *uri_out = u;
    *val_out = v;
    return VTL_res_kOk;
}

/* ============================================================
 * ПУБЛИЧНЫЙ API
 * ============================================================ */

/* --- Загрузка --- */

VTL_AppResult VTL_content_platform_vimeo_video_SendNow(
        const VTL_Filename video_file_name) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    api_data.title = "Untitled";
    return VTL_content_platform_vimeo_DoUpload(&api_data, video_file_name);
}

VTL_AppResult VTL_content_platform_vimeo_video_w_text_SendNow(
        const VTL_Filename video_file_name,
        const VTL_Filename text_file_name) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *text = VTL_content_platform_vimeo_LoadFile(text_file_name);
    if (!text) return VTL_res_kFileReadErr;
    /* Первая строка — title, остальное — description */
    char *newline = strchr(text, '\n');
    if (newline) {
        *newline = '\0';
        api_data.title = text;
        api_data.description = newline + 1;
    } else {
        api_data.title = text;
        api_data.description = NULL;
    }
    VTL_AppResult r = VTL_content_platform_vimeo_DoUpload(
            &api_data, video_file_name);
    free(text);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_w_text_and_tags_SendNow(
        const VTL_Filename video_file_name,
        const VTL_Filename text_file_name,
        const VTL_Filename tags_file_name) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *text = VTL_content_platform_vimeo_LoadFile(text_file_name);
    if (!text) return VTL_res_kFileReadErr;
    char *tags = VTL_content_platform_vimeo_LoadFile(tags_file_name);
    if (!tags) {
        free(text);
        return VTL_res_kFileReadErr;
    }

    char *newline = strchr(text, '\n');
    if (newline) {
        *newline = '\0';
        api_data.title = text;
        api_data.description = newline + 1;
    } else {
        api_data.title = text;
        api_data.description = NULL;
    }
    api_data.tags_csv = tags;
    VTL_AppResult r = VTL_content_platform_vimeo_DoUpload(
            &api_data, video_file_name);
    free(text);
    free(tags);
    return r;
}

/* --- Метаданные --- */

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetTitle(
        const VTL_Filename video_uri_file, const VTL_Filename title_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, title_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchField(
            video_uri, api_data.access_token, "name", value);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetDescription(
        const VTL_Filename video_uri_file, const VTL_Filename description_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, description_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchField(
            video_uri, api_data.access_token, "description", value);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetTags(
        const VTL_Filename video_uri_file, const VTL_Filename tags_csv_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, tags_csv_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;

    JSON_Value *v = json_value_init_object();
    JSON_Value *ta = json_value_init_array();
    JSON_Array *arr = json_value_get_array(ta);
    char *copy = strdup(value), *sp = NULL;
    if (copy) {
        for (char *t = strtok_r(copy, ",", &sp); t;
             t = strtok_r(NULL, ",", &sp))
            json_array_append_string(arr, t);
        free(copy);
    }
    json_object_set_value(json_value_get_object(v), "tags", ta);
    char *body = json_serialize_to_string(v);
    json_value_free(v);

    VTL_AppResult r = VTL_res_kMemAllocErr;
    if (body) {
        char url[VTL_VIMEO_URL_BUF];
        if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                                VTL_VIMEO_API_BASE "%s",
                                                video_uri) != VTL_res_kOk) {
            json_free_serialized_string(body);
            free(video_uri);
            free(value);
            return VTL_res_kErr;
        }
        char *resp = VTL_content_platform_vimeo_HttpJson(
                "PATCH", url, body, api_data.access_token);
        json_free_serialized_string(body);
        r = resp ? VTL_res_kOk : VTL_res_vimeo_kHttpPatchError;
        free(resp);
    }
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetLanguage(
        const VTL_Filename video_uri_file, const VTL_Filename language_code_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, language_code_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchField(
            video_uri, api_data.access_token, "language", value);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetLicense(
        const VTL_Filename video_uri_file, const VTL_Filename license_code_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, license_code_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchField(
            video_uri, api_data.access_token, "license", value);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetContentRating(
        const VTL_Filename video_uri_file, const VTL_Filename rating_code_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, rating_code_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;

    JSON_Value *v = json_value_init_object();
    JSON_Value *ta = json_value_init_array();
    json_array_append_string(json_value_get_array(ta), value);
    json_object_set_value(json_value_get_object(v), "content_rating", ta);
    char *body = json_serialize_to_string(v);
    json_value_free(v);

    VTL_AppResult r = VTL_res_kMemAllocErr;
    if (body) {
        char url[VTL_VIMEO_URL_BUF];
        if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                                VTL_VIMEO_API_BASE "%s",
                                                video_uri) != VTL_res_kOk) {
            json_free_serialized_string(body);
            free(video_uri);
            free(value);
            return VTL_res_kErr;
        }
        char *resp = VTL_content_platform_vimeo_HttpJson(
                "PATCH", url, body, api_data.access_token);
        json_free_serialized_string(body);
        r = resp ? VTL_res_kOk : VTL_res_vimeo_kHttpPatchError;
        free(resp);
    }
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_meta_SetPassword(
        const VTL_Filename video_uri_file, const VTL_Filename password_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, password_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;

    JSON_Value *v = json_value_init_object();
    JSON_Object *obj = json_value_get_object(v);
    json_object_set_string(obj, "password", value);
    JSON_Value *priv = json_value_init_object();
    json_object_set_string(json_value_get_object(priv), "view", "password");
    json_object_set_value(obj, "privacy", priv);
    char *body = json_serialize_to_string(v);
    json_value_free(v);

    VTL_AppResult r = VTL_res_kMemAllocErr;
    if (body) {
        char url[VTL_VIMEO_URL_BUF];
        if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                                VTL_VIMEO_API_BASE "%s",
                                                video_uri) != VTL_res_kOk) {
            json_free_serialized_string(body);
            free(video_uri);
            free(value);
            return VTL_res_kErr;
        }
        char *resp = VTL_content_platform_vimeo_HttpJson(
                "PATCH", url, body, api_data.access_token);
        json_free_serialized_string(body);
        r = resp ? VTL_res_kOk : VTL_res_vimeo_kHttpPatchError;
        free(resp);
    }
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_Delete(
        const VTL_Filename video_uri_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUri(
            video_uri_file, &video_uri);
    if (lr != VTL_res_kOk) return lr;
    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(url, sizeof(url),
                                            VTL_VIMEO_API_BASE "%s",
                                            video_uri) != VTL_res_kOk) {
        free(video_uri);
        return VTL_res_kErr;
    }
    VTL_AppResult r = VTL_content_platform_vimeo_HttpDelete(
            url, api_data.access_token);
    free(video_uri);
    return r;
}

/* --- Приватность --- */

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetView(
        const VTL_Filename video_uri_file, const VTL_Filename view_code_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, view_code_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchPrivacy(
            video_uri, api_data.access_token, "view", value);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetEmbed(
        const VTL_Filename video_uri_file, const VTL_Filename embed_code_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, embed_code_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchPrivacy(
            video_uri, api_data.access_token, "embed", value);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetDownload(
        const VTL_Filename video_uri_file, const VTL_Filename bool_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, bool_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchPrivacyBool(
            video_uri, api_data.access_token, "download", value[0] == '1');
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetAdd(
        const VTL_Filename video_uri_file, const VTL_Filename bool_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, bool_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchPrivacyBool(
            video_uri, api_data.access_token, "add", value[0] == '1');
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_privacy_SetComments(
        const VTL_Filename video_uri_file, const VTL_Filename comments_code_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, comments_code_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchPrivacy(
            video_uri, api_data.access_token, "comments", value);
    free(video_uri);
    free(value);
    return r;
}

/* --- Плеер --- */

VTL_AppResult VTL_content_platform_vimeo_video_player_SetColor(
        const VTL_Filename video_uri_file, const VTL_Filename hex_color_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, hex_color_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchEmbedString(
            video_uri, api_data.access_token, "color", value);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowTitle(
        const VTL_Filename video_uri_file, const VTL_Filename bool_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, bool_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchEmbedTitle(
            video_uri, api_data.access_token,
            "name", value[0] == '1' ? "show" : "hide");
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowByline(
        const VTL_Filename video_uri_file, const VTL_Filename bool_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, bool_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchEmbedTitle(
            video_uri, api_data.access_token,
            "owner", value[0] == '1' ? "show" : "hide");
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_player_SetShowPortrait(
        const VTL_Filename video_uri_file, const VTL_Filename bool_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, bool_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchEmbedTitle(
            video_uri, api_data.access_token,
            "portrait", value[0] == '1' ? "show" : "hide");
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_player_SetAutoplay(
        const VTL_Filename video_uri_file, const VTL_Filename bool_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, bool_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchEmbedBool(
            video_uri, api_data.access_token, "autoplay", value[0] == '1');
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_player_SetLoop(
        const VTL_Filename video_uri_file, const VTL_Filename bool_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, bool_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    VTL_AppResult r = VTL_content_platform_vimeo_PatchEmbedBool(
            video_uri, api_data.access_token, "loop", value[0] == '1');
    free(video_uri);
    free(value);
    return r;
}

/* --- Получение информации --- */

VTL_AppResult VTL_content_platform_vimeo_video_GetUploadStatus(
        const VTL_Filename video_uri_file, const VTL_Filename out_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUri(
            video_uri_file, &video_uri);
    if (lr != VTL_res_kOk) return lr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            url, sizeof(url),
            VTL_VIMEO_API_BASE "%s?fields=upload.status",
            video_uri) != VTL_res_kOk) {
        free(video_uri);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpGet(url, api_data.access_token);
    free(video_uri);
    if (!resp) return VTL_res_kErr;

    JSON_Value *rv = json_parse_string(resp);
    free(resp);
    if (!rv) return VTL_res_vimeo_kJsonParseError;

    JSON_Object *ro = json_value_get_object(rv);
    JSON_Object *uplo = json_object_get_object(ro, "upload");
    const char *st = uplo ? json_object_get_string(uplo, "status") : NULL;
    if (!st) {
        json_value_free(rv);
        return VTL_res_vimeo_kJsonParseError;
    }
    VTL_AppResult r = VTL_content_platform_vimeo_WriteFile(out_file, st);
    json_value_free(rv);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_GetEmbedCode(
        const VTL_Filename video_uri_file, const VTL_Filename out_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUri(
            video_uri_file, &video_uri);
    if (lr != VTL_res_kOk) return lr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            url, sizeof(url),
            VTL_VIMEO_API_BASE "%s?fields=embed.html",
            video_uri) != VTL_res_kOk) {
        free(video_uri);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpGet(url, api_data.access_token);
    free(video_uri);
    if (!resp) return VTL_res_kErr;

    JSON_Value *rv = json_parse_string(resp);
    free(resp);
    if (!rv) return VTL_res_vimeo_kJsonParseError;

    JSON_Object *ro = json_value_get_object(rv);
    JSON_Object *emb = json_object_get_object(ro, "embed");
    const char *html = emb ? json_object_get_string(emb, "html") : NULL;
    if (!html) {
        json_value_free(rv);
        return VTL_res_vimeo_kJsonParseError;
    }
    VTL_AppResult r = VTL_content_platform_vimeo_WriteFile(out_file, html);
    json_value_free(rv);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_GetMyVideos(
        int page, int per_page, const VTL_Filename out_file) {
    if (page < 1 || per_page < 1) return VTL_res_kErr;

    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            url, sizeof(url),
            VTL_VIMEO_API_BASE "/me/videos?page=%d&per_page=%d&sort=date",
            page, per_page) != VTL_res_kOk)
        return VTL_res_kErr;

    char *resp = VTL_content_platform_vimeo_HttpGet(url, api_data.access_token);
    if (!resp) return VTL_res_kErr;
    VTL_AppResult r = VTL_content_platform_vimeo_WriteFile(out_file, resp);
    free(resp);
    return r;
}

/* --- Домены --- */

VTL_AppResult VTL_content_platform_vimeo_video_domain_Add(
        const VTL_Filename video_uri_file, const VTL_Filename domain_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, domain_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    char url[VTL_VIMEO_URL_BUF];
    VTL_AppResult br = VTL_content_platform_vimeo_BuildUrl(
            url, sizeof(url),
            VTL_VIMEO_API_BASE "%s/privacy/domains/%s", video_uri, value);
    if (br != VTL_res_kOk) {
        free(video_uri);
        free(value);
        return br;
    }
    VTL_AppResult r = VTL_content_platform_vimeo_HttpPut(
            url, api_data.access_token);
    free(video_uri);
    free(value);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_domain_Remove(
        const VTL_Filename video_uri_file, const VTL_Filename domain_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri, *value;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUriAndValue(
            video_uri_file, domain_file, &video_uri, &value);
    if (lr != VTL_res_kOk) return lr;
    char url[VTL_VIMEO_URL_BUF];
    VTL_AppResult br = VTL_content_platform_vimeo_BuildUrl(
            url, sizeof(url),
            VTL_VIMEO_API_BASE "%s/privacy/domains/%s", video_uri, value);
    if (br != VTL_res_kOk) {
        free(video_uri);
        free(value);
        return br;
    }
    VTL_AppResult r = VTL_content_platform_vimeo_HttpDelete(
            url, api_data.access_token);
    free(video_uri);
    free(value);
    return r;
}

/* --- Обложка --- */

VTL_AppResult VTL_content_platform_vimeo_video_thumbnail_Upload(
        const VTL_Filename video_uri_file, const VTL_Filename image_file_name) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUri(
            video_uri_file, &video_uri);
    if (lr != VTL_res_kOk) return lr;

    /* Шаг 1: создать слот */
    char slot_url[VTL_VIMEO_URL_BUF];
    VTL_AppResult br = VTL_content_platform_vimeo_BuildUrl(
            slot_url, sizeof(slot_url),
            VTL_VIMEO_API_BASE "%s/pictures", video_uri);
    free(video_uri);
    if (br != VTL_res_kOk) return br;

    char *slot_resp = VTL_content_platform_vimeo_HttpJson(
            "POST", slot_url, "{\"active\":true}", api_data.access_token);
    if (!slot_resp) return VTL_res_vimeo_kHttpPostError;

    JSON_Value *rv = json_parse_string(slot_resp);
    free(slot_resp);
    if (!rv) return VTL_res_vimeo_kJsonParseError;

    JSON_Object *ro = json_value_get_object(rv);
    const char *link = json_object_get_string(ro, "link");
    const char *pic_uri = json_object_get_string(ro, "uri");

    if (!link || !pic_uri) {
        json_value_free(rv);
        return VTL_res_vimeo_kJsonParseError;
    }

    char link_copy[VTL_VIMEO_UPLOAD_LINK_BUF];
    char pic_uri_copy[VTL_VIMEO_PIC_URI_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            link_copy, sizeof(link_copy), "%s", link) != VTL_res_kOk
        || VTL_content_platform_vimeo_BuildUrl(
            pic_uri_copy, sizeof(pic_uri_copy), "%s", pic_uri) != VTL_res_kOk) {
        json_value_free(rv);
        return VTL_res_kErr;
    }
    json_value_free(rv);

    /* Шаг 2: загрузить изображение */
    long img_size = VTL_content_platform_vimeo_GetFileSize(image_file_name);
    if (img_size < 0) return VTL_res_kFileOpenErr;

    FILE *fp = fopen(image_file_name, "rb");
    if (!fp) return VTL_res_kFileOpenErr;
    char *img_buf = malloc((size_t) img_size);
    if (!img_buf) {
        fclose(fp);
        return VTL_res_kMemAllocErr;
    }
    size_t img_read = fread(img_buf, 1, (size_t) img_size, fp);
    fclose(fp);
    if ((long) img_read != img_size) {
        free(img_buf);
        return VTL_res_kFileReadErr;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        free(img_buf);
        return VTL_res_kErr;
    }

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: image/jpeg");

    FILE *mem = fmemopen(img_buf, (size_t) img_size, "rb");
    if (!mem) {
        free(img_buf);
        curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
        return VTL_res_kErr;
    }
    curl_easy_setopt(curl, CURLOPT_URL, link_copy);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, mem);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) img_size);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     VTL_content_platform_vimeo_DiscardWriteCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, VTL_VIMEO_THUMB_TIMEOUT_SEC);
    VTL_content_platform_vimeo_ApplyCurlSecurity(curl);
    CURLcode res = curl_easy_perform(curl);
    fclose(mem);
    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    free(img_buf);
    if (res != CURLE_OK) return VTL_res_kErr;

    /* Шаг 3: активировать */
    char act_url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            act_url, sizeof(act_url),
            VTL_VIMEO_API_BASE "%s", pic_uri_copy) != VTL_res_kOk)
        return VTL_res_kErr;
    char *act_resp = VTL_content_platform_vimeo_HttpJson(
            "PATCH", act_url, "{\"active\":true}", api_data.access_token);
    if (!act_resp) return VTL_res_vimeo_kHttpPatchError;
    free(act_resp);
    return VTL_res_kOk;
}

/* --- Главы --- */

VTL_AppResult VTL_content_platform_vimeo_video_chapter_Add(
        const VTL_Filename video_uri_file, const VTL_Filename chapter_data_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUri(
            video_uri_file, &video_uri);
    if (lr != VTL_res_kOk) return lr;

    char *data = VTL_content_platform_vimeo_LoadFile(chapter_data_file);
    if (!data) {
        free(video_uri);
        return VTL_res_kFileReadErr;
    }

    /* Формат файла: первая строка = время в секундах, вторая = заголовок */
    double timecode = 0.0;
    char title[VTL_VIMEO_CHAPTER_TITLE_BUF] = "Chapter";
    char *newline = strchr(data, '\n');
    char *time_str = data;
    if (newline) {
        *newline = '\0';
        snprintf(title, sizeof(title), "%s", newline + 1);
    }
    char *endptr = NULL;
    errno = 0;
    double parsed = strtod(time_str, &endptr);
    if (endptr != time_str && errno == 0) timecode = parsed;
    free(data);

    JSON_Value *v = json_value_init_object();
    JSON_Object *obj = json_value_get_object(v);
    json_object_set_number(obj, "timecode", timecode);
    json_object_set_string(obj, "title", title);
    char *body = json_serialize_to_string(v);
    json_value_free(v);

    VTL_AppResult r = VTL_res_kMemAllocErr;
    if (body) {
        char url[VTL_VIMEO_URL_BUF];
        if (VTL_content_platform_vimeo_BuildUrl(
                url, sizeof(url),
                VTL_VIMEO_API_BASE "%s/chapters", video_uri) != VTL_res_kOk) {
            json_free_serialized_string(body);
            free(video_uri);
            return VTL_res_kErr;
        }
        char *resp = VTL_content_platform_vimeo_HttpJson(
                "POST", url, body, api_data.access_token);
        json_free_serialized_string(body);
        r = resp ? VTL_res_kOk : VTL_res_vimeo_kHttpPostError;
        free(resp);
    }
    free(video_uri);
    return r;
}

VTL_AppResult VTL_content_platform_vimeo_video_chapter_GetAll(
        const VTL_Filename video_uri_file, const VTL_Filename out_file) {
    VTL_net_api_data_vimeo api_data;
    VTL_AppResult ir = VTL_content_platform_vimeo_ApiDataInit(&api_data);
    if (ir != VTL_res_kOk) return ir;
    char *video_uri;
    VTL_AppResult lr = VTL_content_platform_vimeo_LoadUri(
            video_uri_file, &video_uri);
    if (lr != VTL_res_kOk) return lr;

    char url[VTL_VIMEO_URL_BUF];
    if (VTL_content_platform_vimeo_BuildUrl(
            url, sizeof(url),
            VTL_VIMEO_API_BASE "%s/chapters", video_uri) != VTL_res_kOk) {
        free(video_uri);
        return VTL_res_kErr;
    }
    char *resp = VTL_content_platform_vimeo_HttpGet(url, api_data.access_token);
    free(video_uri);
    if (!resp) return VTL_res_kErr;
    VTL_AppResult r = VTL_content_platform_vimeo_WriteFile(out_file, resp);
    free(resp);
    return r;
}
