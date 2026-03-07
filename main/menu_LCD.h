/*
 * menu_LCD.h
 *
 *  Created on: 12 mar 2024
 *      Author: Joruso
 */

#ifndef MENU_LCD_H
#define MENU_LCD_H

#include "driver/gpio.h"

#define PIN_CLK GPIO_NUM_21
#define PIN_DT GPIO_NUM_47
#define PIN_SW GPIO_NUM_48

void init_menu();

#endif 