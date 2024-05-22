#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H
#include <esp_err.h>

esp_err_t nvs_manager_init ();
void nvs_manager_deinit ();

esp_err_t nvs_get_value_str (const char *key, char *value_ptr, size_t *length);
esp_err_t nvs_get_value_num_u32 (const char *key, uint32_t *value_ptr);
esp_err_t nvs_get_value_num_u8 (const char *key, uint8_t *value_ptr);

esp_err_t nvs_set_value_str (const char *key, char *value, size_t *length);
esp_err_t nvs_set_value_num_u32 (const char *key, uint32_t value);
esp_err_t nvs_set_value_num_u8 (const char *key, uint8_t value);

#endif 