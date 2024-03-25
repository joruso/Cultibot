/*
 * AM2302.c
 *
 *  Created on: 12 mar 2024
 *      Author: Joruso
 */


#include "AM2302.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define STACK_SIZE 2048
#define INTERVAL_READ_sec 1

const static char *TAG = "AM2302";

static float temp;
static float hum;

void vTaskSensor( void * pvParameters );

esp_err_t AM2302_init(gpio_num_t pin)
{

	temp=0;
	hum=0;
	TaskHandle_t xHandle ;

	if (xTaskCreate( vTaskSensor, "AM2302", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle)
			== pdPASS){
		return ESP_OK;
	}
	return ESP_FAIL;
}



void vTaskSensor( void * pvParameters )
{
	for( ;; )
	{

		vTaskDelay(INTERVAL_READ_sec*CONFIG_FREERTOS_HZ);
		ESP_LOGI(TAG,"Tarea sensor");
	}
}

float get_temperature(){return temp;}
float get_rel_humidity(){return hum;}
