#ifndef MAIN_I2C_LCD_H_
#define MAIN_I2C_LCD_H_

void LCD_init();
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_home(void);
void LCD_clearScreen(void);
void LCD_writeChar(char c);
void LCD_writeStr(char* str);


#endif /* MAIN_I2C_LCD_H_ */
