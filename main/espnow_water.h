#ifndef MAIN_ESPNOW_WATER_H_
#define MAIN_ESPNOW_WATER_H_

#include "esp_err.h"

#define ESPNOW_QUEUE_SIZE 6
#define CONFIG_ESPNOW_CHANNEL 3
#define ESPNOW_WIFI_IF WIFI_IF_AP

esp_err_t espnow_water_init();
void espnow_test();
void calibrate_pulses_water();
//void espnow_irrigate(parameterIrrigation_t parameter_send);

/**
 * @brief Checks if the ESP32 is paired with another ESP device as a slave.
 * 
 * This function returns the current connection status with a slave device through ESP-NOW.
 * 
 * @return uint8_t Returns 0 if not paired with another
 */
uint8_t espnow_is_conected(); 

/**
 * @brief Stops the espnow_task.
 * 
 * This function terminates the espnow_task if it is currently running, freeing its resources.
 */
void stop_espnow_task(void);

/**
 * @brief Restarts the espnow_task.
 * 
 * This function creates and launches the espnow_task if it is not already running.
 */
void start_espnow_task(void);

#endif