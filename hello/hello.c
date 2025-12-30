#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

#define RED   0x02
#define GREEN 0x08
#define BLUE  0x04


int main(void)
{
    volatile unsigned long ulLoop;
    
    // Enable the GPIO port that is used for the on-board LED.
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;

    // Do a dummy read to insert a few cycles after enabling the peripheral.
    ulLoop = SYSCTL_RCGC2_R;

    // Enable the GPIO pin for the LED (PF3).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    GPIO_PORTF_DIR_R = 0x0E;
    GPIO_PORTF_DEN_R = 0x0E;

    // Loop forever.
    while(1)
    {
        // Turn on the LED.
        GPIO_PORTF_DATA_R |= BLUE+RED;

        // Delay for a bit.
        for(ulLoop = 0; ulLoop < 200000; ulLoop++)
        {
        }

        // Turn off the LED.
        // GPIO_PORTF_DATA_R &= ~(BLUE+RED);

        // Delay for a bit.
        for(ulLoop = 0; ulLoop < 200000; ulLoop++)
        {
        }
    }
}
