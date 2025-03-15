#ifndef MAIN_ESPNOW_WATER_H_
#define MAIN_ESPNOW_WATER_H_

#include "esp_err.h"
#include "protocolWater.h"

esp_err_t espnow_water_init();


/**
 * @brief Calibrate the device.
 * 
 * This function send to espnow queue a set comand for calibrate water parameters.
 * 
 * @return esp_err_t Returns ESP_ERR_ESPNOW_NOT_FOUND if not paired, ESP_ERR_NO_MEM if queue is full or ESP_OK
 */
esp_err_t espnow_calibrate(parameterIrrigation_t parameter_send);

/**
 * @brief Send test during sec.
 * 
 * This function send to espnow queue a test comand for turning ON the peripherals during sec.
 * 
 * @return esp_err_t Returns ESP_ERR_ESPNOW_NOT_FOUND if not paired, ESP_ERR_NO_MEM if queue is full or ESP_OK
 */
esp_err_t espnow_test(uint8_t sec);

/**
 * @brief Send comand for irrigate.
 * 
 * This function send to espnow queue a irrigation comand.
 * 
 * @return esp_err_t Returns ESP_ERR_ESPNOW_NOT_FOUND if not paired, ESP_ERR_NO_MEM if queue is full or ESP_OK
 */
esp_err_t espnow_irrigate(parameterIrrigation_t parameter_send);

/**
 * @brief Checks if the ESP32 is paired with another ESP device as a slave.
 * 
 * This function returns the current connection status with a slave device through ESP-NOW.
 * 
 * @return uint8_t Returns 0 if not paired with another
 */
uint8_t espnow_is_conected(void); 

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