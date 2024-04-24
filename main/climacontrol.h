/*
 * climate_control.h
 *
 *  Created on: 29 ene 2024
 *      Author: Joruso
 */

#ifndef MAIN_CLIMATECONTROL_H_
#define MAIN_CLIMATECONTROL_H_

#include "esp_log.h"


void climate_init();
void config_light(uint8_t hour_on, uint8_t min_on,
                  uint8_t hour_off, uint8_t min_off);

#endif /* MAIN_CLIMACONTROL_H_ */
