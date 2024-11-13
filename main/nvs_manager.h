#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H
#include <esp_err.h>

esp_err_t nvs_manager_init (void);
void nvs_manager_deinit (void);

/**
 * @brief Retrieves a string value stored in the ESP32's non-volatile storage (NVS).
 *
 * This function retrieves a string value that has been previously stored in the non-volatile 
 * storage (NVS) system using a specific key. The retrieved string is copied to the buffer 
 * provided in `value_ptr`, and the size of the retrieved value is adjusted in `length`.
 *
 * @param[in]     key        Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Shouldn't be empty.
 * @param[out]    out_value  Pointer to the output value.
 *                           May be NULL for nvs_get_str and nvs_get_blob, in this
 *                           case required length will be returned in length argument.
 * @param[inout]  length     A non-zero pointer to the variable holding the length of out_value.
 *                           In case out_value a zero, will be set to the length
 *                           required to hold the value. In case out_value is not
 *                           zero, will be set to the actual length of the value
 *                           written. For nvs_get_str this includes zero terminator.
 * 
 * 
 *  @return
 *             - ESP_OK if the value was retrieved successfully
 *             - ESP_FAIL if there is an internal error; most likely due to corrupted
 *               NVS partition (only if NVS assertion checks are disabled)
 *             - ESP_ERR_NVS_NOT_FOUND if the requested key doesn't exist
 *             - ESP_ERR_NVS_INVALID_HANDLE if handle has been closed or is NULL
 *             - ESP_ERR_NVS_INVALID_NAME if key name doesn't satisfy constraints
 *             - ESP_ERR_NVS_INVALID_LENGTH if \c length is not sufficient to store data
 */
esp_err_t nvs_get_value_str (const char *key, char *value_ptr, size_t *length);

/**
 * @brief Retrieves a 32-bit unsigned integer value from the ESP32's non-volatile storage (NVS).
 *
 * This function retrieves a 32-bit (uint32_t) value previously stored in the non-volatile
 * storage (NVS) system using a specific key.
 *
 * @param[in]     key        Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Should not be empty.
 * @param[out]    value_ptr  Pointer to the output value where the 32-bit value will be stored.
 *
 * @return
 *             - ESP_OK if the value was retrieved successfully
 *             - ESP_FAIL if there is an internal error, likely due to corrupted NVS partition
 *             - ESP_ERR_NVS_NOT_FOUND if the requested key does not exist
 *             - ESP_ERR_NVS_INVALID_HANDLE if handle has been closed or is NULL
 *             - ESP_ERR_NVS_INVALID_NAME if key name does not satisfy constraints
 *             - ESP_ERR_NVS_INVALID_LENGTH if value_ptr is not sufficient to store the data
 */
esp_err_t nvs_get_value_num_u32(const char *key, uint32_t *value_ptr);

/**
 * @brief Retrieves an 8-bit unsigned integer value from the ESP32's non-volatile storage (NVS).
 *
 * This function retrieves an 8-bit (uint8_t) value previously stored in the non-volatile
 * storage (NVS) system using a specific key.
 *
 * @param[in]     key        Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Should not be empty.
 * @param[out]    value_ptr  Pointer to the output value where the 8-bit value will be stored.
 *
 * @return
 *             - ESP_OK if the value was retrieved successfully
 *             - ESP_FAIL if there is an internal error, likely due to corrupted NVS partition
 *             - ESP_ERR_NVS_NOT_FOUND if the requested key does not exist
 *             - ESP_ERR_NVS_INVALID_HANDLE if handle has been closed or is NULL
 *             - ESP_ERR_NVS_INVALID_NAME if key name does not satisfy constraints
 *             - ESP_ERR_NVS_INVALID_LENGTH if value_ptr is not sufficient to store the data
 */
esp_err_t nvs_get_value_num_u8(const char *key, uint8_t *value_ptr);

/**
 * @brief Stores a string value in the ESP32's non-volatile storage (NVS).
 *
 * This function stores a string value in the non-volatile storage (NVS) system using a specific key.
 *
 * @param[in]  key     Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Should not be empty.
 * @param[in]  value   Pointer to the input string to be stored. Must be null-terminated.
 *
 * @return
 *             - ESP_OK if the value was stored successfully
 *             - ESP_ERR_NVS_INVALID_HANDLE if handle has been closed or is NULL
 *             - ESP_ERR_NVS_INVALID_NAME if key name does not satisfy constraints
 *             - ESP_ERR_NVS_INVALID_LENGTH if the length of value is too long
 *             - ESP_ERR_NVS_NO_FREE_PAGES if the NVS storage is full
 */
esp_err_t nvs_set_value_str(const char *key, const char *value);

/**
 * @brief Stores a 32-bit unsigned integer value in the ESP32's non-volatile storage (NVS).
 *
 * This function stores a 32-bit (uint32_t) value in the non-volatile storage (NVS) system using a specific key.
 *
 * @param[in]  key     Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Should not be empty.
 * @param[in]  value   The 32-bit value to be stored.
 *
 * @return
 *             - ESP_OK if the value was stored successfully
 *             - ESP_ERR_NVS_INVALID_HANDLE if handle has been closed or is NULL
 *             - ESP_ERR_NVS_INVALID_NAME if key name does not satisfy constraints
 *             - ESP_ERR_NVS_NO_FREE_PAGES if the NVS storage is full
 */
esp_err_t nvs_set_value_num_u32(const char *key, uint32_t value);

/**
 * @brief Stores an 8-bit unsigned integer value in the ESP32's non-volatile storage (NVS).
 *
 * This function stores an 8-bit (uint8_t) value in the non-volatile storage (NVS) system using a specific key.
 *
 * @param[in]  key     Key name. Maximum length is (NVS_KEY_NAME_MAX_SIZE-1) characters. Should not be empty.
 * @param[in]  value   The 8-bit value to be stored.
 *
 * @return
 *             - ESP_OK if the value was stored successfully
 *             - ESP_ERR_NVS_INVALID_HANDLE if handle has been closed or is NULL
 *             - ESP_ERR_NVS_INVALID_NAME if key name does not satisfy constraints
 *             - ESP_ERR_NVS_NO_FREE_PAGES if the NVS storage is full
 */
esp_err_t nvs_set_value_num_u8(const char *key, uint8_t value);


#endif 