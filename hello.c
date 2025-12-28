#include <stdint.h>
#include <stdbool.h>
// #include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h" 
#include "driverlib/uart.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "font.h"

/* ===== LCD pins ===== */
#define LCD_CS_PORT  GPIO_PORTA_BASE
#define LCD_CS_PIN   GPIO_PIN_6

#define LCD_DC_PORT  GPIO_PORTA_BASE
#define LCD_DC_PIN   GPIO_PIN_7

#define LCD_RST_PORT GPIO_PORTB_BASE
#define LCD_RST_PIN  GPIO_PIN_2

#define CS_LOW()   GPIOPinWrite(LCD_CS_PORT, LCD_CS_PIN, 0)
#define CS_HIGH()  GPIOPinWrite(LCD_CS_PORT, LCD_CS_PIN, LCD_CS_PIN)
#define DC_CMD()   GPIOPinWrite(LCD_DC_PORT, LCD_DC_PIN, 0)
#define DC_DATA()  GPIOPinWrite(LCD_DC_PORT, LCD_DC_PIN, LCD_DC_PIN)
#define RST_LOW()  GPIOPinWrite(LCD_RST_PORT, LCD_RST_PIN, 0)
#define RST_HIGH() GPIOPinWrite(LCD_RST_PORT, LCD_RST_PIN, LCD_RST_PIN)

#define UART_BUF_SIZE 64

static char uart_buf[UART_BUF_SIZE];
static volatile uint8_t uart_pos = 0;
static volatile bool uart_line_ready = false;

/* ===== LCD API prototypes ===== */
// static void lcd_hw_init(void);
static void lcd_init(void);
static void lcd_goto(uint8_t x, uint8_t y);
static void lcd_print(const char *s);
static void lcd_clear(void);
/* ===== UART ===== */
static void uart_init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTConfigSetExpClk(UART0_BASE,
                        SysCtlClockGet(),
                        115200,
                        UART_CONFIG_WLEN_8 |
                        UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE);

    UARTEnable(UART0_BASE);
}

static void uart_poll(void) {
    while (UARTCharsAvail(UART0_BASE)) {
        char c = UARTCharGetNonBlocking(UART0_BASE);

        if (c == '\r') continue;

        if (c == '\n') {
            uart_buf[uart_pos] = 0;
            uart_pos = 0;
            uart_line_ready = true;
            return;
        }

        if (uart_pos < UART_BUF_SIZE - 1) {
            uart_buf[uart_pos++] = c;
        }
    }
}

static uint8_t lcd_row = 0;

static void lcd_print_line(const char *s) {
    lcd_goto(0, lcd_row);
    lcd_print(s);

    if (++lcd_row >= 6) {
        lcd_row = 0;
        lcd_clear();
    }
}

/* ===== SPI ===== */
static void spi_send(uint8_t data) {
    uint32_t dummy;
    SSIDataPut(SSI0_BASE, data);
    SSIDataGet(SSI0_BASE, &dummy);
}

/* ===== LCD ===== */
static void lcd_cmd(uint8_t c) {
    DC_CMD();
    CS_LOW();
    spi_send(c);
    CS_HIGH();
}

static void lcd_data(uint8_t d) {
    DC_DATA();
    CS_LOW();
    spi_send(d);
    CS_HIGH();
}

static void lcd_init(void) {
    RST_LOW();
    SysCtlDelay(SysCtlClockGet()/100);
    RST_HIGH();

    lcd_cmd(0x21);
    lcd_cmd(0x80 | 0x3E);   // contrast
    lcd_cmd(0x13);
    lcd_cmd(0x06);
    lcd_cmd(0x20);
    lcd_cmd(0x0C);
}

static void lcd_goto(uint8_t x, uint8_t y) {
    lcd_cmd(0x80 | x);
    lcd_cmd(0x40 | y);
}

static void lcd_clear(void) {
    lcd_goto(0,0);
    for (int i=0;i<504;i++)
        lcd_data(0x00);
}

static void lcd_char(uint8_t c) {
    for (int i=0;i<5;i++)
        lcd_data(ASCII[c][i]);
    lcd_data(0x00);
}

/* ===== UTF-8 print (исправленный) ===== */
static void lcd_print(const char *s) {
    const uint8_t *p = (const uint8_t *)s;

    while (*p) {
        uint8_t c = *p++;

        if (c < 0x80) lcd_char(c);
        else if (c == 0xD0) {
            uint8_t c2 = *p++;
            if (c2 == 0x81) lcd_char(0xC0);
            else if (c2 >= 0x90 && c2 <= 0x9F)
                lcd_char(0x80 + (c2 - 0x90));
            else if (c2 >= 0xA0 && c2 <= 0xAF)
                lcd_char(0x90 + (c2 - 0xA0));
        }
        else if (c == 0xD1) {
            uint8_t c2 = *p++;
            if (c2 == 0x91) lcd_char(0xC1);
            else if (c2 >= 0x80 && c2 <= 0x8F)
                lcd_char(0xA0 + (c2 - 0x80));
            else if (c2 >= 0x90 && c2 <= 0x9F)
                lcd_char(0xB0 + (c2 - 0x90));
        }
    }
}

/* ===== MAIN ===== */
int main(void) {

    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_5);
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_2);

    CS_HIGH();
    RST_HIGH();

    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(),
                       SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 1000000, 8);
    SSIEnable(SSI0_BASE);

    uart_init();
    // lcd_hw_init();     // твой SPI + GPIO init
    lcd_init();
    lcd_clear();
    lcd_goto(0,0);
    // lcd_print("ПРИВЕТ СТАННЫЙ МИР! Hello World!");
    // lcd_print("0123456789012356");
    lcd_print("UART LCD READY");

    while (1) {
        uart_poll();

        if (uart_line_ready) {
            uart_line_ready = false;
            lcd_print_line(uart_buf);
        }
    }
}
