#include "i2c_lcd.h"
#include <driver/i2c_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "rom/ets_sys.h"

// LCD module defines
#define LCD_LINEONE 0x00   // start of line 1
#define LCD_LINETWO 0x40   // start of line 2
#define LCD_LINETHREE 0x14 // start of line 3
#define LCD_LINEFOUR 0x54  // start of line 4

#define LCD_BACKLIGHT 0x08
#define LCD_NO_BACKLIGHT 0x00
#define LCD_ENABLE 0x04
#define LCD_COMMAND 0x00
#define LCD_WRITE 0x01

#define LCD_SET_DDRAM_ADDR 0x80
#define LCD_READ_BF 0x40

// LCD instructions
#define LCD_CLEAR 0x01             // replace all characters with ASCII 'space'
#define LCD_HOME 0x02              // return cursor to first position on first line
#define LCD_ENTRY_MODE 0x06        // shift cursor from left to right on read/write
#define LCD_DISPLAY_OFF 0x08       // turn display off
#define LCD_DISPLAY_ON 0x0C        // display on, cursor off, don't blink character
#define LCD_FUNCTION_RESET 0x30    // reset the LCD
#define LCD_FUNCTION_SET_4BIT 0x28 // 4-bit data, 2-line display, 5 x 7 font
#define LCD_SET_CURSOR 0x80        // set cursor position

// Pin mappings
// P0 -> RS
// P1 -> RW
// P2 -> E
// P3 -> Backlight
// P4 -> D4
// P5 -> D5
// P6 -> D6
// P7 -> D7

static char TAG[] = "LCD Driver";
static uint8_t LCD_cols;
static uint8_t LCD_rows;
i2c_master_dev_handle_t dev_handle;

static esp_err_t LCD_writeNibble(uint8_t nibble, uint8_t mode);
static esp_err_t LCD_writeByte(uint8_t data, uint8_t mode);
static esp_err_t LCD_pulseEnable(uint8_t nibble);

esp_err_t LCD_init()
{
    esp_err_t err;
    LCD_cols = LCD_COLS;
    LCD_rows = LCD_ROW;

    i2c_master_bus_config_t conf = {
        .sda_io_num = SDA_IO,
        .scl_io_num = SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0};
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&conf, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_I2C_addr,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    vTaskDelay(0.04 * CONFIG_FREERTOS_HZ); // Initial 40 mSec delay

    // Reset the LCD controller
    err = LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND); // First part of reset sequence
    if (err != ESP_OK)
        return err;
    vTaskDelay(0.0041 * CONFIG_FREERTOS_HZ); // 4.1 mS delay (min)
    // LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND);    // second part of reset sequence
    ets_delay_us(200);                                      // 100 uS delay (min)
    err = LCD_writeNibble(LCD_FUNCTION_RESET, LCD_COMMAND); // Third time's a charm
    if (err != ESP_OK)
        return err;
    err = LCD_writeNibble(LCD_FUNCTION_SET_4BIT, LCD_COMMAND); // Activate 4-bit mode
    if (err != ESP_OK)
        return err;
    ets_delay_us(80); // 40 uS delay (min)

    // --- Busy flag now available ---
    // Function Set instruction
    err = LCD_writeByte(LCD_FUNCTION_SET_4BIT, LCD_COMMAND); // Set mode, lines, and font
    if (err != ESP_OK)
        return err;
    ets_delay_us(80);

    // Clear Display instruction
    err = LCD_writeByte(LCD_CLEAR, LCD_COMMAND); // clear display RAM
    if (err != ESP_OK)
        return err;
    vTaskDelay(0.01 * CONFIG_FREERTOS_HZ); // Clearing memory takes a bit longer

    // Entry Mode Set instruction
    err = LCD_writeByte(LCD_ENTRY_MODE, LCD_COMMAND); // Set desired shift characteristics
    if (err != ESP_OK)
        return err;
    ets_delay_us(80);

    return LCD_writeByte(LCD_DISPLAY_ON, LCD_COMMAND); // Ensure LCD is set to on
}

void LCD_setCursor(uint8_t col, uint8_t row)
{
    if (row > LCD_rows - 1)
    {
        ESP_LOGE(TAG, "Cannot write to row %d. Please select a row in the range (0, %d)", row, LCD_rows - 1);
        row = LCD_rows - 1;
    }
    uint8_t row_offsets[] = {LCD_LINEONE, LCD_LINETWO, LCD_LINETHREE, LCD_LINEFOUR};
    LCD_writeByte(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]), LCD_COMMAND);
}

void LCD_writeChar(char c)
{
    LCD_writeByte(c, LCD_WRITE); // Write data to DDRAM
}

void LCD_writeStr(char *str)
{
    while (*str)
    {
        LCD_writeChar(*str++);
    }
}

void LCD_LedScreen()
{
    uint8_t data = LCD_NO_BACKLIGHT & 0xF0;

    i2c_master_transmit(dev_handle, &data, 1, 200);

    LCD_pulseEnable(data); // Clock data into LCD

    i2c_master_transmit(dev_handle, &data, 1, 200);

    LCD_pulseEnable(data); // Clock data into LCD
}

void LCD_home(void)
{
    LCD_writeByte(LCD_HOME, LCD_COMMAND);
    vTaskDelay(0.01 * CONFIG_FREERTOS_HZ); // This command takes a while to complete
}

void LCD_clearScreen(void)
{
    LCD_writeByte(LCD_CLEAR, LCD_COMMAND);
    vTaskDelay(0.01 * CONFIG_FREERTOS_HZ); // This command takes a while to complete
}

static esp_err_t LCD_writeByte(uint8_t data, uint8_t mode)
{
    esp_err_t err;
    err = LCD_writeNibble(data & 0xF0, mode);
    err = LCD_writeNibble((data << 4) & 0xF0, mode);
    return err;
}

static esp_err_t LCD_writeNibble(uint8_t nibble, uint8_t mode)
{
    esp_err_t err;
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;

    err = i2c_master_transmit(dev_handle, &data, 1, 200);
    if (err != ESP_OK)
        return err;

    return LCD_pulseEnable(data); // Clock data into LCD
}

static esp_err_t LCD_pulseEnable(uint8_t data)
{
    esp_err_t err;
    uint8_t towrite = data | LCD_ENABLE;
    err = i2c_master_transmit(dev_handle, &towrite, 1, -1);
    if (err != ESP_OK)
        return err;
    ets_delay_us(1);
    towrite = (data & ~LCD_ENABLE);
    err = i2c_master_transmit(dev_handle, &towrite, 1, -1);

    ets_delay_us(50);
    return err;
}

void LCD_writeUint(uint32_t num)
{
    char char_value[12];
    sprintf(char_value, "%lu       ", num);
    LCD_writeStr(char_value);
}