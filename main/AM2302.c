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
#include "esp_err.h"
#include "rom/ets_sys.h"
#include "driver/gpio.h"

// Definition of basics for the module

#define STACK_SIZE 2048
const static char *TAG = "AM2302";

float temperature;
float humidity;

void vTaskSensor(void *pvParameters);

int getSignalLevel(int usTimeOut, bool state)
{

	int uSec = 0;
	while (gpio_get_level(GPIO_NUM) == state)
	{

		if (uSec > usTimeOut)
			return -1;

		++uSec;
		ets_delay_us(1);
	}

	return uSec;
}

#define MAXdhtData 5 // Number of bytes that transmit DHT22

esp_err_t readDHT()
{
	int uSec = 0;

	uint8_t dhtData[MAXdhtData];
	uint8_t byteInx = 0;
	uint8_t bitInx = 7;

	for (int k = 0; k < MAXdhtData; k++)
		dhtData[k] = 0;

	// == Send start signal to DHT sensor ===========

	gpio_set_direction(GPIO_NUM, GPIO_MODE_OUTPUT);

	// pull down for 3 ms for a smooth and nice wake up
	gpio_set_level(GPIO_NUM, 0);
	ets_delay_us(3000);

	// pull up for 25 us for a gentile asking for data
	gpio_set_level(GPIO_NUM, 1);
	ets_delay_us(25);

	gpio_set_direction(GPIO_NUM, GPIO_MODE_INPUT); // change to input mode

	// == DHT will keep the line low for 80 us and then high for 80us ====

	uSec = getSignalLevel(85, 0);
	//	ESP_LOGI( TAG, "Response = %d", uSec );
	if (uSec < 0)
		return ESP_ERR_TIMEOUT;

	// -- 80us up ------------------------

	uSec = getSignalLevel(85, 1);
	//	ESP_LOGI( TAG, "Response = %d", uSec );
	if (uSec < 0)
		return ESP_ERR_TIMEOUT;

	// == No errors, read the 40 data bits ================

	for (int k = 0; k < 40; k++)
	{

		// -- starts new data transmission with >50us low signal

		uSec = getSignalLevel(56, 0);
		if (uSec < 0)
			return ESP_ERR_TIMEOUT;

		// -- check to see if after >70us rx data is a 0 or a 1

		uSec = getSignalLevel(75, 1);
		if (uSec < 0)
			return ESP_ERR_TIMEOUT;

		// add the current read to the output data
		// since all dhtData array where set to 0 at the start,
		// only look for "1" (>28us us)

		if (uSec > 40)
		{
			dhtData[byteInx] |= (1 << bitInx);
		}

		// index to next byte

		if (bitInx == 0)
		{
			bitInx = 7;
			++byteInx;
		}
		else
			bitInx--;
	}

	// == get humidity from Data[0] and Data[1] ==========================

	humidity = dhtData[0];
	humidity *= 0x100; // >> 8
	humidity += dhtData[1];
	humidity /= 10; // get the decimal

	// == get temp from Data[2] and Data[3]

	temperature = dhtData[2] & 0x7F;
	temperature *= 0x100; // >> 8
	temperature += dhtData[3];
	temperature /= 10;

	if (dhtData[2] & 0x80) // negative temp, brrr it's freezing
		temperature *= -1;

	// == verify if checksum is ok ===========================================
	// Checksum is the sum of Data 8 bits masked out 0xFF

	if (dhtData[4] == ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF))
		return ESP_OK;

	return ESP_ERR_INVALID_RESPONSE;
}

esp_err_t AM2302_init()
{

	temperature = 0;
	humidity = 0;
	TaskHandle_t xHandle;

	if (xTaskCreate(vTaskSensor, TAG, STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle) == pdPASS)
	{
		return ESP_OK;
	}
	return ESP_FAIL;
}

void vTaskSensor(void *pvParameters)
{
	esp_err_t err;
	for (;;)
	{
		err = readDHT();
		if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_INVALID_RESPONSE)
		{
			humidity = -1;
			temperature = -1;
		}
		vTaskDelay(INTERVAL_READ_sec * CONFIG_FREERTOS_HZ);
	}
}

float get_temperature()
{
	return temperature;
}
float get_rel_humidity()
{
	return humidity;
}

size_t strfTemp_hum(char *str)
{
	if (humidity == -1)
	{
		sprintf(str, "ERR reading sensor");
	}
	else
	{
		sprintf(str, "T: %.1f C Hr: %.1f", temperature, humidity);
	}

	return sizeof(str);
}