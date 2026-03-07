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
#include "cJSON.h"
#include <math.h>
#include "sntp.h"
#include "espnow_water.h"
#include "climate_control.h"

extern const char dash_start[] asm("_binary_dash_html_start");
extern const char dash_end[] asm("_binary_dash_html_end");

extern const char config_start[] asm("_binary_config_html_start");
extern const char config_end[] asm("_binary_config_html_end");

extern const char riego_start[] asm("_binary_riego_html_start");
extern const char riego_end[] asm("_binary_riego_html_end");

extern const char clima_start[] asm("_binary_clima_html_start");
extern const char clima_end[] asm("_binary_clima_html_end");

httpd_handle_t server = NULL;

static const char *TAG = "REST Server";

esp_err_t get_dash_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    const uint32_t len_dash = dash_end - dash_start;
    return httpd_resp_send(req, dash_start, len_dash);
}
esp_err_t get_config_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    const uint32_t len_confi = config_end - config_start;
    return httpd_resp_send(req, config_start, len_confi);
}
esp_err_t get_riego_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    const uint32_t len_confi = riego_end - riego_start;
    return httpd_resp_send(req, riego_start, len_confi);
}
esp_err_t get_clima_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "text/html");
    const uint32_t len_confi = clima_end - clima_start;
    return httpd_resp_send(req, clima_start, len_confi);
}

esp_err_t post_riego_json_handler(httpd_req_t *req)
{
    esp_err_t err;
    char *buff;
    int recv_len = req->content_len;

    buff = (char *)malloc((recv_len) * sizeof(char));
    if (buff == NULL)
    {
        ESP_LOGE(TAG, "Malloc buff fail");
        free(buff);
        return ESP_ERR_NO_MEM;
    }

    int ret = httpd_req_recv(req, buff, recv_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        free(buff);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s", buff);

    cJSON *aux, *root;

    root = cJSON_Parse(buff);

    aux = cJSON_GetObjectItem(root, "comando");
    if (cJSON_IsNumber(aux))
    {
        uint8_t value = cJSON_GetNumberValue(aux);
        switch (value)
        {
        case COMANDO_TEST:
            aux = cJSON_GetObjectItem(root, "test_seconds");
            if (cJSON_IsNumber(aux))
            {
                value = cJSON_GetNumberValue(aux);
                err = espnow_test(value);
                if (err == ESP_ERR_ESPNOW_NOT_FOUND)
                {
                    httpd_resp_send_custom_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not conected Water");
                }
            }

            break;
        case COMANDO_IRRIGATE:
            irrigate_now();
            break;

        case COMANDO_SET_IRRIGATION:
            parameterIrrigation_t parameters;
            aux = cJSON_GetObjectItem(root, FERTI_1_ML);
            if (cJSON_IsNumber(aux))
            {
                value = cJSON_GetNumberValue(aux);
                if (value > 0)
                {
                    parameters.aditive1_ml_per_L = value;
                }
            }
            else
            {
                parameters.aditive1_ml_per_L = 0;
            }
            aux = cJSON_GetObjectItem(root, FERTI_2_ML);
            if (cJSON_IsNumber(aux))
            {
                value = cJSON_GetNumberValue(aux);
                if (value > 0)
                {
                    parameters.aditive2_ml_per_L = value;
                }
            }
            else
            {
                parameters.aditive2_ml_per_L = 0;
            }
            aux = cJSON_GetObjectItem(root, FERTI_3_ML);
            if (cJSON_IsNumber(aux))
            {
                value = cJSON_GetNumberValue(aux);
                if (value > 0)
                {
                    parameters.aditive3_ml_per_L = value;
                }
            }
            else
            {
                parameters.aditive3_ml_per_L = 0;
            }
            aux = cJSON_GetObjectItem(root, WATER_LITER);
            if (cJSON_IsNumber(aux))
            {
                parameters.water_dl = cJSON_GetNumberValue(aux) * 10;
            }
            else
            {
                parameters.water_dl = 0;
            }

            err = espnow_calibrate(parameters);
            if (err == ESP_ERR_ESPNOW_NOT_FOUND)
            {
                httpd_resp_send_custom_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not conected Water");
            }

            break;
        default:
            httpd_resp_send_custom_err(req, HTTPD_400, "Error comand");
            free(buff);
            free(root);
            free(aux);
            return ESP_ERR_INVALID_ARG;
            break;
        }
    }

    httpd_resp_send_custom_err(req, HTTPD_200, "OK");

    free(buff);
    free(root);
    free(aux);

    return ESP_OK;
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
    cJSON_AddStringToObject(root, WIFI_SSID, wifi_ssid);

    uint8_t num, num2;
    ESP_ERROR_CHECK(nvs_get_value_num_u8(WATER_LITER, &num));
    ESP_ERROR_CHECK(nvs_get_value_num_u8(WATER_DECILITER, &num2));
    float lit = (float)num + ((float)num2 * 0.1f);

    lit = round(lit * 10) / 10;

    ESP_LOGI(TAG, "%f", lit);
    cJSON_AddNumberToObject(root, WATER_LITER, lit);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_1_ML, &num));
    cJSON_AddNumberToObject(root, FERTI_1_ML, num);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_2_ML, &num));
    cJSON_AddNumberToObject(root, FERTI_2_ML, num);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_3_ML, &num));
    cJSON_AddNumberToObject(root, FERTI_3_ML, num);

    char str[10];
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_HOUR, &num));
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_MIN, &num2));
    snprintf(str, sizeof(str), "%02u:%02u", num, num2);
    cJSON_AddStringToObject(root, "HourMinON", str);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_HOUR, &num));
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_MIN, &num2));
    snprintf(str, sizeof(str), "%02u:%02u", num, num2);
    cJSON_AddStringToObject(root, "HourMinOFF", str);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_MONTH, &num));
    cJSON_AddNumberToObject(root, LAST_IRRI_MONTH, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_DAY, &num));
    cJSON_AddNumberToObject(root, LAST_IRRI_DAY, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_HOUR, &num));
    cJSON_AddNumberToObject(root, LAST_IRRI_HOUR, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_MIN, &num));
    cJSON_AddNumberToObject(root, LAST_IRRI_MIN, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(HOURS_BETWEEN_IRRIGATIONS, &num));
    cJSON_AddNumberToObject(root, HOURS_BETWEEN_IRRIGATIONS, num);

    
    ESP_ERROR_CHECK(nvs_get_value_num_u8(TEMP_DAY, &num));
    cJSON_AddNumberToObject(root, TEMP_DAY, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(TEMP_NIGHT, &num));
    cJSON_AddNumberToObject(root, TEMP_NIGHT, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(HISTERESIS_DAY, &num));
    cJSON_AddNumberToObject(root, HISTERESIS_DAY, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(HISTERESIS_NIGHT, &num));
    cJSON_AddNumberToObject(root, HISTERESIS_NIGHT, num);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(HUMEDAD_REL_DAY, &num));
    cJSON_AddNumberToObject(root, HUMEDAD_REL_DAY, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(HUMEDAD_REL_NIGHT, &num));
    cJSON_AddNumberToObject(root, HUMEDAD_REL_NIGHT, num);

    ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_VENT_STATUS, &num));
    cJSON_AddNumberToObject(root, INDOOR_VENT_STATUS, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_VENT_M_ON, &num));
    cJSON_AddNumberToObject(root, INDOOR_VENT_M_ON, num);
    ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_VENT_M_OFF, &num));
    cJSON_AddNumberToObject(root, INDOOR_VENT_M_OFF, num);

    const char *buff = cJSON_Print(root);
    ESP_LOGI(TAG, "%s", buff);

    free(wifi_ssid);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buff, HTTPD_RESP_USE_STRLEN);
}
esp_err_t post_config_json_handler(httpd_req_t *req)
{

    char *buff;
    int recv_len = req->content_len;

    buff = (char *)malloc((recv_len) * sizeof(char));
    if (buff == NULL)
    {
        ESP_LOGE(TAG, "Malloc buff fail");
        free(buff);
        return ESP_ERR_NO_MEM;
    }

    int ret = httpd_req_recv(req, buff, recv_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        free(buff);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s", buff);

    cJSON *aux, *root;

    root = cJSON_Parse(buff);

    aux = cJSON_GetObjectItem(root, WIFI_SSID);
    if (cJSON_IsString(aux))
    {
        ESP_LOGI(TAG, "%s", aux->valuestring);
        ESP_ERROR_CHECK(nvs_set_value_str(WIFI_SSID, aux->valuestring));
    }
    aux = cJSON_GetObjectItem(root, WIFI_PASS);
    if (cJSON_IsString(aux))
    {
        ESP_LOGI(TAG, "%s", aux->valuestring);
        ESP_ERROR_CHECK(nvs_set_value_str(WIFI_PASS, aux->valuestring));
        if (wifi_connect_sta() == ESP_OK)
        {
            obtain_time();
        }
    }
    

    aux = cJSON_GetObjectItem(root, WATER_LITER);
    if (cJSON_IsNumber(aux))
    {
        float liters = cJSON_GetNumberValue(aux);
        ESP_LOGI(TAG, "Valor en litros %f", liters);
        uint8_t value = (uint8_t)liters;
        ESP_LOGI(TAG, "Valor entero %u", value);
        ESP_ERROR_CHECK(nvs_set_value_num_u8(WATER_LITER, value));
        value = (uint8_t)((liters - value) * 10);
        ESP_LOGI(TAG, "Valor decimal %u", value);
        ESP_ERROR_CHECK(nvs_set_value_num_u8(WATER_DECILITER, value));
    }

    aux = cJSON_GetObjectItem(root, FERTI_1_ML);
    if (cJSON_IsNumber(aux))
    {
        uint8_t additive = aux->valueint;
        ESP_ERROR_CHECK(nvs_set_value_num_u8(FERTI_1_ML, additive));
    }
    aux = cJSON_GetObjectItem(root, FERTI_2_ML);
    if (cJSON_IsNumber(aux))
    {
        uint8_t additive = aux->valueint;
        ESP_ERROR_CHECK(nvs_set_value_num_u8(FERTI_2_ML, additive));
    }
    aux = cJSON_GetObjectItem(root, FERTI_3_ML);
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
esp_err_t get_json_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "temperature", get_parameter_clima(TEMP));
    cJSON_AddNumberToObject(root, "rel_humidity", get_parameter_clima(HUM_REL));
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
httpd_uri_t riego_get = {
    .uri = "/riego",
    .method = HTTP_GET,
    .handler = get_riego_handler,
    .user_ctx = NULL};
httpd_uri_t clima_get = {
    .uri = "/clima",
    .method = HTTP_GET,
    .handler = get_clima_handler,
    .user_ctx = NULL};
httpd_uri_t riego_json_post = {
    .uri = "/riego_json",
    .method = HTTP_POST,
    .handler = post_riego_json_handler,
    .user_ctx = NULL};
httpd_uri_t json_get = {
    .uri = "/info_json",
    .method = HTTP_GET,
    .handler = get_json_handler,
    .user_ctx = NULL};
httpd_uri_t config_json_post = {
    .uri = "/save_config",
    .method = HTTP_POST,
    .handler = post_config_json_handler,
    .user_ctx = NULL};
httpd_uri_t config_json_get = {
    .uri = "/config_json",
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
        httpd_register_uri_handler(server, &riego_get);
        httpd_register_uri_handler(server, &clima_get);
        httpd_register_uri_handler(server, &riego_json_post);
        httpd_register_uri_handler(server, &json_get);
        httpd_register_uri_handler(server, &config_json_post);
        httpd_register_uri_handler(server, &config_json_get);
    }
}

void stop_webserver(void)
{
    if (server)
    {
        httpd_stop(server);
    }
}
