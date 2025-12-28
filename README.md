настройка порта
```bash
stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts
```

отправка сообщения
```bash
echo "Hello from PC!" > /dev/ttyACM0
```