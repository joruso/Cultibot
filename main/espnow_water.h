#ifndef MAIN_ESPNOW_WATER_H_
#define MAIN_ESPNOW_WATER_H_

#include "esp_err.h"

#define ESPNOW_QUEUE_SIZE 6
#define CONFIG_ESPNOW_CHANNEL 3
#define ESPNOW_WIFI_IF WIFI_IF_AP

esp_err_t espnow_water_init();
void espnow_test();
void calibrate_pulses_water();
uint8_t espnow_is_conected(); //return 0 if not paired with other esp slave else return !=0
//void espnow_irrigate(parameterIrrigation_t parameter_send);

#endif