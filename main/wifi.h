/*
 * wifi.h
 *
 *  Created on: 21 ene 2024
 *      Author: Joruso
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include <esp_err.h>

esp_err_t wifi_init(void);
esp_err_t wifi_connect_sta(void);
esp_err_t wifi_send_UDP_broadcast_info(int *duration);

#endif /* MAIN_WIFI_H_ */
