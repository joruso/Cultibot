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
#include "esp_log.h"

#define GPIO_OUTPUT_PINES ((1ULL << GPIO_NUM_LIGHT) | (1ULL << GPIO_NUM_EXTRACTOR) | (1ULL << GPIO_NUM_VENTI_INTERNO))

#define STACK_SIZE 4096
const static char *TAG = "CLIMATE_CTR";

static void vTaskControl(void *pvParameters);
void reset_measures(void);

static TaskHandle_t xHandle;
static uint8_t temp_day, temp_night, histeresis_day, histeresis_night, hum_day, hum_night; // Variables temperatura y humedad
static uint8_t interior_vent_status, interior_vent_min_on, interior_vent_min_off;		   // Variables ventilador interno
static uint8_t hour_btw_irrigations;													   // Variables riego tiempo
static time_t next_irrigation;															   // Variables riego tiempo
static uint8_t liter_irrigation, dLiter_irrigation, aditive1, aditive2, aditive3;		   // Variables cantidad riego
static uint16_t start_Light, end_Light;

static struct tm time_info;

static float min_temp_day, min_temp_night,
	max_temp_day, max_temp_night,
	avg_temp_day, avg_temp_night;
static uint16_t count_temp_day, count_temp_night;

void climate_init()
{
	// Inizializamos la conexcion con los sensores y con el otro ESP
	AM2302_init();
	espnow_water_init();
	gpio_config_t conf_gpio;
	conf_gpio.mode = GPIO_MODE_OUTPUT;
	conf_gpio.pin_bit_mask = GPIO_OUTPUT_PINES;
	conf_gpio.intr_type = GPIO_INTR_DISABLE;

	ESP_ERROR_CHECK(gpio_config(&conf_gpio));

	xTaskCreate(vTaskControl, TAG, STACK_SIZE, NULL, tskIDLE_PRIORITY, &xHandle);
}

void climate_load_parameters_from_nvs(void)
{
	uint8_t h_on, m_on, h_off, m_off, last_month_irri, last_day_irri, last_hour_irri, last_min_irri;
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_MIN, &m_on));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_MIN, &m_off));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_ON_HOUR, &h_on));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LIGHT_OFF_HOUR, &h_off));

	start_Light = h_on * 60 + m_on;
	end_Light = h_off * 60 + m_off;

	ESP_ERROR_CHECK(nvs_get_value_num_u8(TEMP_DAY, &temp_day));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(TEMP_NIGHT, &temp_night));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(HISTERESIS_DAY, &histeresis_day));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(HISTERESIS_NIGHT, &histeresis_night));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(HUMEDAD_REL_DAY, &hum_day));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(HUMEDAD_REL_NIGHT, &hum_night));

	ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_VENT_STATUS, &interior_vent_status));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_VENT_M_ON, &interior_vent_min_on));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_VENT_M_OFF, &interior_vent_min_off));

	ESP_ERROR_CHECK(nvs_get_value_num_u8(HOURS_BETWEEN_IRRIGATIONS, &hour_btw_irrigations));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_MONTH, &last_month_irri));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_DAY, &last_day_irri));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_HOUR, &last_hour_irri));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(LAST_IRRI_MIN, &last_min_irri));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(WATER_LITER, &liter_irrigation));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(WATER_DECILITER, &dLiter_irrigation));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_1_ML, &aditive1));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_2_ML, &aditive2));
	ESP_ERROR_CHECK(nvs_get_value_num_u8(FERTI_3_ML, &aditive3));

	struct tm time_next_date;
	time_t now;
	time(&now);
	localtime_r(&now, &time_next_date);
	time_next_date.tm_mon = last_month_irri;
	time_next_date.tm_mday = last_day_irri;
	time_next_date.tm_hour = last_hour_irri;
	time_next_date.tm_min = last_min_irri;
	time_next_date.tm_hour += hour_btw_irrigations;
	next_irrigation = mktime(&time_next_date);

	xTaskAbortDelay(xHandle);
}

static void prove_parameters_day()
{
	float temp_read = get_temperature();

	ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_LIGHT, 1));
	if ((float)(temp_day + histeresis_day) < temp_read || (float)hum_day < get_rel_humidity())
	{
		gpio_set_level(GPIO_NUM_EXTRACTOR, 1);
	}
	else if ((float)(temp_day - histeresis_day) > temp_read)
	{
		gpio_set_level(GPIO_NUM_EXTRACTOR, 0);
	}

	avg_temp_day += temp_read;
	count_temp_day++;
	if (temp_read > max_temp_day)
	{
		max_temp_day = temp_read;
	}
	else if (temp_read < min_temp_day)
	{
		min_temp_day = temp_read;
	}
}

static void prove_parameters_night()
{
	float temp_read = get_temperature();
	ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_LIGHT, 0));
	if ((float)(temp_night + histeresis_night) < temp_read || (float)hum_night < get_rel_humidity())
	{
		// ESP_LOGI(TAG, "vent ON");
		gpio_set_level(GPIO_NUM_EXTRACTOR, 1);
	}
	else if ((float)(temp_night - histeresis_night) > temp_read)
	{
		// ESP_LOGI(TAG, "vent OFF");
		gpio_set_level(GPIO_NUM_EXTRACTOR, 0);
	}

	avg_temp_night += temp_read;
	count_temp_night++;
	if (temp_read > max_temp_night)
	{
		max_temp_night = temp_read;
	}
	else if (temp_read < min_temp_night)
	{
		min_temp_night = temp_read;
	}
}

static void prove_light(uint16_t *current_time)
{
	if (start_Light > end_Light)
	{
		if (*current_time >= start_Light || *current_time <= end_Light)
		{
			// ESP_LOGI(TAG, "Light ON");
			prove_parameters_day();
		}
		else
		{
			// ESP_LOGI(TAG, "Light OFF");
			prove_parameters_night();
		}
	}
	else
	{
		if (*current_time >= start_Light && *current_time <= end_Light)
		{
			// ESP_LOGI(TAG, "Light ON");
			prove_parameters_day();
		}
		else
		{
			// ESP_LOGI(TAG, "Light OFF");
			prove_parameters_night();
		}
	}
}

static void prove_vent_indoor(time_t *current_time)
{
	static uint16_t next_interval_ventinterior = 0;
	if (interior_vent_status >= 2)
	{
		// ESP_LOGI(TAG, "%d, %d", *current_time, next_interval_ventinterior);
		if (*current_time >= next_interval_ventinterior)
		{
			if (interior_vent_status == 2)
			{
				next_interval_ventinterior = *current_time + interior_vent_min_on;
				gpio_set_level(GPIO_NUM_VENTI_INTERNO, 1);
				//ESP_LOGI(TAG, "Indoor fan ON");
				interior_vent_status = 3;
			}
			else
			{
				next_interval_ventinterior = *current_time + interior_vent_min_off;
				gpio_set_level(GPIO_NUM_VENTI_INTERNO, 0);
				//ESP_LOGI(TAG, "Indoor fan OFF");
				interior_vent_status = 2;
			}
		}
	}
	else if (interior_vent_status == 1)
	{
		gpio_set_level(GPIO_NUM_VENTI_INTERNO, 1);
		//ESP_LOGI(TAG, "Indoor fan permanently ON");
	}
	else
	{
		gpio_set_level(GPIO_NUM_VENTI_INTERNO, 0);
		//ESP_LOGI(TAG, "Indoor fan permanently OFF");
	}
}

static void vTaskControl(void *pvParameters)
{
	climate_load_parameters_from_nvs();
	time_t now;


	uint8_t day_now = time_info.tm_mday;

	reset_measures();

	for (;;)
	{
		time(&now);
		localtime_r(&now, &time_info);
		// ESP_LOGI(TAG,"HOUR->%u",time_info.tm_hour);
		//ESP_LOGI(TAG, "%u:%u", time_info.tm_hour, time_info.tm_min);
		//ESP_LOGI(TAG, "%u:%u", start_Light, end_Light);

		uint16_t current_time = time_info.tm_hour * 60 + time_info.tm_min;

		prove_vent_indoor(&now);
		prove_light(&current_time);

		if (now > next_irrigation)
		{
			irrigate_now();
		}

		if (time_info.tm_mday != day_now)
		{
			// Ha pasado al dia siguiente comparto los datos de la temperatura

			reset_measures();
			day_now = time_info.tm_mday;
		}

		vTaskDelay(INTERVAL_REFRESH_SEC * CONFIG_FREERTOS_HZ);
	}
}

void reset_measures(void)
{

	max_temp_day = 0;
	max_temp_night = 0;
	min_temp_day = 40;
	min_temp_night = 40;
	avg_temp_day = 0;
	avg_temp_night = 0;
	count_temp_day = 0;
	count_temp_night = 0;
}

void irrigation_succed(void)
{
	struct tm time_next = time_info;
	time_next.tm_hour += hour_btw_irrigations;
	next_irrigation = mktime(&time_next);
	nvs_set_value_num_u8(LAST_IRRI_MONTH,time_info.tm_mon);
	nvs_set_value_num_u8(LAST_IRRI_DAY,time_info.tm_mday);
	nvs_set_value_num_u8(LAST_IRRI_HOUR,time_info.tm_hour);
	nvs_set_value_num_u8(LAST_IRRI_MIN,time_info.tm_min);
}

void irrigation_in_progress(void){
	next_irrigation += (INTERVAL_RETRY_IRRIGATION_HOUR * 3600);
}

void irrigate_now(void)
{
	parameterIrrigation_t parameter_send;
	parameter_send.water_dl = liter_irrigation * 10 + dLiter_irrigation;
	parameter_send.aditive1_ml_per_L = aditive1;
	parameter_send.aditive2_ml_per_L = aditive2;
	parameter_send.aditive3_ml_per_L = aditive3;
	ESP_LOGI(TAG, "Mandado comando irrigate con los valores: %u dL, %u-%u-%u", parameter_send.water_dl, parameter_send.aditive1_ml_per_L, parameter_send.aditive2_ml_per_L, parameter_send.aditive3_ml_per_L);
	espnow_irrigate(parameter_send);

	next_irrigation += INTERVAL_RETRY_IRRIGATION_SEND_MIN * 60 ;
}

float get_parameter_clima(TipoParametro param)
{
	switch (param)
	{
	case MAX_TEMP_DAY:
		return max_temp_day;
		break;
	case MIN_TEMP_DAY:
		return min_temp_day;
		break;
	case MAX_TEMP_NIGHT:
		return max_temp_night;
		break;
	case MIN_TEMP_NIGHT:
		return min_temp_night;
		break;
	case AVG_TEMP_DAY:
		return avg_temp_day;
		break;
	case AVG_TEMP_NIGHT:
		return avg_temp_night;
		break;
	case TEMP:
		return get_temperature();
		break;
	case HUM_REL:
		return get_rel_humidity();
		break;
	default:
		return -1;
		break;
	}
}