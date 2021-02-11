gcc server.c -o server -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_rs232_slave_server.c -o 32_device -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_tcp_master_client.c -o tcp-poll -Wall -std=c99 `pkg-config --libs --cflags libmodbus` 
gcc tcp-slave.c -o tcp-slave -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus`
gcc ini_example.c ini.c  -o init_example
gcc ini.c ini_xmacros.c -o mac
gcc ini.c modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus` 
gcc ini.c modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus` -lpthread
gcc log.c ini.c modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus` -lpthread
gcc log.c ini.c modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus` -lpthread -DLOG_USE_COLOR
gdb --args ./tcp-slave-temp test.ini 
gcc log.c ini.c modbus_tcp_slave_server.c -o tcp-slave-temp -Wall -std=c99 `pkg-config --libs --cflags libmodbus` -lpthread -DLOG_USE_COLOR -g
gcc ini_xmacros.c ini.c log.c -o mac -std=c99 `pkg-config --libs --cflags libmodbus` -lpthread
./mac test.ini test1.ini test2.ini 
gcc ini_xmacros.c ini.c log.c -o mac -std=gnu99 `pkg-config --libs --cflags libmodbus` -lpthread
gcc ini_xmacros.c ini.c log.c -o mac -std=gnu99 `pkg-config --libs --cflags libmodbus` -lpthread
socat -d -d pty,raw,echo=0 pty,raw,echo=0
$CC ini_xmacros.c ini.c log.c -o mac -std=gnu99 `pkg-config --libs --cflags libmodbus` -lpthread
$CC ini_xmacros.c ini.c log.c -o mac -std=gnu99 `pkg-config --libs --cflags libmodbus` -lpthread -O2 -D_GNU_SOURCE
