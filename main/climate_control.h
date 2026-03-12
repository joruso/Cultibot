/*
 * climate_control.h
 *
 *  Created on: 29 ene 2024
 *      Author: Joruso
 */

#ifndef MAIN_CLIMATECONTROL_H_
#define MAIN_CLIMATECONTROL_H_

#include "cultibot.h"

#define INTERVAL_REFRESH_SEC 10 // Time between each time refresh
#define INTERVAL_RETRY_IRRIGATION_HOUR 3
#define INTERVAL_RETRY_IRRIGATION_SEND_MIN 5

#define GPIO_NUM_LIGHT GPIO_NUM_36         // Pin of the board
#define GPIO_NUM_EXTRACTOR GPIO_NUM_39     // Pin of the board
#define GPIO_NUM_VENTI_INTERNO_1 GPIO_NUM_40 // Pin of the board
#define GPIO_NUM_VENTI_INTERNO_2 GPIO_NUM_37   // Pin of the board
// #define GPIO_HEATER_HUMIFIER GPIO_NUM_36           // Pin of the board


void climate_init(void);

void irrigation_succed(void);       // Call this function when recv COMANDO_IRRIGATE_OK

void irrigation_in_progress(void);   // Call this function when recv COMANDO_IRRIGATING

void irrigate_now(void);
#endif /* MAIN_CLIMACONTROL_H_ */
