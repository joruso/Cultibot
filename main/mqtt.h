#ifndef MQTT_H
#define MQTT_H

#include <esp_err.h>

void init_mqtt(void);

void send_mqtt(char *msg, uint8_t QoS);

esp_err_t set_mqtt_uri(const char *uri);

#endif