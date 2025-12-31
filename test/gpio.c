#include "gpio.h"
#include "tm4c123gh6pm.h"

/* Инициализация светодиодов (PF1..PF3) */
void init_leds(void) {
    /* Включаем тактирование GPIOF */
    SYSCTL_RCGCGPIO_R |= (1U << 5);
    while ((SYSCTL_PRGPIO_R & (1U << 5)) == 0) {}

    GPIO_PORTF_DIR_R |= (1U << 1) | (1U << 2) | (1U << 3);
    GPIO_PORTF_DEN_R |= (1U << 1) | (1U << 2) | (1U << 3);
}

/* Инициализация кнопок SW1=PF4, SW2=PF0 */
void init_buttons(void) {
    /* Разблокируем PF0 для записи в CR */
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= (1U << 0);

    /* Входы */
    GPIO_PORTF_DIR_R &= ~((1U << 0) | (1U << 4));
    /* Включаем подтяжку вверх */
    GPIO_PORTF_PUR_R |= (1U << 0) | (1U << 4);
    /* Разрешаем цифровые функции */
    GPIO_PORTF_DEN_R |= (1U << 0) | (1U << 4);
}

/* Возвращают 1 если кнопка нажата (active-low) */
int read_sw1(void) {
    return ((GPIO_PORTF_DATA_R & (1U << 4)) == 0) ? 1 : 0;
}

int read_sw2(void) {
    return ((GPIO_PORTF_DATA_R & (1U << 0)) == 0) ? 1 : 0;
}
