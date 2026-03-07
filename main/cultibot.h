#ifndef CULTIBOT_H
#define CULTIBOT_H

// Definicion de librerias comunes a todos los archivos
#include <esp_err.h>
#include "nvs_manager.h"

// Descomentar las lineas para los test
// #define	TEST_MEMORIA

// -----------------------------------------------------------------------------------
// ---------------  Definicion de valores constantes ---------------------------------
// Constantes del wifi en modo AP
#define ESP_WIFI_SSID "Cultibot"
#define ESP_WIFI_PASS "hola1234"



// -----------------------------------------------------------------------------------
/* ---------------  Definicion de  los accesos a memoria -----------------------------
 -Todos los valores que sean numeros se guardan en memoria como uint8_t
 -los nombres entre comillas son los usados en los JSON
 */
#define WATER_LITER "litres_water"                 
#define WATER_DECILITER "dl_water"                 
#define FERTI_1_ML "additive_1_ml"                 
#define FERTI_2_ML "additive_2_ml"                 
#define FERTI_3_ML "additive_3_ml"                 
#define LAST_IRRI_MIN "min_irrigation"             
#define LAST_IRRI_HOUR "hour_irrigation"           
#define LAST_IRRI_DAY "day_irrigation"
#define LAST_IRRI_MONTH "month_irrigati"              
#define HOURS_BETWEEN_IRRIGATIONS "hours_btw_irri" 
//#define INTERVAL_IRRI_MIN "interval_irri"          
//#define NUM_INTERVAL_IRRIGATION "num_irri_inter"   

#define TEMP_DAY "temp_day"      
#define TEMP_NIGHT "temp_night"         
#define HISTERESIS_DAY "histeresis_day" 
#define HISTERESIS_NIGHT "hister_night" 

#define HUMEDAD_REL_DAY "hum_rel_day"     
#define HUMEDAD_REL_NIGHT "hum_rel_night" 

#define LIGHT_ON_MIN "light_on_m"    
#define LIGHT_OFF_MIN "light_off_m"  
#define LIGHT_ON_HOUR "light_on_h"   
#define LIGHT_OFF_HOUR "light_off_h" 

#define INDOOR_VENT_STATUS "ind_v_status" // 0-> apagado 1->encendido 2->intervalo
#define INDOOR_VENT_M_ON "ind_v_m_on"
#define INDOOR_VENT_M_OFF "ind_v_m_off"  

#define WIFI_SSID "wifi_ssid"   // Tiene una longuitud de 32 bits
#define WIFI_PASS "wifi_pass"   // Tiene una longuitud de 64 bits

#define MQTT_URI "mqtt_uri"


// Funcion para refrescar los parametros despues de ser modificados
void climate_load_parameters_from_nvs(void);

// Tipos y funciones para obtener los parametros del clima
typedef enum
{
    MAX_TEMP_DAY,
    MIN_TEMP_DAY,
    MAX_TEMP_NIGHT,
    MIN_TEMP_NIGHT,
    AVG_TEMP_DAY,
    AVG_TEMP_NIGHT,
    TEMP,
    HUM_REL,
} TipoParametro;
float get_parameter_clima(TipoParametro param);

#endif