/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "wifi.h"
#include "cultibot.h"
#include "sntp.h"

#include "server.h"
#include "climate_control.h"
#include "menu_LCD.h"

const static char *TAG = "MAIN";

// Codigo para testear cuanta memoria sobra DESCOMENTAR LA SIGUIENTE LINEA
// #define TEST_MEMORIA
#ifdef TEST_MEMORIA

#define NUM 1048
#define STACK_SIZE 8196
static uint8_t ucParameterToPass[NUM];
void vTaskCode(void *pvParameters)
{
	uint8_t ucParametero = *(uint8_t *)pvParameters;
	for (;;)
	{
		vTaskDelay(1 * CONFIG_FREERTOS_HZ);
		ESP_LOGI(TAG, "Tarea %i", ucParametero);
	}
}
void loop()
{
	TaskHandle_t xHandle;
	int i = 1;
	while (1)
	{
		ucParameterToPass[i - 1] = i;
		xHandle = NULL;
		if (xTaskCreate(vTaskCode, "NAME", STACK_SIZE, &ucParameterToPass[i - 1], tskIDLE_PRIORITY, &xHandle) == pdPASS)
		{
		}
		i++;
		vTaskDelay(1 * CONFIG_FREERTOS_HZ);
	}
}
#endif // TEST DE MEMORIA

#include "nvs_flash.h"
void init_drivers()
{
	esp_err_t err;
	ESP_ERROR_CHECK(nvs_manager_init());

	// ESP_ERROR_CHECK(nvs_set_value_str(WIFI_SSID,"JORUSO 7454"));
	// ESP_ERROR_CHECK(nvs_set_value_str(WIFI_PASS,"hola1234"));

	err = wifi_init();

	if (err == ESP_OK)
	{
		obtain_time();
		setenv("TZ", "UTC-1,M3.31.0/2,M10.29.0/3", 1);
	}
	else if (err != ESP_ERR_WIFI_NOT_CONNECT)
	{
		ESP_ERROR_CHECK(err);
	}


	ESP_LOGI(TAG, "Wifi initialized");

	// nvs_manager_deinit();
}

void app_main(void)
{
	init_drivers();
	ESP_LOGI(TAG, "Drivers initialized");
	start_webserver();
	ESP_LOGI(TAG, "Server initialized");
	climate_init();
	ESP_LOGI(TAG, "Climate control initialized");
	init_menu(); 

#ifdef TEST_MEMORIA
	loop();
#endif
	while (1)
	{
		vTaskDelay (10000*portTICK_PERIOD_MS);
	}
}
