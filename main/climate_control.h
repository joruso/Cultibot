/*
 * climate_control.h
 *
 *  Created on: 29 ene 2024
 *      Author: Joruso
 */

#ifndef MAIN_CLIMATECONTROL_H_
#define MAIN_CLIMATECONTROL_H_

#include "cultibot.h"

#define INTERVAL_REFRESH_SEC 2	                // Time between each time refresh
#define GPIO_NUM_LIGHT GPIO_NUM_36              // Pin of the board
#define GPIO_NUM_EXTRACTOR GPIO_NUM_36          // Pin of the board
#define GPIO_NUM_VENTI_INTERNO GPIO_NUM_36      // Pin of the board
#define GPIO_NUM_HUMIFIER GPIO_NUM_36           // Pin of the board
//#define GPIO_NUM_VENTI_INTERNO2 GPIO_NUM_36   // Pin of the board




void climate_init();


#endif /* MAIN_CLIMACONTROL_H_ */
