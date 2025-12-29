#ifndef DASHBOARD_LCD_H
#define DASHBOARD_LCD_H

#include <stdint.h>

void lcd_hw_init(void);
void lcd_init(void);
void lcd_clear(void);
void lcd_goto(uint8_t x, uint8_t y);
void lcd_print(const char *s);
void lcd_print_line(const char *s);
void lcd_test_pattern(void);

#endif // DASHBOARD_LCD_H
