#include <modbus.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <inttypes.h>
#include <stdlib.h>
#include <asm/ioctls.h>

#include "log.h"

#define UART_PATH "/dev/ttyUSB1"

/*
 * Modbus slave (server):
 *
 * This is a sample program using libModbus to communicate over a rs232 uart
 * using the modbus protocol.
 */

//Simple function that grab temperature on my laptop and copy it to the mapping
//structure from libmodbus ( this code is for test purpose ).
void update_temps(modbus_mapping_t *mb_mapping)
{
    FILE *f1,*f2;
    uint32_t t1,t2;
    //size_t s;

    f1 = fopen("/sys/class/thermal/thermal_zone0/temp","r");
    fscanf(f1,"%"PRIu32,&t1);
    t1 = t1/1000;
    mb_mapping->tab_registers[0] =(uint16_t) t1;

    f2 = fopen("/sys/class/thermal/thermal_zone1/temp","r");
    fscanf(f2,"%"PRIu32,&t2);
    t2 = t2/1000;
    mb_mapping->tab_registers[1] =(uint16_t) t2;

    fclose(f1);
    fclose(f2);
}

#include "ini.h"

/* define the config struct type */
typedef struct {
#define CFG(s, n, default) char *s##_##n;
#include "config.def"
} config;

/* create one and fill in its default values */
config Config = {
#define CFG(s, n, default) default,
#include "config.def"
};

config Configs[10] = {
#define CFG(s, n, default) default,
#include "config.def"
};

/* process a line of the INI file, storing valid values into config struct */
int handler(void *user, const char *section, const char *name,
        const char *value)
{
    config *cfg = (config *)user;

    if (0) ;
#define CFG(s, n, default) else if (strcmp(section, #s)==0 && \
        strcmp(name, #n)==0) cfg->s##_##n = strdup(value);
#include "config.def"

    return 1;
}

/* print all the variables in the config, one per line */
void dump_config(config *cfg)
{
    #define CFG(s, n, default) printf("%s_%s = %s\n", #s, #n, cfg->s##_##n);
    #include "config.def"
}

#include<pthread.h>
void *thread(void *para) {
    config *p = (config *)(para);
    //printf("type: %s\n", p->connection_type );
    log_trace("type: %s\n", p->connection_type );
    if (!strcmp("tcp", p->connection_type)) {
        printf("supported type:tcp\n");
    }
    else if (!strcmp("tcp", p->connection_type)) {
        printf("supported type:tcp\n");
    } else {
        printf("unsupport type!\n");
    }
    while(1) {
        sleep(1);
        printf("thread\n");
    }
}

int main(int argc, char *argv[]){
    int ret,rc;
    uint8_t *request;//Will contain internal libmodubs data from a request that must be given back to answer
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;

    if ( argc > 1 ) {
        printf("argc=%d\n", argc);
        for ( int j = 0; j < argc; j++ ) {
            printf("argv[%d]=%s\t", j, argv[j]);
        }
        puts("\n");
    }
    if ( argc > 2 ) {
        //parse all the ini files
        for ( int j = 1; j < argc; j++ ) {
            //printf("argv[%d]=%s\t", j, argv[j]);
            if (ini_parse( argv[j], handler, &(Configs[j]) ) < 0) {
                printf("Can't load 'test.ini', using defaults\n");
            } else {
                pthread_t thread_id;
                printf("before create pthread\n");
                pthread_create(&thread_id, NULL, thread, &Configs[j]);
                dump_config(&Config);
                printf("Connection is:%s\n", Config.connection_type);
            }
        }
    }

#if 0
    //ini import
    if (ini_parse("test.ini", handler, &Config) < 0)
        printf("Can't load 'test.ini', using defaults\n");
    dump_config(&Config);
    printf("Connection is:%s\n", Config.connection_type);
    //Config.connection
#endif

    //Set uart configuration and store it into the modbus context structure
    ctx = modbus_new_tcp("0.0.0.0", 5552);
    //ctx = modbus_new_tcp("192.168.199.245", 502);
    //ctx = modbus_new_rtu(UART_PATH, 115200, 'N', 8, 1);
    if (ctx == NULL) {
        perror("Unable to create the libmodbus context");
        return -1;
    }

    request= malloc(MODBUS_RTU_MAX_ADU_LENGTH);
    modbus_set_debug(ctx, TRUE);

    ret = modbus_set_slave(ctx, 1);//Set slave address
    if(ret < 0){
        perror("modbus_set_slave error");
        return -1;
    }

    //Works fine without it, I got a "Bad file descriptor" with it, likely
    //because my uart is RS232 only...
    /*	ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS232);
        if(ret < 0){
        perror("modbus_rtu_set_serial_mode error\n");
        return -1;
        }
        */
    //Modbus is configured, now it must opens the UART (even if a connexion
    //does not make sense in the modbus protocol.
#if 0
    ret = modbus_connect(ctx);
    if(ret < 0){
        perror("modbus_connect error");
        return -1;
    }
#endif

    //Init the modbus mapping structure, will contain the data
    //that will be read/write by a client.
    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0,
            MODBUS_MAX_READ_REGISTERS, 0);
    if(mb_mapping == NULL){
        perror("Cannot allocate mb_mapping");
        return -1;
    }

    //Wait for connection
    int s = modbus_tcp_listen(ctx, 1);
    modbus_tcp_accept(ctx, &s);
    int i = 0;

    for(;;){
        do {
            rc = modbus_receive(ctx, request);
        } while (rc == 0);
        if(rc < 0){
            perror("Error in modbus receive");
            //	return -1;
        }

        printf("SLAVE: regs[] =\t");
        for(i = 1; i != 11; i++) { // looks like 1..n index
            printf("%d ", mb_mapping->tab_registers[i]);
        }
        printf("\n");
        printf("Request received rc= %d\n",rc);

        update_temps(mb_mapping);
        ret = modbus_reply(ctx,request,rc,mb_mapping);//rc, request size must be given back to modbus_reply as well as "request" data
        if(ret < 0){
            perror("modbus reply error");
            return -1;
        }
    }

    //Clean up everything:
    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
