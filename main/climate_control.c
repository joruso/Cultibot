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
#include "espnow_water.h"
#include "nvs_manager.h"

#define GPIO_OUTPUT_PINES (1ULL<<GPIO_NUM_LIGHT)

#define STACK_SIZE 2048
const static char *TAG = "CLIMATE_CTR";

void vTaskControl(void *pvParameters);

static uint8_t h_on, m_on, h_off, m_off;

void load_parameters_from_nvs(){
	
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_MIN, &m_on));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_MIN, &m_off));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_HOUR, &h_on));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_HOUR, &h_off));

	//hay que cargar todas las memorias y ver como guardo el proximo riego --> wday


}

void climate_init()
{
	// Inizializamos la conexcion con los sensores y con el otro ESP
	AM2302_init();
	espnow_water_init();

	load_parameters_from_nvs();

	gpio_config_t conf_gpio;
	conf_gpio.mode=GPIO_MODE_OUTPUT;
	conf_gpio.pin_bit_mask=GPIO_OUTPUT_PINES;
	conf_gpio.intr_type=GPIO_INTR_DISABLE;

	ESP_ERROR_CHECK(gpio_config(&conf_gpio));

	TaskHandle_t xHandle;
	xTaskCreate(vTaskControl, TAG, STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle);
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
		if (time_info.tm_hour >= h_on && time_info.tm_hour <= h_off 
			&& time_info.tm_min >= m_on && time_info.tm_min <= m_off)
		{
			on_light();
		}
		else
		{
			off_light();
		}
		vTaskDelay(INTERVAL_REFRESH_SEC * CONFIG_FREERTOS_HZ);
	}
}