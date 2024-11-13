/*
 * server.c
 *
 *  Created on: 23 ene 2024
 *      Author: Joruso
 */

#include "server.h"
#include <sys/param.h>
#include "cultibot.h"

extern const char dash_start[] asm("_binary_dash_html_start");
extern const char dash_end[] asm("_binary_dash_html_end");

extern const char config_start[] asm("_binary_config_html_start");
extern const char config_end[] asm("_binary_config_html_end");

httpd_handle_t server = NULL;

esp_err_t get_json_handler(httpd_req_t *req)
{
    //httpd_resp_send(req, dash_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t get_dash_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, dash_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t get_config_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, config_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


esp_err_t post_handler(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    /* Send a simple response */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


httpd_uri_t dash_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_dash_handler,
    .user_ctx = NULL
};

httpd_uri_t config_get = {
    .uri      = "/config",
    .method   = HTTP_GET,
    .handler  = get_config_handler,
    .user_ctx = NULL
};

httpd_uri_t json_get = {
    .uri      = "/info_json",
    .method   = HTTP_GET,
    .handler  = get_json_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post = {
    .uri      = "/uri",
    .method   = HTTP_POST,
    .handler  = post_handler,
    .user_ctx = NULL
};

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &dash_get);
        httpd_register_uri_handler(server, &config_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &json_get);
    }
}

void stop_webserver(void)
{
    if (server) {
        httpd_stop(server);
    }
}
 