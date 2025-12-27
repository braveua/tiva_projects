file=blinky
file=5110
file=hello
TW=/home/brave/ti/tivaware
opt1=" -mcpu=cortex-m4 -mthumb -Os -ffunction-sections  -fdata-sections -nostdlib"
opt2=" -mfloat-abi=hard -mfpu=fpv4-sp-d16"
#ls $TW/inc/hw_nvic.h
#echo $TW/inc
arm-none-eabi-gcc $opt1 -T blinky.ld -Wl,--gc-sections -Wl,-Map=$file.map -g -Wall -Wextra -o $file.elf $file.c startup_gcc.c -I $TW -L /home/brave/ti/tivaware/driverlib/gcc $opt2 -ldriver -lgcc -lc
arm-none-eabi-objcopy -O binary $file.elf $file.bin
#sudo openocd -f board/ek-lm4f120xl.cfg -c "program 5110.bin verify reset exit 0x0"
lm4flash $file.bin