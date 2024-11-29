#ifndef __protocolWater_H__
#define __protocolWater_H__

#include <stdint.h>
#include <esp_now.h>

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

typedef enum
{
    COMANDO_ACK,
    COMANDO_PAIR,
    COMANDO_ALIVE,
    COMANDO_TEST_10_SEG,
    COMANDO_SET_IRRIGATION,
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
    union
    {
        parameterIrrigation_t paramIrrigation;
    };
} data_water_t;

typedef struct __attribute__((packed))
{
    uint8_t mac_addr [ESP_NOW_ETH_ALEN];
    data_water_t data;
} packet_water_t;


#endif