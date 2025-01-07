/*
 * server.c
 *
 *  Created on: 23 ene 2024
 *      Author: Joruso
 */

#include "server.h"
#include <sys/param.h>
#include "cultibot.h"
#include "wifi.h"
#include "nvs_manager.h"

extern const char dash_start[] asm("_binary_dash_html_start");
extern const char dash_end[] asm("_binary_dash_html_end");

extern const char config_start[] asm("_binary_config_html_start");
extern const char config_end[] asm("_binary_config_html_end");

httpd_handle_t server = NULL;

static const char *TAG = "REST Server";

esp_err_t get_json_handler(httpd_req_t *req)
{

    return ESP_OK;
}

esp_err_t get_dash_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, dash_start, HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG,"Received get request");
    return ESP_OK;
}

esp_err_t get_config_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, config_start, HTTPD_RESP_USE_STRLEN);
}

esp_err_t post_config_wifi_handler(httpd_req_t *req)
{

    char *buff, *ssid, *password;
    buff = (char *)malloc(((*req).content_len + 1) * sizeof(char));
    ssid = (char *)malloc(32 * sizeof(char));
    password = (char *)malloc(64 * sizeof(char));
    if (ssid == NULL || password == NULL || buff == NULL)
    {
        ESP_LOGE(TAG, "Malloc fail");
        return ESP_ERR_NO_MEM;
    }

    httpd_req_recv(req, buff, (*req).content_len);

    buff[(*req).content_len] = '\0';

    // ESP_LOGI (TAG, "%s", buff);

    if (sscanf(buff, "ssid=%31[^&]&password=%63[^&]", ssid, password) == 2)
    {
        if (sizeof(ssid)==0)ssid="clear";
        if (sizeof(password)==0)password="clear";
        
        ESP_LOGI(TAG, "%s", ssid);
        ESP_LOGI(TAG, "%s", password);
    	ESP_ERROR_CHECK(nvs_set_value_str(WIFI_SSID,ssid));
	    ESP_ERROR_CHECK(nvs_set_value_str(WIFI_PASS,password));

        free(buff);
        free(ssid);
        free(password);

        wifi_connect_sta();
    }

    return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req)
{

    return ESP_OK;
}

httpd_uri_t dash_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_dash_handler,
    .user_ctx = NULL};

httpd_uri_t config_get = {
    .uri = "/config",
    .method = HTTP_GET,
    .handler = get_config_handler,
    .user_ctx = NULL};

httpd_uri_t json_get = {
    .uri = "/info_json",
    .method = HTTP_GET,
    .handler = get_json_handler,
    .user_ctx = NULL};

httpd_uri_t uri_post = {
    .uri = "/uri",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL};

httpd_uri_t config_wifi_post = {
    .uri = "/save_wifi",
    .method = HTTP_POST,
    .handler = post_config_wifi_handler,
    .user_ctx = NULL};

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &dash_get);
        httpd_register_uri_handler(server, &config_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &json_get);
        httpd_register_uri_handler(server, &config_wifi_post);
    }
}

void stop_webserver(void)
{
    if (server)
    {
        httpd_stop(server);
    }
}
