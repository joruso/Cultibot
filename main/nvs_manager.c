#include "nvs_manager.h"
#include "nvs_flash.h"

#define NAMESPACE_NVS "storage"

static nvs_handle_t nvs_handle_manager;

esp_err_t nvs_manager_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(NAMESPACE_NVS, NVS_READWRITE, &nvs_handle_manager);
    return err;
}


void nvs_manager_deinit(void)
{
    nvs_close(nvs_handle_manager);
}

esp_err_t nvs_get_value_str(const char *key, char *value, size_t *length)
{
    esp_err_t err = nvs_get_str(nvs_handle_manager, key, value, length);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return nvs_set_str(nvs_handle_manager, key, "value");
    }
    return err;
}

esp_err_t nvs_get_value_num_u32(const char *key, uint32_t *value)
{
    esp_err_t err = nvs_get_u32(nvs_handle_manager, key, value);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return nvs_set_value_num_u32(key, 1000);
    }
    return err;
}

esp_err_t nvs_get_value_num_u8(const char *key, uint8_t *value)
{
    esp_err_t err = nvs_get_u8(nvs_handle_manager, key, value);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return nvs_set_value_num_u8(key, 10);
    }
    return err;
}

esp_err_t nvs_set_value_str(const char *key, const char *value)
{
    return nvs_set_str(nvs_handle_manager, key, value);
}

esp_err_t nvs_set_value_num_u32(const char *key, uint32_t value)
{
    return nvs_set_u32(nvs_handle_manager, key, value);
}

esp_err_t nvs_set_value_num_u8(const char *key, uint8_t value)
{
    return nvs_set_u8(nvs_handle_manager, key, value);
}