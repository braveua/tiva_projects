настройка порта
stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts
отправка сообщения
echo "Hello from PC!" > /dev/ttyACM0
