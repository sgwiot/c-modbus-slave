TARGET=mbsimu
#TARGET=mbsimu zlibtest

CFLAGS=-std=gnu99 -O2 -D_GNU_SOURCE -DLOG_USE_COLOR -g
#CFLAGS=-D_REENTRANT  -I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/alsa-lib
#CFLAGS1=-D_REENTRANT  -I$(PKG_CONFIG_SYSROOT_DIR)/usr/include/libmad
LDFLAGS=-lpthread
#.PHONY all

all: $(TARGET)

mbsimu: log.o ini.o mbsimu.o
	$(CC) $(CFLAGS) -o $@ $^ `pkg-config --libs --cflags libmodbus` $(LDFLAGS)

mbsimu.o: src/mbsimu.c
	$(CC) -c $(CFLAGS) $(LDFLAGS) -o $@ $< `pkg-config --libs --cflags libmodbus`
ini.o: src/ini.c src/ini.h
	$(CC) -c $(CFLAGS) -o $@ $< `pkg-config --libs --cflags libmodbus`
log.o: src/log.c src/log.h
	$(CC) -c $(CFLAGS) -o $@ $< `pkg-config --libs --cflags libmodbus`

clean:
	rm -f $(OBJS) $(TARGET) log.o ini.o mbsimu.o
