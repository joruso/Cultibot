#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "cultibot.h"

#define BROKER_URI "mqtt://broker.hivemq.com"
#define TOPIC "test/topic/esp32"

static const char *TAG = "MQTT_CLIENT";
esp_mqtt_client_handle_t client;

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Conectado al broker MQTT");

        msg_id = esp_mqtt_client_publish(client, TOPIC, "MESSAGE", 0, 2, 0);
        ESP_LOGI(TAG, "Mensaje enviado con ID: %d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Desconectado del broker MQTT");
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "Mensaje publicado con ID: %d", event->msg_id);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Error en el cliente MQTT");
        break;

    default:
        ESP_LOGI(TAG, "Evento desconocido: %d", event->event_id);
        break;
    }
}

void init_mqtt(void)
{
    char *aux;
    size_t size;
    nvs_get_value_str(MQTT_URI, NULL, &size);
    aux = (char *)malloc(size * sizeof(char));

    if (aux == NULL)
    {
        ESP_LOGE(TAG, "Malloc fail");
        free(aux);
        return;
    }
    nvs_get_value_str(MQTT_URI, aux, &size);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);

    free(aux);
}

void send_mqtt(char *msg, uint8_t QoS)
{
}

esp_err_t set_mqtt_uri(const char *uri)
{

    esp_err_t err;

    err = esp_mqtt_client_set_uri(client, uri);
    if (err != ESP_OK)
    {
        return err;
    }
    return esp_mqtt_client_reconnect(client);
}