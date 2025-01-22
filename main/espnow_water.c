#include "espnow_water.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include "esp_crc.h"

#define QUEUE_SIZE 6

static const char *TAG = "ESP-NOW";
static QueueHandle_t espnow_queue = NULL;
static uint8_t paired = 0;
static uint16_t seq_packet_count;
static TaskHandle_t espnowTaskHandle = NULL;
static uint8_t mac_peer[6];

static void espnow_register_slave(packet_water_t *packet);
static void espnow_task(void *pvParameter);
static esp_err_t espnow_send_with_acknowledge(packet_water_t *packet, tipoComando comando);
static esp_err_t espnow_send_ACK(packet_water_t *packet);

static void espnow_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{

    packet_water_t data_recv;
    memcpy(&data_recv.data, data, sizeof(data_water_t));
    memcpy(&data_recv.mac_addr, esp_now_info->src_addr, ESP_NOW_ETH_ALEN);
    // ESP_LOGI(TAG, "recv from:" MACSTR "", MAC2STR(esp_now_info->src_addr));
    // ESP_LOGI(TAG, "recv to:" MACSTR "", MAC2STR(esp_now_info->des_addr));
    // ESP_LOGI(TAG, "Comand recv %u with id %u", data_recv.data.comando,data_recv.data.seq_num);
    if (xQueueSendFromISR(espnow_queue, &data_recv, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue full, dropping packet");
    }
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{

    ESP_LOGI(TAG, "send to:" MACSTR "", MAC2STR(mac_addr));
    if (mac_addr == NULL)
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "Send completed");
    }
    else
    {
        ESP_LOGE(TAG, "Send error");
    }
}

static void espnow_task(void *pvParameter)
{
    packet_water_t packet_water;

    while (1)
    {
        espnow_register_slave(&packet_water);

        while (paired && xQueueReceive(espnow_queue, &packet_water, ESPNOW_TIMEOUT_S * configTICK_RATE_HZ) == pdTRUE)
        {
            switch (packet_water.data.comando)
            {
            case COMANDO_SET_IRRIGATION:

                ESP_ERROR_CHECK(espnow_send_with_acknowledge(&packet_water, COMANDO_SET_IRRIGATION));
                break;
            case COMANDO_TEST_10_SEG:
                ESP_ERROR_CHECK(espnow_send_with_acknowledge(&packet_water, COMANDO_TEST_10_SEG));
                break;

            case COMANDO_IRRIGATE:

                ESP_ERROR_CHECK(espnow_send_with_acknowledge(&packet_water, COMANDO_IRRIGATE));

                break;

            case COMANDO_ALIVE:
                if (esp_now_is_peer_exist(packet_water.mac_addr))
                {
                    ESP_ERROR_CHECK(espnow_send_ACK(&packet_water));
                }

                break;
            case COMANDO_IRRIGATE_FAIL:
                espnow_send_ACK(&packet_water);
                break;

            default:
                break;
            }
        }
        ESP_LOGI(TAG, "TIMEOUT");
        paired = 0;
    }
}

static esp_err_t espnow_send_ACK(packet_water_t *packet)
{
    esp_err_t err;
    uint16_t crc, crc_cal;
    crc = packet->data.crc;
    packet->data.crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(packet->data), sizeof(data_water_t));

    if (crc == crc_cal)
    {
        packet->data.comando = COMANDO_ACK;
        if (seq_packet_count == packet->data.seq_num)
        {
            for (int i = 0; i < ESPNOW_MAX_RETRIES; i++)
            {
                err = esp_now_send((uint8_t *)packet->mac_addr, (uint8_t *)&packet->data, sizeof(packet->data));
                if (err != ESP_OK)
                {
                    ESP_LOGW(TAG, "Retry %d/%d for command ACK failed with err: %s", i + 1, ESPNOW_MAX_RETRIES, esp_err_to_name(err));
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                else
                {
                    ESP_LOGI(TAG, "Send ACK id:%u complete", packet->data.seq_num);
                    seq_packet_count++;
                    return ESP_OK;
                }
            }
        }
        else if (seq_packet_count - 1 == packet->data.seq_num)
        {
            ESP_LOGW(TAG, "Received secuence number equal to ours number-1");
            packet->data.seq_num = seq_packet_count;
            for (int i = 0; i < ESPNOW_MAX_RETRIES; i++)
            {
                err = esp_now_send((uint8_t *)packet->mac_addr, (uint8_t *)&packet->data, sizeof(packet->data));
                if (err != ESP_OK)
                {
                    ESP_LOGW(TAG, "Retry %d/%d for command ACK failed with err: %s", i + 1, ESPNOW_MAX_RETRIES, esp_err_to_name(err));
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                else
                {
                    ESP_LOGI(TAG, "Send ACK id:%u complete", packet->data.seq_num);
                    seq_packet_count++;
                    return ESP_OK;
                }
            }
        }
        else
        {
            paired = 0;
            return ESP_ERR_ESPNOW_ARG;
        }
    }
    else
    { // crc =! crc_cal
        ESP_LOGE(TAG, "CHECKSUM ERROR");
        err = ESP_ERR_ESPNOW_ARG;
    }

    return err;
}

static esp_err_t espnow_send_with_acknowledge(packet_water_t *packet, tipoComando comando)
{

    esp_err_t err = ESP_FAIL;
    for (int j = 0; j < ESPNOW_MAX_RETRIES; j++)
    {
        err = ESP_FAIL;
        packet->data.seq_num = seq_packet_count;
        packet->data.crc = 0;
        packet->data.comando = comando;
        packet->data.crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)&(packet->data), sizeof(data_water_t));

        for (int i = 0; i < ESPNOW_MAX_RETRIES && err == ESP_FAIL; i++)
        {
            err = esp_now_send((uint8_t *)packet->mac_addr, (uint8_t *)&packet->data, sizeof(packet->data));
            if (err != ESP_OK)
            {
                ESP_LOGW(TAG, "Retry %d/%d for command %d failed, ERR: %s", i + 1, ESPNOW_MAX_RETRIES, comando, esp_err_to_name(err));
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        ESP_LOGI(TAG, "Comand recv %u with id %u must to be %u err: %s", packet->data.comando, packet->data.seq_num, seq_packet_count, esp_err_to_name(err));

        if (xQueueReceive(espnow_queue, &packet, ESPNOW_RETRYTIMEOUT_S * configTICK_RATE_HZ) == pdTRUE)
        {
            if (packet->data.comando == COMANDO_ACK && packet->data.seq_num == seq_packet_count)
            {
                ESP_LOGI(TAG, "Send comand complete");
                seq_packet_count++;
                return ESP_OK;
            }
            else if (packet->data.seq_num != seq_packet_count)
            {
                ESP_LOGE(TAG, "SEQ NUM ERROR, received %u, must to be %u", packet->data.seq_num, seq_packet_count);
                paired = 0;
                return ESP_ERR_ESPNOW_ARG;
            }
            else if (packet->data.comando != COMANDO_ACK)
            {
                ESP_LOGE(TAG, "COMAND ERROR, received %u, must to be %u", packet->data.comando, COMANDO_ACK);
                paired = 0;
                return ESP_ERR_ESPNOW_ARG;
            }
            else
            {
                ESP_LOGE(TAG, "PEER NOT EXIST, received from " MACSTR "", MAC2STR(packet->mac_addr));
            }
        }
        else
        {
            ESP_LOGE(TAG, "Not recv ACK err: %s", esp_err_to_name(err));
            err = ESP_ERR_TIMEOUT;
        }
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send command %d after %d retries", comando, ESPNOW_MAX_RETRIES);
    }

    return err;
}

esp_err_t espnow_water_init()
{

    seq_packet_count = 0;
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    espnow_queue = xQueueCreate(QUEUE_SIZE, sizeof(packet_water_t));

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
    peer->ifidx = WIFI_IF_AP;
    peer->encrypt = false;
    memcpy(peer->peer_addr, BROADCAST_DIR, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    xTaskCreate(espnow_task, "espnow_task", 4096, NULL, 4, &espnowTaskHandle);

    return ESP_OK;
}

void espnow_register_slave(packet_water_t *packet)
{
    esp_err_t err;
    esp_now_peer_info_t peer;
    peer.channel = 0;
    peer.ifidx = WIFI_IF_AP;
    peer.encrypt = false;

    packet->data.comando = COMANDO_PAIR;
    packet->data.seq_num = seq_packet_count;

    while (paired == 0)
    {
        packet->data.comando = COMANDO_PAIR;
        memcpy(&packet->mac_addr, BROADCAST_DIR, ESP_NOW_ETH_ALEN);
        esp_now_send((uint8_t *)packet->mac_addr, (uint8_t *)&packet->data, sizeof(packet->data));
        ESP_LOGI(TAG, "send comand PAIR");
        if (xQueueReceive(espnow_queue, packet, 400 / portTICK_PERIOD_MS) == pdTRUE)
        {
            // ESP_LOGI(TAG, "RECV PAQ");
            if (packet->data.comando == COMANDO_ACK)
            {
                ESP_LOGI(TAG, "ESP-NOW with MAC: " MACSTR "", MAC2STR(packet->mac_addr));
                if (!esp_now_is_peer_exist((uint8_t *)&packet->mac_addr))
                {
                    memcpy(peer.peer_addr, &packet->mac_addr, ESP_NOW_ETH_ALEN);
                    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
                    ESP_LOGI(TAG, "Register with MAC: " MACSTR "", MAC2STR(packet->mac_addr));
                    memcpy(&mac_peer[0], &packet->mac_addr, ESP_NOW_ETH_ALEN);
                }
                else
                {
                    ESP_LOGI(TAG, "Conected again");
                }
                paired = 1;
            }
        }
    }
}

void espnow_irrigate(parameterIrrigation_t parameter_send)
{

    packet_water_t packet;
    packet.data.comando = COMANDO_IRRIGATE;
    packet.data.paramIrrigation = parameter_send;
    memcpy(&packet.mac_addr, &mac_peer[0], ESP_NOW_ETH_ALEN);

    if (xQueueSend(espnow_queue, &packet, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue full, dropping packet irrigate");
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

uint8_t espnow_is_conected()
{
    return paired;
}

void espnow_test()
{

    packet_water_t packet;
    packet.data.comando = COMANDO_TEST_10_SEG;
    memcpy(&packet.mac_addr, &mac_peer[0], ESP_NOW_ETH_ALEN);
    if (xQueueSend(espnow_queue, &packet, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue full, dropping packet irrigate");
    }
    else
    {
        ESP_LOGI(TAG, "send COMAND TEST to:" MACSTR " ", MAC2STR(packet.mac_addr));
    }
}
