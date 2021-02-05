gcc server.c -o server -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_rs232_slave_server.c -o 32_device -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_tcp_master_client.c -o tcp-poll -Wall -std=c99 `pkg-config --libs --cflags libmodbus` 
gcc tcp-slave.c -o tcp-slave -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc ini_example.c ini.c  -o init_example
gcc ini.c ini_xmacros.c -o mac
gcc ini.c modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus` 
