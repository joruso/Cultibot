/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "i2c_lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "wifi.h"
#include "sntp.h"

void init_drivers();

void app_main(void)
{
	time_t now;
	struct tm time_info;
	char strtime_buffer [64];


    init_drivers();

    obtain_time();

    setenv("TZ","UTC-1,M3.31.0/2,M10.29.0/3",1);

    while (1) {
    	time(&now);
    	localtime_r(&now,&time_info);
    	strftime(strtime_buffer, sizeof(strtime_buffer),"%F", &time_info);
    	LCD_home();
    	LCD_writeStr(strtime_buffer);
    	LCD_writeStr(" ");
    	strftime(strtime_buffer, sizeof(strtime_buffer),"%T", &time_info);
    	LCD_writeStr(strtime_buffer);

    	vTaskDelay(0.9*CONFIG_FREERTOS_HZ);


    }
}

void init_drivers(){
	wifi_init_sta();
	LCD_init ();
	LCD_clearScreen();
}
