/*
 * climate_control.c
 *
 *  Created on: 29 ene 2024
 *      Author: Joruso
 */

#include "climate_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "AM2302.h"
#include <time.h>
#include "driver/gpio.h"

// Definition of basics for the module
#define INTERVAL_refresh_sec 2	   // Time between each time refresh
#define GPIO_NUM_LIGHT GPIO_NUM_36 // Pin of the board

#define GPIO_OUTPUT_PINES (1ULL<<GPIO_NUM_LIGHT)
#define STACK_SIZE 2048
const static char *TAG = "CLIMATE_CTR";

void vTaskControl(void *pvParameters);

uint8_t h_on, m_on, h_off, m_off;

void climate_init()
{
	AM2302_init();

	h_on = 9;
	h_off = 21;
	m_on = 0;
	m_off = 0;

	gpio_config_t conf_gpio;
	conf_gpio.mode=GPIO_MODE_OUTPUT;
	conf_gpio.pin_bit_mask=GPIO_OUTPUT_PINES;
	conf_gpio.intr_type=GPIO_INTR_DISABLE;

	ESP_ERROR_CHECK(gpio_config(&conf_gpio));

	TaskHandle_t xHandle;
	xTaskCreate(vTaskControl, TAG, STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle);
}

void config_light(uint8_t hour_on, uint8_t min_on,
				  uint8_t hour_off, uint8_t min_off)
{
	// se configuran las horas de encendido de la iluminacion
}

void off_light()
{
	ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_LIGHT,1));
}

void on_light()
{
	ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_LIGHT,0));
}

void vTaskControl(void *pvParameters)
{
	time_t now;
	struct tm time_info;

	for (;;)
	{
		time(&now);
		localtime_r(&now, &time_info);
		//ESP_LOGI(TAG,"HOUR->%u",time_info.tm_hour);
		if (time_info.tm_hour >= h_on && time_info.tm_hour <= h_off)
		{
			on_light();
		}
		else
		{
			off_light();
		}
		vTaskDelay(INTERVAL_refresh_sec * CONFIG_FREERTOS_HZ);
	}
}
