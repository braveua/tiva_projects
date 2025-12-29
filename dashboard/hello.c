#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h" 
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"
#include "font.h"
#include "lcd.h"

#define UART_BUF_SIZE 64

static char uart_buf[UART_BUF_SIZE];
static volatile uint8_t uart_pos = 0;
static volatile bool uart_line_ready = false;
/* LCD implementation moved to dashboard/lcd.c; public prototypes in dashboard/lcd.h */
/* ensure lcd.c is compiled or included when building single-file projects */
/* ===== Button pins (LaunchPad: PF4 = SW1, PF0 = SW2) ===== */
#define BTN_PORT GPIO_PORTF_BASE
#define BTN1_PIN GPIO_PIN_4
#define BTN2_PIN GPIO_PIN_0

/* ===== Forward declarations for new interrupt handlers and actions ===== */
// Called when physical button 1 is pressed (PB0)
static void button1_action(void);
// Called when physical button 2 is pressed (PB1)
static void button2_action(void);

// GPIO interrupt handler for Port F (buttons)
static void GPIOPortFIntHandler(void);

// UART interrupt handler
static void UART0IntHandler(void);

// Initialize buttons on Port B and enable GPIO interrupts
static void buttons_init(void) {
    // Configure PF4 and PF0 as inputs with internal pull-up resistors
    GPIOPinTypeGPIOInput(BTN_PORT, BTN1_PIN | BTN2_PIN);
    GPIOPadConfigSet(BTN_PORT, BTN1_PIN | BTN2_PIN,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Register GPIO Port F interrupt handler
    GPIOIntRegister(BTN_PORT, GPIOPortFIntHandler);

    // For PF0 (NMI), unlock commit register before configuring
    HWREG(BTN_PORT + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(BTN_PORT + GPIO_O_CR) |= BTN2_PIN;
    HWREG(BTN_PORT + GPIO_O_LOCK) = 0;

    // Configure to trigger on falling edge (button press -> line goes low)
    GPIOIntTypeSet(BTN_PORT, BTN1_PIN | BTN2_PIN, GPIO_FALLING_EDGE);

    // Clear any prior interrupts and enable
    GPIOIntClear(BTN_PORT, BTN1_PIN | BTN2_PIN);
    GPIOIntEnable(BTN_PORT, BTN1_PIN | BTN2_PIN);
    // Enable the NVIC interrupt for GPIOF so handler runs
    IntEnable(INT_GPIOF);

    // Debug mark that buttons_init completed
    UARTCharPutNonBlocking(UART0_BASE, 'I');
    UARTCharPutNonBlocking(UART0_BASE, 'N');
    UARTCharPutNonBlocking(UART0_BASE, 'I');
    UARTCharPutNonBlocking(UART0_BASE, '\n');
}
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

    // Register UART interrupt handler and enable RX interrupts
    UARTIntRegister(UART0_BASE, UART0IntHandler);
    // Enable receive and receive timeout interrupts
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}

// uart_poll removed; UART now handled via interrupts (UART0IntHandler)

/* LCD implementation moved to dashboard/lcd.c */

/* ===== MAIN ===== */
int main(void) {

    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ); // 50 MHz

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // Enable GPIOA
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // Enable GPIOB
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // Enable GPIOF for LaunchPad buttons
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);  // Enable SSI0

    GPIOPinConfigure(GPIO_PA0_U0RX); // Configure PA0 for UART0 RX
    GPIOPinConfigure(GPIO_PA1_U0TX); // Configure PA1 for UART0 TX

    /* initialize LCD pins (SSI pins + CS/DC/RST) */
    lcd_hw_init();

    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(),
                       SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 1000000, 8);
    SSIEnable(SSI0_BASE);

    uart_init();
    buttons_init();
    // Enable processor interrupts after peripheral configuration
    IntMasterEnable();
    lcd_init();
    lcd_clear();
    lcd_test_pattern();
    lcd_goto(0,0);
    // lcd_print("ПРИВЕТ СТАННЫЙ МИР! Hello World!");
    lcd_print("UART LCD READY");

    // Debug: report initial button pin states over UART (1 = high, 0 = low)
    {
        uint8_t st = GPIOPinRead(BTN_PORT, BTN1_PIN | BTN2_PIN);
        UARTCharPutNonBlocking(UART0_BASE, 'S');
        UARTCharPutNonBlocking(UART0_BASE, ':');
        UARTCharPutNonBlocking(UART0_BASE, (st & BTN1_PIN) ? '1' : '0');
        UARTCharPutNonBlocking(UART0_BASE, ',');
        UARTCharPutNonBlocking(UART0_BASE, (st & BTN2_PIN) ? '1' : '0');
        UARTCharPutNonBlocking(UART0_BASE, '\n');
    }

    uint8_t prev = GPIOPinRead(BTN_PORT, BTN1_PIN | BTN2_PIN) & (BTN1_PIN | BTN2_PIN);
    while (1) {
        // Poll button pins to detect hardware changes (debug)
        uint8_t now = GPIOPinRead(BTN_PORT, BTN1_PIN | BTN2_PIN) & (BTN1_PIN | BTN2_PIN);
        if (now != prev) {
            UARTCharPutNonBlocking(UART0_BASE, 'P');
            UARTCharPutNonBlocking(UART0_BASE, (now & BTN1_PIN) ? '1' : '0');
            UARTCharPutNonBlocking(UART0_BASE, (now & BTN2_PIN) ? '1' : '0');
            UARTCharPutNonBlocking(UART0_BASE, '\n');
            prev = now;
        }

        if (uart_line_ready) {
            uart_line_ready = false;
            lcd_print_line(uart_buf);
        }

        SysCtlDelay(SysCtlClockGet() / 20);
    }
}

/* ===== New interrupt handlers and button actions ===== */

// GPIO Port F interrupt handler: called on button press (falling edge).
// It determines which button caused the interrupt and calls the
// corresponding action function.
static void GPIOPortFIntHandler(void) {
    uint32_t status = GPIOIntStatus(BTN_PORT, true);

    // Clear the interrupts that we are handling
    GPIOIntClear(BTN_PORT, status);

    if (status & BTN1_PIN) {
        // Debug: notify over UART that BTN1 interrupt fired
        UARTCharPutNonBlocking(UART0_BASE, 'B');
        UARTCharPutNonBlocking(UART0_BASE, '1');
        UARTCharPutNonBlocking(UART0_BASE, '\n');
        button1_action();
    }
    if (status & BTN2_PIN) {
        // Debug: notify over UART that BTN2 interrupt fired
        UARTCharPutNonBlocking(UART0_BASE, 'B');
        UARTCharPutNonBlocking(UART0_BASE, '2');
        UARTCharPutNonBlocking(UART0_BASE, '\n');
        button2_action();
    }
}

// Simple action invoked on button 1 press.
// Here we send a short message over UART and update the LCD.
static void button1_action(void) {
    const char *msg = "Button1 pressed\n";
    for (const char *p = msg; *p; ++p) UARTCharPutNonBlocking(UART0_BASE, *p);
    lcd_print_line("Button1");
}

// Simple action invoked on button 2 press.
// Toggles a short message on the LCD and sends via UART.
static void button2_action(void) {
    const char *msg = "Button2 pressed\n";
    for (const char *p = msg; *p; ++p) UARTCharPutNonBlocking(UART0_BASE, *p);
    lcd_print_line("Button2");
}

// UART0 interrupt handler: reads incoming characters and assembles
// them into a line buffer similar to previous polling behavior.
// When a newline is received, it sets `uart_line_ready` for main loop.
static void UART0IntHandler(void) {
    uint32_t status = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts
    UARTIntClear(UART0_BASE, status);

    // Read all available characters
    while (UARTCharsAvail(UART0_BASE)) {
        char c = UARTCharGetNonBlocking(UART0_BASE);

        if (c == '\r') continue;

        if (c == '\n') {
            uart_buf[uart_pos] = 0;
            uart_pos = 0;
            uart_line_ready = true;
            // If needed, wake main thread or set an event here
            break;
        }

        if (uart_pos < UART_BUF_SIZE - 1) {
            uart_buf[uart_pos++] = c;
        }
    }
}
