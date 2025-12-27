TW=/home/brave/ti/tivaware
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Os -ffunction-sections  -fdata-sections -nostdlib -T blinky.ld -Wl,--gc-sections -Wl,-Map=blinky.map -g -Wall -Wextra -o blinky.elf blinky.c startup_gcc.c -I $TW -L /home/brave/ti/tivaware/driverlib/gcc -ldriver -lgcc -lc
arm-none-eabi-objcopy -O binary blinky.elf blinky.bin

