/*
 * wifi.c
 *
 *  Created on: 21 ene 2024
 *      Author: Joruso
 */

#include "wifi.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "nvs_manager.h"
#include "cultibot.h"
#include "espnow_water.h"

#include <string.h>

#include "esp_mac.h"

#define WIFI_MAXIMUM_RETRY 8

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define MAX_STA_CONN 1 // Maximo de dispositivos conectados en modo station

// Grupo de flag de eventos para manejar los eventos del Driver
static EventGroupHandle_t s_wifi_event_group;
esp_netif_t * netif_ptr = NULL;

static const char *TAG = "wifi station";
static int s_retry_num = 0;
static esp_ip4_addr_t ip_esp;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ip_esp = event->ip_info.ip;

        uint8_t primary;
        wifi_second_chan_t secondary;
        esp_wifi_get_channel(&primary, &secondary);
        ESP_LOGI(TAG, "got channel number: %i", primary);

        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

esp_err_t wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    return wifi_connect_sta();
}

esp_err_t wifi_connect_sta(void)
{
    stop_espnow_task();
    if (netif_ptr != NULL){
        ESP_ERROR_CHECK(esp_wifi_stop());
        esp_netif_destroy_default_wifi(netif_ptr);
        netif_ptr = NULL;
    }

    netif_ptr = esp_netif_create_default_wifi_sta();
    esp_err_t err;


    char *wifi_ssid, *wifi_pass;
    size_t ssid_tam, pass_tam;

    ESP_ERROR_CHECK(nvs_get_value_str(WIFI_SSID, NULL, &ssid_tam));
    ESP_ERROR_CHECK(nvs_get_value_str(WIFI_PASS, NULL, &pass_tam));

    wifi_ssid = (char *)malloc(ssid_tam * sizeof(char));
    wifi_pass = (char *)malloc(pass_tam * sizeof(char));
    if (wifi_ssid == NULL || wifi_pass == NULL)
    {
        ESP_LOGE(TAG, "Error en la asignación de memoria");
        free(wifi_ssid);
        free(wifi_pass);
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(nvs_get_value_str(WIFI_SSID, wifi_ssid, &ssid_tam));
    ESP_ERROR_CHECK(nvs_get_value_str(WIFI_PASS, wifi_pass, &pass_tam));

    
    wifi_config_t wifi_config_apsta = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .failure_retry_cnt = 1,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
        .ap.ssid_hidden = 1,        
    };
    s_retry_num = 0;
    //ESP_LOGI(TAG, "%u", s_retry_num);
    memcpy(wifi_config_apsta.sta.ssid, wifi_ssid, ssid_tam);
    memcpy(wifi_config_apsta.sta.password, wifi_pass, pass_tam);

    //ESP_LOGI(TAG, "%s", wifi_config_sta.sta.password);

    xEventGroupClearBits(s_wifi_event_group,WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_apsta));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_apsta));
    ESP_ERROR_CHECK(esp_wifi_start());
    //ESP_LOGI(TAG, "%u", s_retry_num);

    //ESP_LOGI(TAG, "%s", wifi_ssid);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           portMAX_DELAY);

    free(wifi_ssid);
    free(wifi_pass);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_ssid, wifi_pass);
        err = ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s, starting acces point",
                 wifi_ssid, wifi_pass);
        
        ESP_ERROR_CHECK(esp_wifi_stop());
        
        if (netif_ptr != NULL){
            esp_netif_destroy_default_wifi(netif_ptr);
            netif_ptr = NULL;
        };
        netif_ptr = esp_netif_create_default_wifi_ap();

        wifi_config_t wifi_config_apsta = {
            .ap = {
                .ssid = ESP_WIFI_SSID,
                .ssid_len = strlen(ESP_WIFI_SSID),
                //.channel = 1,
                .ssid_hidden = 0,
                .password = ESP_WIFI_PASS,
                .max_connection = MAX_STA_CONN,
                .authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {
                    .required = true,
                },
            },
        };

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_apsta));
        ESP_ERROR_CHECK(esp_wifi_start());
        err = ESP_ERR_WIFI_NOT_CONNECT;
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        err = ESP_ERR_WIFI_NOT_INIT;
    }

    start_espnow_task();
    return err;
}
