#include "espnow_water.h"
#include "esp_log.h"
#include <string.h>
#include "protocolWater.h"
#include "freertos/FreeRTOS.h"

#include <stdbool.h>

static const char *TAG = "ESP-NOW";

static QueueHandle_t espnow_queue = NULL;
static uint8_t paired = 0;
static TaskHandle_t espnowTaskHandle = NULL;

#define BROADCAST_DIR \
    (uint8_t[ESP_NOW_ETH_ALEN]) { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

static void espnow_task(void *pvParameter);
static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{

    packet_water_t data_recv;
    memcpy(&data_recv.data, data, sizeof(data_water_t));
    memcpy(&data_recv.mac_addr, esp_now_info->src_addr, ESP_NOW_ETH_ALEN);
    // ESP_LOGI(TAG, "recv from:" MACSTR "", MAC2STR(esp_now_info->src_addr));
    // ESP_LOGI(TAG, "recv to:" MACSTR "", MAC2STR(esp_now_info->des_addr));
    xQueueSend(espnow_queue, &data_recv, 0);
}
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{

    // ESP_LOGI(TAG, "send to:" MACSTR "", MAC2STR(mac_addr));
    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        // ESP_LOGI(TAG, "Send completed");
    }
    else
    {
        ESP_LOGE(TAG, "Send error");
    }
}
void espnow_register_slave(packet_water_t *packet_water_ptr);
void espnow_send_comand(packet_water_t *packet_water_ptr, tipoComando comando)
{
    packet_water_ptr->data.comando = comando;
    esp_err_t err;
    err = esp_now_send((uint8_t *)&packet_water_ptr->mac_addr,
                       (uint8_t *)&packet_water_ptr->data,
                       sizeof(packet_water_ptr->data));

    ESP_ERROR_CHECK(err);
}

esp_err_t espnow_water_init()
{

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(packet_water_t));

    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(espnow_queue);
        esp_now_deinit();
        vTaskDelete(NULL);
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 0;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, BROADCAST_DIR, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    xTaskCreate(espnow_task, "espnow_task", 4096, NULL, 4, &espnowTaskHandle);

    return ESP_OK;
}


static void espnow_task(void *pvParameter)
{
    packet_water_t packet_water;

    while (1)
    {
        espnow_register_slave(&packet_water);

        while (xQueueReceive(espnow_queue, &packet_water, 20 * configTICK_RATE_HZ) == pdTRUE)
        {
            switch (packet_water.data.comando)
            {
            case COMANDO_SET_IRRIGATION:
                // se configura el riego
                break;
            case COMANDO_TEST_10_SEG:
                // hace un test de 15 seg para ver cuantos pulsos se han producido
                break;
            case COMANDO_ALIVE:
                espnow_send_comand(&packet_water, COMANDO_ACK);
                break;

            default:
                break;
            }
        }
        ESP_LOGI(TAG, "TIMEOUT");
        paired = 0;
    }
}

uint8_t espnow_is_conected()
{
    return paired;
}

void espnow_test();
void calibrate_pulses_water();
void espnow_register_slave(packet_water_t *packet_water_ptr)
{

    esp_now_peer_info_t peer;
    peer.channel = 0;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;

    while (paired == 0)
    {

        memcpy(&packet_water_ptr->mac_addr, BROADCAST_DIR, ESP_NOW_ETH_ALEN);
        espnow_send_comand(packet_water_ptr, COMANDO_PAIR);

        // ESP_LOGI(TAG, "sent with:" MACSTR "", MAC2STR(packet_water.mac_addr));
        xQueueReceive(espnow_queue, packet_water_ptr, 150 / portTICK_PERIOD_MS);
        if (packet_water_ptr->data.comando == COMANDO_ACK)
        {
            if (!esp_now_is_peer_exist((uint8_t *)&packet_water_ptr->mac_addr))
            {
                memcpy(peer.peer_addr, &packet_water_ptr->mac_addr, ESP_NOW_ETH_ALEN);
                ESP_ERROR_CHECK(esp_now_add_peer(&peer));
                ESP_LOGI(TAG, "Register ESP-NOW with MAC: " MACSTR "", MAC2STR(packet_water_ptr->mac_addr));
            }
            else
            {
                ESP_LOGI(TAG, "Conected again");
            }
            paired = 1;
        }
    }
}

void stop_espnow_task(void)
{
    if (espnowTaskHandle != NULL)
    {
        vTaskDelete(espnowTaskHandle);
        espnowTaskHandle = NULL;
        ESP_LOGI(TAG, "Task stopped");
    }
}

void start_espnow_task()
{
    if (espnowTaskHandle == NULL && espnow_queue != NULL)
    {
        xTaskCreate(espnow_task, "espnow_task", 4096, NULL, 4, &espnowTaskHandle);
        ESP_LOGI(TAG, "Task restarted");
    }
}