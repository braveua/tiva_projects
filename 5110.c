#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

/* ===== LCD pins (UNDER YOUR WIRING) ===== */
/* CS  -> PA6 */
#define LCD_CS_LOW()   (GPIO_PORTA_DATA_R &= ~(1 << 6))
#define LCD_CS_HIGH()  (GPIO_PORTA_DATA_R |=  (1 << 6))

/* DC  -> PA7 */
#define LCD_DC_CMD()   (GPIO_PORTA_DATA_R &= ~(1 << 7))
#define LCD_DC_DATA()  (GPIO_PORTA_DATA_R |=  (1 << 7))

/* RST -> PB2 */
#define LCD_RST_LOW()  (GPIO_PORTB_DATA_R &= ~(1 << 2))
#define LCD_RST_HIGH() (GPIO_PORTB_DATA_R |=  (1 << 2))

/* ===== Simple delay ===== */
static void delay(volatile uint32_t d) {
    while (d--) __asm("nop");
}

/* ===== SPI send ===== */
static void spi_send(uint8_t data) {
    while ((SSI0_SR_R & (1 << 1)) == 0); // TX FIFO not full
    SSI0_DR_R = data;
    while (SSI0_SR_R & (1 << 4));        // Busy
}

/* ===== LCD low-level ===== */
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

    lcd_cmd(0x21);   // extended instruction set
    lcd_cmd(0xBF);   // HIGH contrast (important!)
    lcd_cmd(0x04);   // temperature coefficient
    lcd_cmd(0x14);   // bias mode
    lcd_cmd(0x20);   // basic instruction set
    lcd_cmd(0x0C);   // normal display
}

/* ===== Fill screen ===== */
static void lcd_fill(uint8_t v) {
    for (int i = 0; i < 504; i++)
        lcd_data(v);
}

/* ===== MAIN ===== */
int main(void) {

    /* === Enable clocks: GPIOA, GPIOB, GPIOF, SSI0 === */
    SYSCTL_RCGCGPIO_R |= (1 << 0) | (1 << 1) | (1 << 5);
    SYSCTL_RCGCSSI_R  |= (1 << 0);

    while (!(SYSCTL_PRGPIO_R & (1 << 0)));
    while (!(SYSCTL_PRGPIO_R & (1 << 1)));

    /* === LED (PF1 RED) === */
    GPIO_PORTF_DIR_R |= (1 << 1);
    GPIO_PORTF_DEN_R |= (1 << 1);

    /* === SPI pins ===
       PA2 = SSI0CLK
       PA5 = SSI0TX
    */
    GPIO_PORTA_AFSEL_R |= (1 << 2) | (1 << 5);
    GPIO_PORTA_PCTL_R  = (GPIO_PORTA_PCTL_R & ~0x00F0F000) | 0x00202000;
    GPIO_PORTA_DEN_R  |= (1 << 2) | (1 << 5);

    /* === GPIO pins ===
       PA6 = CS
       PA7 = DC
       PB2 = RST
    */
    GPIO_PORTA_AFSEL_R &= ~((1 << 6) | (1 << 7));
    GPIO_PORTA_DIR_R   |=  (1 << 6) | (1 << 7);
    GPIO_PORTA_DEN_R   |=  (1 << 6) | (1 << 7);

    GPIO_PORTB_AFSEL_R &= ~(1 << 2);
    GPIO_PORTB_DIR_R   |=  (1 << 2);
    GPIO_PORTB_DEN_R   |=  (1 << 2);

    LCD_CS_HIGH();
    LCD_RST_HIGH();

    /* === SSI0 config === */
    SSI0_CR1_R = 0;
    SSI0_CPSR_R = 20;        // slow & safe
    SSI0_CR0_R  = 0x07;      // 8-bit, SPI mode 0
    SSI0_CR1_R |= (1 << 1); // enable SSI

    /* === Proof of life: blink LED === */
    GPIO_PORTF_DATA_R |= (1 << 1);
    delay(300000);
    GPIO_PORTF_DATA_R &= ~(1 << 1);

    /* === LCD TEST === */
    lcd_init();
    lcd_fill(0xFF);          // FULL BLACK SCREEN

    while (1) {
        GPIO_PORTF_DATA_R ^= (1 << 1);
        delay(500000);
    }
}
