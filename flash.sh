openocd -f interface/ti-icdi.cfg -f target/stellaris.cfg -c "adapter speed 1000" -c "reset_config none" -c "program 5110.bin verify reset exit"
