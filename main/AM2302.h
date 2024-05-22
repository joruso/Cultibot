/*
 * AM2302.h
 *
 *  Created on: 12 mar 2024
 *      Author: Joruso
 */

#ifndef MAIN_AM2302_H_
#define MAIN_AM2302_H_


#include "esp_err.h"

#define INTERVAL_READ_sec 3	 // Time between each read
#define GPIO_NUM GPIO_NUM_35 // Pin of the board

esp_err_t AM2302_init();

float get_temperature();
float get_rel_humidity();
size_t strfTemp_hum(char * str); // returns in str -> Temp: %t Hum: %h

#endif /* MAIN_AM2302_H_ */
