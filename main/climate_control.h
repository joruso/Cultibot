/*
 * climate_control.h
 *
 *  Created on: 29 ene 2024
 *      Author: Joruso
 */

#ifndef MAIN_CLIMATECONTROL_H_
#define MAIN_CLIMATECONTROL_H_

#include "cultibot.h"

#define INTERVAL_refresh_sec 2	   // Time between each time refresh
#define GPIO_NUM_LIGHT GPIO_NUM_36 // Pin of the board


void climate_init();

void load_parameters_from_nvs();

#endif /* MAIN_CLIMACONTROL_H_ */
