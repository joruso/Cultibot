#ifndef _I2C_LCD_H_
#define _I2C_LCD_H_

#include <esp_err.h>

#define SDA_IO GPIO_NUM_5
#define SCL_IO GPIO_NUM_6
#define LCD_ROW 4
#define LCD_COLS 20
#define LCD_I2C_addr 0x27

esp_err_t LCD_init();
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_home(void);
void LCD_clearScreen(void);
void LCD_writeChar(char c);
void LCD_writeStr(char* str);
void LCD_writeUint(uint32_t num);

#endif /* MAIN_I2C_LCD_H_ */
