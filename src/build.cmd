gcc server.c -o server -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_rs232_slave_server.c -o 32_device -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_tcp_master_client.c -o tcp-poll -Wall -std=c99 `pkg-config --libs --cflags libmodbus` 
