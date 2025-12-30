#include <stdint.h>          // базовые типы (uint32_t и т.п.)
#include "inc/hw_memmap.h"   // базовый адресный слой памяти
#include "inc/hw_sysctl.h"    // определения регистров SysCtl (включая SYSCTL_RCGC2_R)
#include "inc/tm4c123gh6pm.h" // полные определения всех регистров конкретного чипа
/* либо, если используете драйверную библиотеку TivaWare */
#include "driverlib/sysctl.h"




void ButtonsInit(){
    // Configure pushbutton pins as input
    SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // Enable clock for GPIO Port F
    GPIO_PORTF_DIR_R &= ~(0x11); // Set PF0 and PF4 as input
    GPIO_PORTF_DEN_R |= 0x11;    // Enable digital function for PF0 and PF4
    // Enable internal pull-up resistors if necessary
    GPIO_PORTF_PUR_R |= 0x11;    // Enable internal pull-up resistors for PF0 and PF4    
    while(1==1){
    // Если нажата кнопка 1 - PF4
    if ((GPIO_PORTF_DATA_R & 0x10) == 0) {
        // Зажигаем светодиод на PF1
        GPIO_PORTF_DATA_R |= 0x02; // Set PF1 high
    }
    }
}
int main(void){
    ButtonsInit();
    // UART_Init();
    // LCD_Init();
}