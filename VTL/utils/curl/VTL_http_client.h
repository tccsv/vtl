#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <curl/curl.h>


typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
} HttpMethod;


typedef struct {
    char *content_type;           // MIME-тип тела запроса (например, "application/json")
    char *body;                   // Тело запроса
    struct curl_slist *headers;   // Дополнительные заголовки (список строк "Имя: значение")
} HttpRequest;


typedef struct {
    long status_code;   
    char *body;
} HttpResponse;

int VTL_curl_http_client_Request(const char *url, HttpMethod method, const HttpRequest *request, HttpResponse *response);

void VTL_curl_http_client_ResponseCleanup(HttpResponse *response);

void VTL_curl_http_client_AddHeader(HttpRequest *request, const char *header_line);

int VTL_curl_http_client_RequestMultipart(const char *url, struct curl_httppost *form, const struct curl_slist *extra_headers, HttpResponse *response);

#endif