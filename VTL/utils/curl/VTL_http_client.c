#include "VTL/utils/curl/VTL_http_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static size_t VTL_curl_http_client_WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    HttpResponse *resp = (HttpResponse *)userp;

    if (resp->body == NULL) {
        resp->body = calloc(1, 1);
        if (!resp->body) {
            return 0;
        }
    }

    size_t current_len = strlen(resp->body);
    char *new_body = realloc(resp->body, current_len + total_size + 1);
    if (!new_body) {
        free(resp->body);
        resp->body = NULL;
        return 0; 
    }
    resp->body = new_body;

    memcpy(resp->body + current_len, contents, total_size);
    resp->body[current_len + total_size] = '\0';

    return total_size;
}

int VTL_curl_http_client_Request(const char *url, HttpMethod method, const HttpRequest *request, HttpResponse *response) {
    if (!url || !response) return 0;

    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    response->body = NULL;
    response->status_code = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    if (request && request->content_type) {
        char ct_header[128];
        snprintf(ct_header, sizeof(ct_header), "Content-Type: %s", request->content_type);
        request->headers = curl_slist_append(request->headers, ct_header);
    }

    if (request && request->headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request->headers);
    }

    switch (method) {
        case HTTP_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if (request && request->body) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
            }
            break;

        case HTTP_PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            if (request && request->body) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
            }
            break;

        case HTTP_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;

        case HTTP_GET:
        default:
            break;
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return 0;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    curl_easy_cleanup(curl);
    return 1;
}

void VTL_curl_http_client_ResponseCleanup(HttpResponse *response) {
    if (!response) return;
    if (response->body) {
        free(response->body);
        response->body = NULL;
    }
    response->status_code = 0;
}

void VTL_curl_http_client_AddHeader(HttpRequest *request, const char *header_line) {
    if (!request || !header_line) return;
    request->headers = curl_slist_append(request->headers, header_line);
}


int VTL_curl_http_client_RequestMultipart(
    const char              *url,
    struct curl_httppost    *form,
    const struct curl_slist *extra_headers,
    HttpResponse            *response
) {

    if (!url || !response) {
        if (form) curl_formfree(form);
        return 0;
    }

    struct curl_slist *local_headers = NULL;
    if (extra_headers) {
        struct curl_slist *iter = (struct curl_slist *)extra_headers;
        while (iter) {
            local_headers = curl_slist_append(local_headers, iter->data);
            iter = iter->next;
        }
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        if (local_headers) curl_slist_free_all(local_headers);
        curl_formfree(form);
        return 0;
    }

    response->body = NULL;
    response->status_code = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VTL_curl_http_client_WriteCallback);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Включить детальный вывод
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

    if (local_headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, local_headers);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        if (local_headers) curl_slist_free_all(local_headers);
        curl_formfree(form);
        return 0;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    curl_easy_cleanup(curl);
    if (local_headers) curl_slist_free_all(local_headers);
    curl_formfree(form);
    return 1;
}
