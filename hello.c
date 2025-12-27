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

/* ===== Font 5x7 ===== */
static const uint8_t font5x7[][5] = {
    /* space */ {0x00,0x00,0x00,0x00,0x00},
    /* ! */     {0x00,0x00,0x5F,0x00,0x00},

    /* '0' */   {0x3E,0x51,0x49,0x45,0x3E},
    /* '1' */   {0x00,0x42,0x7F,0x40,0x00},
    /* '2' */   {0x42,0x61,0x51,0x49,0x46},
    /* '3' */   {0x21,0x41,0x45,0x4B,0x31},
    /* '4' */   {0x18,0x14,0x12,0x7F,0x10},
    /* '5' */   {0x27,0x45,0x45,0x45,0x39},
    /* '6' */   {0x3C,0x4A,0x49,0x49,0x30},
    /* '7' */   {0x01,0x71,0x09,0x05,0x03},
    /* '8' */   {0x36,0x49,0x49,0x49,0x36},
    /* '9' */   {0x06,0x49,0x49,0x29,0x1E},

    /* 'A' */   {0x7E,0x11,0x11,0x11,0x7E},
    /* 'B' */   {0x7F,0x49,0x49,0x49,0x36},
    /* 'C' */   {0x3E,0x41,0x41,0x41,0x22},
    /* 'D' */   {0x7F,0x41,0x41,0x22,0x1C},
    /* 'E' */   {0x7F,0x49,0x49,0x49,0x41},
    /* 'F' */   {0x7F,0x09,0x09,0x09,0x01},
    /* 'G' */   {0x3E,0x41,0x49,0x49,0x7A},
    /* 'H' */   {0x7F,0x08,0x08,0x08,0x7F},
    /* 'I' */   {0x00,0x41,0x7F,0x41,0x00},
    /* 'J' */   {0x20,0x40,0x41,0x3F,0x01},
    /* 'K' */   {0x7F,0x08,0x14,0x22,0x41},
    /* 'L' */   {0x7F,0x40,0x40,0x40,0x40},
    /* 'M' */   {0x7F,0x02,0x0C,0x02,0x7F},
    /* 'N' */   {0x7F,0x04,0x08,0x10,0x7F},
    /* 'O' */   {0x3E,0x41,0x41,0x41,0x3E},
    /* 'P' */   {0x7F,0x09,0x09,0x09,0x06},
    /* 'Q' */   {0x3E,0x41,0x51,0x21,0x5E},
    /* 'R' */   {0x7F,0x09,0x19,0x29,0x46},
    /* 'S' */   {0x46,0x49,0x49,0x49,0x31},
    /* 'T' */   {0x01,0x01,0x7F,0x01,0x01},
    /* 'U' */   {0x3F,0x40,0x40,0x40,0x3F},
    /* 'V' */   {0x1F,0x20,0x40,0x20,0x1F},
    /* 'W' */   {0x7F,0x20,0x18,0x20,0x7F},
    /* 'X' */   {0x63,0x14,0x08,0x14,0x63},
    /* 'Y' */   {0x07,0x08,0x70,0x08,0x07},
    /* 'Z' */   {0x61,0x51,0x49,0x45,0x43},
};

/* ===== Draw char ===== */
static void lcd_char(char c) {
    uint8_t idx;

    // if (c == ' ') idx = 0;
    // else if (c == '!') idx = 1;
    // else if (c >= '0' && c <= '9') idx = 2 + (c - '0');
    // else if (c >= 'A' && c <= 'Z') idx = 12 + (c - 'A');
    // else return;

    idx=c;
    for (int i = 0; i < 5; i++)
        // lcd_data(font5x7[idx][i]);        
        lcd_data(ASCII[idx][i]);

    lcd_data(0x00); // space between chars
}

/* ===== Draw string ===== */
// static void lcd_print(const char *s) {
//     while (*s)
//         lcd_char(*s++);
// }
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
    // lcd_print("HELLO WORLD!");
    lcd_print("ЕЁЖЗИЙКЛМНОП");
    /* 🔴 ОТКЛЮЧАЕМ SPI */
    SSI0_CR1_R &= ~(1<<1);   // Disable SSI
    LCD_CS_HIGH();           // LCD deselect


    while (1) {
        // GPIO_PORTF_DATA_R ^= (1<<1);
        delay(400000);
    }
}
