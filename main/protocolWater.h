#ifndef __protocolWater_H__
#define __protocolWater_H__

#include <stdint.h>
#include <esp_now.h>

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#define BROADCAST_DIR \
    (uint8_t[ESP_NOW_ETH_ALEN]) { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

#define ESPNOW_TIMEOUT_S 20
#define ESPNOW_RETRYTIMEOUT_S 1
#define ESPNOW_MAX_RETRIES 3

typedef enum
{
    COMANDO_ACK,                // BIDIRECTIONAL
    COMANDO_PAIR,               // MAIN -> AUX
    COMANDO_ALIVE,              // AUX -> MAIN
    COMANDO_TEST_10_SEG,        // MAIN -> AUX
    COMANDO_SET_IRRIGATION,     // MAIN -> AUX
    COMANDO_IRRIGATE,           // MAIN -> AUX
    COMANDO_IRRIGATE_OK,        // AUX -> MAIN   
    COMANDO_IRRIGATE_FAIL,      // AUX -> MAIN
} tipoComando;

typedef struct __attribute__((packed))
{
    uint16_t water_dl;
    uint8_t aditive1_ml_per_L;
    uint8_t aditive2_ml_per_L;
    uint8_t aditive3_ml_per_L;
} parameterIrrigation_t;

typedef struct __attribute__((packed))
{
    uint8_t comando;
    uint16_t seq_num;
    uint16_t crc;
    parameterIrrigation_t paramIrrigation;
    
} data_water_t;

typedef struct __attribute__((packed))
{
    uint8_t mac_addr [ESP_NOW_ETH_ALEN];
    data_water_t data;
} packet_water_t;


#endif