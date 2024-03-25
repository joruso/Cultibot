/*
 * AM2302.h
 *
 *  Created on: 12 mar 2024
 *      Author: Joruso
 */

#ifndef MAIN_AM2302_H_
#define MAIN_AM2302_H_


#include "driver/gpio.h"

esp_err_t AM2302_init(gpio_num_t pin);

float get_temperature();
float get_rel_humidity();

#endif /* MAIN_AM2302_H_ */
