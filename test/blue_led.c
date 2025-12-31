/*
  blue_led.c
  Простая программа для TM4C123 (Tiva LaunchPad) — зажигает синий светодиод (PF2).
*/

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"

/* Инициализация светодиодов (PF1..PF3 — RGB, PF2 — синий) */
static void init_leds(void) {
  /* Включаем тактирование GPIOF */
  SYSCTL_RCGCGPIO_R |= (1U << 5);
  /* Ждём готовности порта F */
  while ((SYSCTL_PRGPIO_R & (1U << 5)) == 0) {}

  /* PF1, PF2, PF3 как выходы */
  GPIO_PORTF_DIR_R |= (1U << 1) | (1U << 2) | (1U << 3);
  /* Цифровые функции для PF1..PF3 */
  GPIO_PORTF_DEN_R |= (1U << 1) | (1U << 2) | (1U << 3);
}

/* Инициализация кнопок (PF0 и PF4) */
static void init_buttons(void) {
  /* PF0 защищённый пин — нужно разблокировать для записи регистра CR */
  GPIO_PORTF_LOCK_R = 0x4C4F434B; /* ключ разблокировки */
  GPIO_PORTF_CR_R |= (1U << 0);

  /* Устанавливаем PF0 и PF4 как входы */
  GPIO_PORTF_DIR_R &= ~((1U << 0) | (1U << 4));
  /* Включаем подтяжку вверх (pull-up) для кнопок */
  GPIO_PORTF_PUR_R |= (1U << 0) | (1U << 4);
  /* Включаем цифровой режим для PF0 и PF4 */
  GPIO_PORTF_DEN_R |= (1U << 0) | (1U << 4);
}

int main(void) {
  init_leds();
  init_buttons();

  /* Включаем синий светодиод (PF2) */
  GPIO_PORTF_DATA_R |= (1U << 2);

  /* Простейшая логика: при нажатии SW1 или SW2 переключаем PF2 */
  for (;;) {
    if (read_sw1() || read_sw2()) {
      /* Антидребезг */
      for (volatile int i = 0; i < 30000; ++i) { __asm__("nop"); }
      if (read_sw1() || read_sw2()) {
        /* Ждём отпускания любой кнопки */
        while (read_sw1() || read_sw2()) {}
        GPIO_PORTF_DATA_R ^= (1U << 2);
      }
    }
  }
}
