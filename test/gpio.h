#ifndef TEST_GPIO_H
#define TEST_GPIO_H

#include <stdint.h>

/* Инициализация RGB-LED на PF1..PF3 */
void init_leds(void);
/* Инициализация кнопок SW1(SWITCH1=PF4) и SW2(SWITCH2=PF0) */
void init_buttons(void);
/* Чтение состояния кнопок: возвращают 1 если нажата (active-low) */
int read_sw1(void);
int read_sw2(void);

#endif /* TEST_GPIO_H */
