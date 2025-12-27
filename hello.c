#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include "font.h"

/* ===== LCD pins (YOUR WIRING) ===== */
#define LCD_CS_LOW()   (GPIO_PORTA_DATA_R &= ~(1 << 6))
#define LCD_CS_HIGH()  (GPIO_PORTA_DATA_R |=  (1 << 6))

#define LCD_DC_CMD()   (GPIO_PORTA_DATA_R &= ~(1 << 7))
#define LCD_DC_DATA()  (GPIO_PORTA_DATA_R |=  (1 << 7))

#define LCD_RST_LOW()  (GPIO_PORTB_DATA_R &= ~(1 << 2))
#define LCD_RST_HIGH() (GPIO_PORTB_DATA_R |=  (1 << 2))

/* ===== Delay ===== */
static void delay(volatile uint32_t d) {
    while (d--) __asm("nop");
}

/* ===== SPI ===== */
static void spi_send(uint8_t data) {
    while ((SSI0_SR_R & (1 << 1)) == 0);
    SSI0_DR_R = data;
    while (SSI0_SR_R & (1 << 4));
}

/* ===== LCD low level ===== */
static void lcd_cmd(uint8_t c) {
    LCD_DC_CMD();
    LCD_CS_LOW();
    spi_send(c);
    LCD_CS_HIGH();
}

static void lcd_data(uint8_t d) {
    LCD_DC_DATA();
    LCD_CS_LOW();
    spi_send(d);
    LCD_CS_HIGH();
}

/* ===== LCD init ===== */
static void lcd_init(void) {
    LCD_RST_LOW();
    delay(50000);
    LCD_RST_HIGH();

    lcd_cmd(0x21);              // extended mode
    lcd_cmd(0x80 | 0x3E);       // contrast (Vop)
    lcd_cmd(0x13);              // bias 1:48
    lcd_cmd(0x06);              // temperature coefficient
    lcd_cmd(0x20);              // basic mode
    lcd_cmd(0x0C);              // normal display
}

/* ===== Cursor ===== */
static void lcd_goto(uint8_t x, uint8_t y) {
    lcd_cmd(0x80 | x);  // column 0..83
    lcd_cmd(0x40 | y);  // row 0..5
}

/* ===== Clear ===== */
static void lcd_clear(void) {
    lcd_goto(0,0);
    for (int i = 0; i < 504; i++)
        lcd_data(0x00);
}

/* ===== Draw char ===== */
static void lcd_char(char c) {
    uint8_t idx;

    idx=c;
    for (int i = 0; i < 5; i++)
        lcd_data(ASCII[idx][i]);
    lcd_data(0x00); // space between chars
}

/* ===== Draw string ===== */
static void lcd_print(const char *s) {
    unsigned char *ptr = (unsigned char *)s;
    
    while (*ptr) {
        unsigned char c = *ptr++;

        // 1. Если это обычный ASCII (английский, цифры, знаки)
        if (c < 128) {
            lcd_char(c);
        } 
        // 2. Если это первый байт русской буквы (UTF-8)
        else if (c == 0xD0) {
            c = *ptr++; // Берем второй байт
            if (c == 0x81) {
                lcd_char(0xF0); // Символ 'Ё' в CP866
            } else if (c >= 0x90 && c <= 0xBF) {
                // Диапазон 'А'-'п': в UTF-8 это 0x90-0xBF, в CP866 это 0x80-0xAF
                lcd_char(c - 0x10); 
            }
        } 
        else if (c == 0xD1) {
            c = *ptr++; // Берем второй байт
            if (c == 0x91) {
                lcd_char(0xF1); // Символ 'ё' в CP866
            } else if (c >= 0x80 && c <= 0x8F) {
                // Диапазон 'р'-'я': в UTF-8 это 0x80-0x8F, в CP866 это 0xE0-0xEF
                lcd_char(c + 0x60);
            }
        }
    }
}

/* ===== MAIN ===== */
int main(void) {

    SYSCTL_RCGCGPIO_R |= (1<<0)|(1<<1)|(1<<5);
    SYSCTL_RCGCSSI_R  |= (1<<0);

    while(!(SYSCTL_PRGPIO_R&(1<<0)));

    GPIO_PORTF_DIR_R |= (1<<1);
    GPIO_PORTF_DEN_R |= (1<<1);

    GPIO_PORTA_AFSEL_R |= (1<<2)|(1<<5);
    GPIO_PORTA_PCTL_R  = (GPIO_PORTA_PCTL_R & ~0x00F0F000) | 0x00202000;
    GPIO_PORTA_DEN_R  |= (1<<2)|(1<<5);

    GPIO_PORTA_DIR_R |= (1<<6)|(1<<7);
    GPIO_PORTA_DEN_R |= (1<<6)|(1<<7);

    GPIO_PORTB_DIR_R |= (1<<2);
    GPIO_PORTB_DEN_R |= (1<<2);

    LCD_CS_HIGH();
    LCD_RST_HIGH();

    SSI0_CR1_R = 0;
    SSI0_CPSR_R = 20;
    SSI0_CR0_R  = 0x07;
    SSI0_CR1_R |= (1<<1);

    lcd_init();
    lcd_clear();

    /* HELLO WORLD centered */
    lcd_goto(0, 0);
    lcd_print("120КЛ:МН098");
    /* 🔴 ОТКЛЮЧАЕМ SPI */
    SSI0_CR1_R &= ~(1<<1);   // Disable SSI
    LCD_CS_HIGH();           // LCD deselect


    while (1) {
        // GPIO_PORTF_DATA_R ^= (1<<1);
        delay(400000);
    }
}
