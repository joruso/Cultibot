/*
 * menu_LCD.c
 *
 *  Created on: 12 mar 2024
 *      Author: Joruso
 */

#include "menu_LCD.h"
#include "cultibot.h"
#include "i2c_lcd.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "sntp.h"
#include <esp_log.h>
#include "nvs_manager.h"
#include "nvs.h"
#include "espnow_water.h"

#include "esp_timer.h"

#include "wifi.h"

#define TIMEOUT 10000000         // 10s
#define INTERVAL_SPEEDER 1000000 // 1s
#define INTERVAL_INCREMENT 10000 // 10ms

#define GPIO_INPUT_PIN_SEL ((1ULL << PIN_CLK) | (1ULL << PIN_DT) | (1ULL << PIN_SW))
#define ESP_INTR_FLAG_DEFAULT 0

const static char *TAG = "MENU_LCD";
static int8_t estado = -1;
static bool press_sw = false;

static void IRAM_ATTR switch_isr_handler(void *arg)
{

    if (gpio_get_level(PIN_SW))
    {
        press_sw = false;
    }
    else
    {
        if (estado == -1)
        {
            estado = 0;
        }
        press_sw = true;
    }
}

void init_gpio_menu()
{
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = (1ULL << PIN_SW);
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(PIN_SW, switch_isr_handler, NULL);
}

size_t strfTemp_hum(char *str)
{
    float humidity = get_parameter_clima(HUM_REL);
    float temperature = get_parameter_clima(TEMP);
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

void refresh_info_screen()
{
    char strtime_buffer[LCD_COLS];
    time_t now;
    struct tm time_info;

    time(&now);
    localtime_r(&now, &time_info);

    LCD_home();
    strftime(strtime_buffer, sizeof(strtime_buffer), "%F %T", &time_info);
    LCD_writeStr(strtime_buffer);

    LCD_setCursor(0, 1);
    strfTemp_hum(strtime_buffer);
    LCD_writeStr(strtime_buffer);

    static uint8_t lastState;
    if (espnow_is_conected() != lastState)
    {
        lastState = espnow_is_conected();
        LCD_setCursor(0, 2);
        if (lastState == 0)
        {
            LCD_writeStr("Dev water not found");
        }
        else
        {
            LCD_writeStr("                    ");
        }
    }
}

static int8_t show_menu(char *names[])
{
    uint8_t number_items = 0;
    while (names[number_items] != NULL)
    {
        number_items++;
    }
    uint8_t interval_estado[2] = {estado * 10, (estado * 10) + number_items};
    // ESP_LOGI(TAG, "first: %i, end: %i", interval_estado[0], interval_estado[1]);
    uint8_t sig_estado = interval_estado[0];
    uint8_t extraPos = 0;
    uint8_t dial_antiguo = 0;
    uint8_t dial;

    LCD_clearScreen();
    LCD_writeChar('>');
    for (uint8_t i = 0; i < LCD_ROW && i <= number_items - 1; i++)
    {
        LCD_setCursor(2, i);
        LCD_writeStr(names[i]);
    }
    uint64_t lastchange = esp_timer_get_time();

    while (press_sw)
    {
        vTaskDelay(1);
    } // evita pulsaciones indeseadas

    while (!press_sw && lastchange + TIMEOUT > esp_timer_get_time())
    {
        dial = ((gpio_get_level(PIN_DT) << 1) | gpio_get_level(PIN_CLK));
        if (dial == 3 && (dial_antiguo == 1 || dial_antiguo == 2) && !press_sw) // el encoder ha cambiado de posicion
        {
            if (dial_antiguo == 1)
            {
                if (sig_estado < interval_estado[1] - 1)
                {
                    sig_estado++;
                    if (sig_estado >= (interval_estado[0] + LCD_ROW + extraPos))
                    {
                        extraPos++;

                        LCD_clearScreen();
                        for (uint8_t i = 0; i < LCD_ROW && i <= number_items - 1; i++)
                        {
                            LCD_setCursor(2, i);
                            // ESP_LOGI(TAG, "posi: %i, value:", i + extraPos);
                            LCD_writeStr(names[i + extraPos]);
                        }
                        LCD_setCursor(0, sig_estado % 10 - extraPos);
                        LCD_writeChar('>');
                    } // se tiene que refrescar la pantalla -> extraPos++
                    else
                    {
                        LCD_setCursor(0, sig_estado % 10 - extraPos - 1);
                        LCD_writeChar(' ');
                        LCD_setCursor(0, sig_estado % 10 - extraPos);
                        LCD_writeChar('>');
                    } // no se ha refrescado la pantalla
                } // se puede incrementar (sig_estado < interval_estado[1] - 1)
            } // se ha incrementado el dial
            else
            {
                if (sig_estado > interval_estado[0])
                {
                    sig_estado--;
                    if (sig_estado < (interval_estado[0] + extraPos))
                    {
                        extraPos--;

                        LCD_clearScreen();
                        for (uint8_t i = 0; i < LCD_ROW && i <= number_items - 1; i++)
                        {
                            LCD_setCursor(2, i);
                            // ESP_LOGI(TAG, "posi: %i, value:", i + extraPos);
                            LCD_writeStr(names[i + extraPos]);
                        }
                        LCD_setCursor(0, sig_estado % 10 - extraPos);
                        LCD_writeChar('>');
                    }
                    else
                    {

                        LCD_setCursor(0, sig_estado % 10 - extraPos + 1);
                        LCD_writeChar(' ');
                        LCD_setCursor(0, sig_estado % 10 - extraPos);
                        LCD_writeChar('>');
                    }
                }

                // ESP_LOGI(TAG, "sig: %i, extra:%i", sig_estado, extraPos);
            }
            lastchange = esp_timer_get_time();
        }
        dial_antiguo = dial;
        vTaskDelay(1);
    } // fin del while puesto que se ha pulsado el switch o se ha llegado al timeout

    if (lastchange + TIMEOUT < esp_timer_get_time() || sig_estado == 0) // si se llega al timeout se vuelve al estado inicial
    {
        climate_load_parameters_from_nvs();
        while (press_sw)
        {
        }
        return -1;
    }
    if (sig_estado % 10 == 0)
    { // Se ha presionado VOLVER en el menu, no se puede tener mas de 10 opciones por menu
        while (press_sw)
        {
        }
        return 0;
    }
    while (press_sw)
    {
    }
    return sig_estado;
}

// macro para ahorrar algunas lineas de codigo
#define UPDATE_SPEEDER(lastchange, INTERVAL_SPEEDER, CW, CW_previous, speeder)                \
    if (((lastchange) + (INTERVAL_SPEEDER)) >= esp_timer_get_time() && (CW) == (CW_previous)) \
    {                                                                                         \
        (speeder)++;                                                                          \
        if ((speeder) >= 10)                                                                  \
        {                                                                                     \
            (speeder) = 20;                                                                   \
        }                                                                                     \
    }                                                                                         \
    else                                                                                      \
    {                                                                                         \
        (speeder) = 1;                                                                        \
    }

esp_err_t mod_variable(char *text, char *var, uint8_t min_value, uint8_t max_value)
{
    LCD_clearScreen();
    LCD_setCursor(1, 0);
    LCD_writeStr(text);
    uint8_t var_value, value_temp;
    esp_err_t err;
    err = nvs_get_value_num_u8(var, &var_value);
    value_temp = var_value;

    // Imprimo el valor en la pantalla LCD
    LCD_setCursor(3, 2);
    LCD_writeUint(var_value);

    uint8_t dial_antiguo = 0;
    uint8_t dial;
    uint64_t lastchange = esp_timer_get_time();
    uint8_t speeder = 1;
    bool CW = false;
    bool CW_previous = false;

    while (press_sw)
    {
        vTaskDelay(1);
    } // evita pulsaciones indeseadas

    while (!press_sw && lastchange + TIMEOUT > esp_timer_get_time())
    {
        dial = ((gpio_get_level(PIN_DT) << 1) | gpio_get_level(PIN_CLK));
        if (dial == 3 && (dial_antiguo == 1 || dial_antiguo == 2) && !press_sw) // el encoder ha cambiado de posicion
        {
            CW_previous = CW;
            if (dial_antiguo == 1) // se ha girado a la derecha
            {
                CW = true;
                UPDATE_SPEEDER(lastchange, INTERVAL_SPEEDER, CW, CW_previous, speeder);
                value_temp = value_temp + speeder;
                if (value_temp > max_value)
                {
                    value_temp = max_value;
                }
                LCD_setCursor(3, 2);
                LCD_writeUint(value_temp);
            }
            else // se ha girado a la izquierda
            {
                CW = false;
                UPDATE_SPEEDER(lastchange, INTERVAL_SPEEDER, CW, CW_previous, speeder);
                value_temp = value_temp - speeder;
                if (value_temp < min_value || value_temp > max_value)
                {
                    value_temp = min_value;
                }
                LCD_setCursor(3, 2);
                LCD_writeUint(value_temp);
            }
            lastchange = esp_timer_get_time();
        }
        dial_antiguo = dial;
        vTaskDelay(1);
    }
    if (lastchange + TIMEOUT < esp_timer_get_time() || var_value == value_temp) // si se llega al timeout no se modifica la variable
    {
        return ESP_OK;
    }
    err = nvs_set_value_num_u8(var, value_temp);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Var: %s saved with value: %u", var, value_temp);
    }
    return err;
}

void pairing_udp()
{

    int seg = 21;
    LCD_clearScreen();
    LCD_writeStr("Emparejando");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(wifi_send_UDP_broadcast_info(&seg));
    LCD_clearScreen();
}

void init_menu()
{

    if (LCD_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Menu no inicilizado, falla conexion I2C");
        return;
    }
    LCD_clearScreen();
    init_gpio_menu();

    while (1)
    {

        ESP_LOGI(TAG, "est: %i", estado);
        switch (estado)
        {
        case -1: // INFORMACION

            uint64_t lastchange = esp_timer_get_time();
            LCD_clearScreen();
            while (estado == -1 && esp_timer_get_time() < lastchange + TIMEOUT) // Me quedo en este estado hasta que se presione el switch
            {
                // ESP_LOGI(TAG, "est: %i, sig:", estado);
                refresh_info_screen();
                vTaskDelay(0.9 * CONFIG_FREERTOS_HZ);
            }

            if (esp_timer_get_time() > lastchange + TIMEOUT)
            {
                LCD_LedScreen();
                while (estado == -1)
                {
                    vTaskDelay(0.5 * CONFIG_FREERTOS_HZ);
                }
                estado = -1;
            }

            break;

        case 0: // MUESTRA LAS LISTA DE OPCIONES
            estado = show_menu((char *[]){"Volver", "Riego", "Temperatura", "Humedad", "Iluminacion", "Ventiladores", "Hora", "Mostrar param", NULL});
            break;

        case 1: // MENU DEL RIEGO
            estado = show_menu((char *[]){"Volver", "Cantidad de agua", "Crecimiento", "Floracion", "Aditivo 3", "Ultimo Riego", "Horas entre riegos", "Riego por partes", "Hacer Test", NULL});
            break;

        case 11: // MODIFICO LA VARIABLE LITROS DE AGUA
            ESP_ERROR_CHECK(mod_variable("Litros ", WATER_LITER, 0, 240));

            ESP_ERROR_CHECK(mod_variable("+ dL de agua", WATER_DECILITER, 0, 9));
            estado = 1;
            break;
        case 12:
            ESP_ERROR_CHECK(mod_variable("Aditivo 1 ml*1L", FERTI_1_ML, 0, 20));
            estado = 1;
            break;
        case 13:
            ESP_ERROR_CHECK(mod_variable("Aditivo 2 ml*1L", FERTI_2_ML, 0, 20));
            estado = 1;
            break;
        case 14:
            ESP_ERROR_CHECK(mod_variable("Aditivo 3 ml*1L", FERTI_3_ML, 0, 20));
            estado = 1;
            break;
        case 15:
            ESP_ERROR_CHECK(mod_variable("Dia", LAST_IRRI_DAY, 1, 31));
            ESP_ERROR_CHECK(mod_variable("Hora", LAST_IRRI_HOUR, 0, 23));
            ESP_ERROR_CHECK(mod_variable("Minuto", LAST_IRRI_MIN, 0, 59));
            estado = 1;
            break;
        case 16:
            ESP_ERROR_CHECK(mod_variable("Horas entre riegos", HOURS_BETWEEN_IRRIGATIONS, 1, 96));
            estado = 1;
            break;
        case 17:
            // ESP_ERROR_CHECK(mod_variable("Numero de pausas", NUM_INTERVAL_IRRIGATION, 0, 10));
            // ESP_ERROR_CHECK(mod_variable("Min entre huecos", INTERVAL_IRRI_MIN, 0, 10));
            estado = 1;
            break;
        case 18: // hacer el test de riego
            if (espnow_test(10) != ESP_OK)
            {
                LCD_clearScreen();
                LCD_setCursor(2, 1);
                LCD_writeStr("Riego desconectado");
                LCD_setCursor(2, 2);
                LCD_writeStr(" o inalcanzable");
                vTaskDelay(1600 / portTICK_PERIOD_MS);
            }

            estado = 1;
            break;
        case 2: // MENU DE LA TEMPERATURA
            estado = show_menu((char *[]){"Volver", "Temperartura Dia", "Temperatura Noche", "Histeresis Dia", "Histeresis Noche", NULL});
            break;
        case 21:
            ESP_ERROR_CHECK(mod_variable("Temp Dia Celsius", TEMP_DAY, 10, 40));
            estado = 2;
            break;
        case 22:
            ESP_ERROR_CHECK(mod_variable("Temp Noche Celsius", TEMP_NIGHT, 10, 40));
            estado = 2;
            break;
        case 23:
            ESP_ERROR_CHECK(mod_variable("Histe Dia Celsius", HISTERESIS, 1, 5));
            estado = 2;
            break;

        case 3: // MENU DE LA HUMEDAD
            estado = show_menu((char *[]){"Volver", "Humedad Dia", "Humedad Noche", NULL});
            break;
        case 31:
            ESP_ERROR_CHECK(mod_variable("Hum Relativa Dia", HUMEDAD_REL_DAY, 10, 100));
            estado = 3;
            break;
        case 32:
            ESP_ERROR_CHECK(mod_variable("Hum Relativa Noche", HUMEDAD_REL_NIGHT, 10, 100));
            estado = 3;
            break;
        case 4: // MENU DE LA ILUMINACION
            estado = show_menu((char *[]){"Volver", "Encendido luces", "Apagado luces", NULL});
            break;
        case 41:
            ESP_ERROR_CHECK(mod_variable("Hora encendido", LIGHT_ON_HOUR, 0, 23));
            ESP_ERROR_CHECK(mod_variable("Min encendido", LIGHT_ON_MIN, 0, 59));
            estado = 4;
            break;
        case 42:
            ESP_ERROR_CHECK(mod_variable("Hora apagado", LIGHT_OFF_HOUR, 0, 23));
            ESP_ERROR_CHECK(mod_variable("Min apagado", LIGHT_OFF_MIN, 0, 59));
            estado = 4;
            break;
        case 5: // MENU DE LOS VENTILADORES
            estado = show_menu((char *[]){"Volver", "Ventilador interno", "Extractores", NULL});
            break;
        case 51:
            uint8_t *aux;
            aux = (uint8_t *)malloc(sizeof(uint8_t));
            if (aux == NULL)
            {
                free(aux);
                assert(0);
            }
            LCD_clearScreen();
            LCD_setCursor(1, 0);
            LCD_writeStr("0-OFF 1-ON 2-ON/OFF");
            LCD_setCursor(1, 2);
            LCD_writeStr("3-CONMUTAR AMBOS");
            while (!press_sw)
            {
            }

            ESP_ERROR_CHECK(mod_variable("Fan 1 Mode", INDOOR_1_VENT_STATUS, 0, 3));
            ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_1_VENT_STATUS, aux));
            if (*aux == INTERMITENT)
            {
                ESP_ERROR_CHECK(mod_variable("Min encendido", INDOOR_1_VENT_M_ON, 1, 250));
                ESP_ERROR_CHECK(mod_variable("Min apagado", INDOOR_1_VENT_M_OFF, 1, 250));
            }
            if (*aux == TOGGLE) // En este caso no se modifica el fan 2
            {
                ESP_ERROR_CHECK(mod_variable("Min Intervalo 1", INDOOR_1_VENT_M_ON, 1, 250));
                ESP_ERROR_CHECK(mod_variable("Min intervalo 2", INDOOR_1_VENT_M_OFF, 1, 250));
            }
            else
            {
                LCD_clearScreen();
                LCD_setCursor(1, 0);
                LCD_writeStr("0-OFF 1-ON 2-ON/OFF");
                while (!press_sw)
                {
                }
                ESP_ERROR_CHECK(mod_variable("Fan 2 Mode", INDOOR_2_VENT_STATUS, 0, 2));
                ESP_ERROR_CHECK(nvs_get_value_num_u8(INDOOR_2_VENT_STATUS, aux));
                if (*aux == INTERMITENT)
                {
                    ESP_ERROR_CHECK(mod_variable("Min encendido", INDOOR_2_VENT_M_ON, 1, 250));
                    ESP_ERROR_CHECK(mod_variable("Min apagado", INDOOR_2_VENT_M_OFF, 1, 250));
                }
            }
            free(aux);
            estado = 5;
            break;
        case 52:
            ESP_ERROR_CHECK(mod_variable("Hora apagado", LIGHT_OFF_HOUR, 0, 23));
            ESP_ERROR_CHECK(mod_variable("Min apagado", LIGHT_OFF_MIN, 0, 59));
            estado = 5;
            break;
        case 6: // MENU DE LA HORA
            estado = show_menu((char *[]){"Volver", "Minutos", "Hora", NULL});
            break;
        case 7: // EMPAREJAMIENTO POR UDP
            pairing_udp();
            estado = -1;
            break;
        default:
            break;
        }
    }
}