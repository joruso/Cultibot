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
    COMANDO_ACK,
    COMANDO_PAIR,
    COMANDO_ALIVE,
    COMANDO_TEST,
    COMANDO_SET_IRRIGATION,
    COMANDO_IRRIGATE,
    COMANDO_IRRIGATING,
    COMANDO_IRRIGATE_OK,
    COMANDO_IRRIGATE_FAIL,
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

typedef enum
{
    SEND_ESPNOW,
    RECV_ESPNOW
} event_type_t;

typedef struct __attribute__((packed))
{
    uint8_t mac_addr [ESP_NOW_ETH_ALEN];
    data_water_t data;
    event_type_t id;
} packet_water_t;


#endif