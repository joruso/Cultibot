#ifndef MAIN_AM2302_H_
#define MAIN_AM2302_H_


#include "esp_err.h"

#define INTERVAL_READ_sec 3	 // Time between each read
#define GPIO_NUM GPIO_NUM_35 // Pin of the board

esp_err_t AM2302_init();

float get_temperature();
float get_rel_humidity();

#endif /* MAIN_AM2302_H_ */
