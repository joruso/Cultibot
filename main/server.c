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
#include "AM2302.h"
#include "cJSON.h"
#include <math.h>

extern const char dash_start[] asm("_binary_dash_html_start");
extern const char dash_end[] asm("_binary_dash_html_end");

extern const char config_start[] asm("_binary_config2_html_start");
extern const char config_end[] asm("_binary_config2_html_end");

httpd_handle_t server = NULL;

static const char *TAG = "REST Server";

esp_err_t get_dash_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    const uint32_t len_dash = dash_end - dash_start;
    return httpd_resp_send(req, dash_start, len_dash);
}

esp_err_t get_config_json_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    char *wifi_ssid;
    size_t ssid_tam;
    ESP_ERROR_CHECK(nvs_get_value_str(WIFI_SSID, NULL, &ssid_tam));
    wifi_ssid = (char *)malloc(ssid_tam * sizeof(char));
    if (wifi_ssid == NULL)
    {
        ESP_LOGE(TAG, "Malloc fail");
        free(wifi_ssid);
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(nvs_get_value_str(WIFI_SSID, wifi_ssid, &ssid_tam));
    cJSON_AddStringToObject(root, "SSID", wifi_ssid);

    uint8_t num, num2;
    ESP_ERROR_CHECK(nvs_get_value_num_u8(WATER_LITER, &num));
    ESP_ERROR_CHECK(nvs_get_value_num_u8(WATER_DECILITER, &num2));
    float lit = (float)num + ((float)num2 * 0.1f);

    lit = round(lit * 10) / 10;

    ESP_LOGI(TAG,"%f",lit);
    cJSON_AddNumberToObject(root, "liters", lit);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_1_ML, &num));
    cJSON_AddNumberToObject(root, "additive1", num);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_2_ML, &num));
    cJSON_AddNumberToObject(root, "additive2", num);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_3_ML, &num));
    cJSON_AddNumberToObject(root, "additive3", num);

    char str[10];
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_HOUR, &num));
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_MIN, &num2));
    snprintf(str, sizeof(str), "%02u:%02u", num, num2);
    cJSON_AddStringToObject(root, "HourMinON", str);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_HOUR, &num));
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_MIN, &num2));
    snprintf(str, sizeof(str), "%02u:%02u", num, num2);
    cJSON_AddStringToObject(root, "HourMinOFF", str);

    const char *buff = cJSON_Print(root);
    ESP_LOGI(TAG, "%s", buff);

    free(wifi_ssid);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buff, HTTPD_RESP_USE_STRLEN);
}

esp_err_t get_config_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    const uint32_t len_confi = config_end - config_start;
    return httpd_resp_send(req, config_start, len_confi);
}

esp_err_t post_config_handler(httpd_req_t *req)
{

    char *buff;
    int recv_len = req->content_len;

    buff = (char *)malloc((recv_len + 1) * sizeof(char));
    if (buff == NULL)
    {
        ESP_LOGE(TAG, "Malloc buff fail");
        return ESP_ERR_NO_MEM;
    }

    int ret = httpd_req_recv(req, buff, recv_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s", buff);

    cJSON *aux, *root;

    root = cJSON_Parse(buff);

    aux = cJSON_GetObjectItem(root, "ssid");
    if (cJSON_IsString(aux))
    {
        ESP_LOGI(TAG, "%s", aux->valuestring);
        ESP_ERROR_CHECK(nvs_set_value_str(WIFI_SSID, aux->valuestring));

        aux = cJSON_GetObjectItem(root, "password");
        if (cJSON_IsString(aux))
        {
            ESP_LOGI(TAG, "%s", aux->valuestring);
            ESP_ERROR_CHECK(nvs_set_value_str(WIFI_PASS, aux->valuestring));

            wifi_connect_sta();
        }
    }

    aux = cJSON_GetObjectItem(root, "liters");
    if (cJSON_IsNumber(aux))
    {

        ESP_LOGI(TAG, "%f", aux->valuedouble);
        uint8_t liter = (uint8_t)aux->valueint;
        ESP_LOGI(TAG, "%i", aux->valueint);
        double temp = aux->valuedouble;
        uint8_t deciL = (uint8_t)(10 * (temp - liter));
        ESP_LOGI(TAG, "%i", deciL);

        ESP_ERROR_CHECK(nvs_set_value_num_u8(WATER_LITER, liter));
        ESP_ERROR_CHECK(nvs_set_value_num_u8(WATER_DECILITER,deciL));
    }

    aux = cJSON_GetObjectItem(root, "additive1");
    if (cJSON_IsNumber(aux))
    {
        uint8_t additive = aux->valueint;
        ESP_ERROR_CHECK(nvs_set_value_num_u8(FERTI_1_ML, additive));
    }
    aux = cJSON_GetObjectItem(root, "additive2");
    if (cJSON_IsNumber(aux))
    {
        uint8_t additive = aux->valueint;
        ESP_ERROR_CHECK(nvs_set_value_num_u8(FERTI_2_ML, additive));
    }
    aux = cJSON_GetObjectItem(root, "additive3");
    if (cJSON_IsNumber(aux))
    {
        uint8_t additive = aux->valueint;
        ESP_ERROR_CHECK(nvs_set_value_num_u8(FERTI_3_ML, additive));
    }
    aux = cJSON_GetObjectItem(root, "HourMinON");
    if (cJSON_IsString(aux))
    {
        char *temporal = aux->valuestring;
        uint8_t hour, minutes;
        if (sscanf(temporal, "%hhu:%hhu", &hour, &minutes) == 2)
        {
            ESP_ERROR_CHECK(nvs_set_value_num_u8(LIGHT_ON_HOUR, hour));
            ESP_ERROR_CHECK(nvs_set_value_num_u8(LIGHT_ON_MIN, minutes));
            // ESP_LOGI(TAG, "%u", hour);
            // ESP_LOGI(TAG, "%u", minutes);
        }
    }
    aux = cJSON_GetObjectItem(root, "HourMinOFF");
    if (cJSON_IsString(aux))
    {
        char *temporal = aux->valuestring;
        uint8_t hour, minutes;
        if (sscanf(temporal, "%hhu:%hhu", &hour, &minutes) == 2)
        {
            ESP_ERROR_CHECK(nvs_set_value_num_u8(LIGHT_OFF_HOUR, hour));
            ESP_ERROR_CHECK(nvs_set_value_num_u8(LIGHT_OFF_MIN, minutes));
            // ESP_LOGI(TAG, "%u", hour);
            // ESP_LOGI(TAG, "%u", minutes);
        }
    }

    climate_load_parameters_from_nvs();

    free(buff);
    free(aux);
    free(root);
    httpd_resp_send_custom_err(req, HTTPD_200, "OK");

    return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req)
{

    return ESP_OK;
}

esp_err_t get_json_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "temperature", get_temperature());
    cJSON_AddNumberToObject(root, "rel_humidity", get_rel_humidity());
    const char *buff = cJSON_Print(root);

    // ESP_LOGI(TAG,"%s",buff);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buff, HTTPD_RESP_USE_STRLEN);
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
    .uri = "/save_config",
    .method = HTTP_POST,
    .handler = post_config_handler,
    .user_ctx = NULL};

httpd_uri_t config_wifi_get = {
    .uri = "/config/json",
    .method = HTTP_GET,
    .handler = get_config_json_handler,
    .user_ctx = NULL};

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &dash_get);
        httpd_register_uri_handler(server, &config_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &json_get);
        httpd_register_uri_handler(server, &config_wifi_post);
        httpd_register_uri_handler(server, &config_wifi_get);
    }
}

void stop_webserver(void)
{
    if (server)
    {
        httpd_stop(server);
    }
}
