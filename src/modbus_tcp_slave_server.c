#include <modbus.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <inttypes.h>
#include <stdlib.h>
#include <asm/ioctls.h>
#include <unistd.h>
#include <string.h>

#include "log.h"
#define LOG_SILIENCE FALSE

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
#define MAX_THREAD 20
config Configs[MAX_THREAD] = {
    #define CFG(s, n, default) default,
    #include "config.def"
};

/* process a line of the INI file, storing valid values into config struct */
int handler(void *user, const char *section, const char *name, const char *value) {
    config *cfg = (config *)user;

    if (0) ;
#define CFG(s, n, default) else if (strcmp(section, #s)==0 && \
        strcmp(name, #n)==0) cfg->s##_##n = strdup(value);
#include "config.def"

    return 1;
}

/* print all the variables in the config, one per line */
void dump_config(config *cfg) {
    #define CFG(s, n, default) log_trace("%s_%s = %s\n", #s, #n, cfg->s##_##n);
    #include "config.def"
}

#define TCP 1
#define RTU 2
#include<pthread.h>
void *thread(void *para) {
    config *p = (config *)(para);
    int type = -1;
    //log_trace("type: %s\n", p->connection_type );
    log_trace("type: %s\n", p->connection_type );
    if (!strcmp("tcp", p->connection_type)) {
        log_trace("supported type:tcp\n");
        type = TCP;
    }
    else if (!strcmp("rtu", p->connection_type)) {
        log_trace("supported type:rtu\n");
        type = RTU;
    } else {
        log_error("unsupport type!\n");
        //ToDo: finish thread back  to main
    }
    while(1) {
        log_trace("beging loop\n");
        int ret,rc;
        uint8_t *request;//Will contain internal libmodubs data from a request that must be given back to answer
        modbus_t *ctx;
        modbus_mapping_t *mb_mapping;
        int s = -1;

        //Init the modbus mapping structure, will contain the data
        //that will be read/write by a client.
        mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0, MODBUS_MAX_READ_REGISTERS, 0);
        if(mb_mapping == NULL){
            log_error("Cannot allocate mb_mapping");
            //return -1;
        }

        if (type == TCP) {
            log_trace("beging loop:TCP\n");
            //Set uart configuration and store it into the modbus context structure
            int port = atoi(p->connection_port);
            ctx = modbus_new_tcp("0.0.0.0", port);
            //ctx = modbus_new_tcp("192.168.199.245", 502);
            //ctx = modbus_new_rtu(UART_PATH, 115200, 'N', 8, 1);
            if (ctx == NULL) {
                log_error("Unable to create the libmodbus context");
                //return -1;
            }

            request = malloc(MODBUS_TCP_MAX_ADU_LENGTH * 2);
            //modbus_set_debug(ctx, TRUE);
            //modbus_set_debug(ctx, TRUE);

            int slaveid = atoi(p->connection_slaveid);
            ret = modbus_set_slave(ctx, slaveid);//Set slave address
            if(ret < 0){
                log_error("modbus_set_slave error");
                //return -1;
            }
            //Wait for connection
            s = modbus_tcp_listen(ctx, 1);
            modbus_tcp_accept(ctx, &s);
        }
        //Set uart configuration and store it into the modbus context structure
        if ( type == RTU ) {
            log_trace("beging loop:TCP\n");
            //ctx = modbus_new_rtu(p->connection_port, p->connection_baud, 'N', 8, 1);
            int baud = atoi(p->connection_baud);
            int databit = atoi(p->connection_databit);
            int stopbit = atoi(p->connection_stopbit);
            char check = 'N';
            log_error("UART checking is:%s", p->connection_check);
            if(! strcmp("N", p->connection_check)) {
                check = 'N';
            } else if(! strcmp("O", p->connection_check)) {
                check = 'O';
            } else if(! strcmp("E", p->connection_check)) {
                check = 'E';
            } else {
                log_error("UART checking should be in N/E/O !");
            }

            ctx = modbus_new_rtu(p->connection_port, baud, check, databit, stopbit);
            if (ctx == NULL) {
                log_error("Unable to create the libmodbus context");
                //return -1;
            }

            request= malloc(MODBUS_RTU_MAX_ADU_LENGTH * 2);
            if(NULL == request) {
                log_error("malloc Error");
            } else {
                log_trace("malloc OK");
            }
            modbus_set_debug(ctx, TRUE);

            int slaveid = atoi(p->connection_slaveid);
            ret = modbus_set_slave(ctx, slaveid);//Set slave address
            if(ret < 0){
                log_error("modbus_set_slave error");
                //return -1;
            }
            {
                //Works fine without it, I got a "Bad file descriptor" with it, likely
                //because my uart is RS232 only...
                if ( ! strcmp("1", p->connection_rs232) ) {
                    ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS232);
                } else {
                    ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
                }
                if(ret < 0){
                    log_error("modbus_rtu_set_serial_mode error\n");
                    //return -1;
                }
                //Modbus is configured, now it must opens the UART (even if a connexion
                //does not make sense in the modbus protocol.

            }
            //Set debug log data
            if (strcmp("0", p->connection_datadebug)) {
                modbus_set_debug(ctx, TRUE);
            }
            ret = modbus_connect(ctx);
            if(ret < 0){
                log_error("modbus_connect error");
                //return -1;
            }
        }

        while(1) {
#if 1
            do {
                log_trace("modbus_receive begin");
                rc = modbus_receive(ctx, request);
            } while (rc == 0);
#endif

#if 0
            log_trace("modbus_receive begin");
            //sleep(1);
            rc = modbus_receive(ctx, request);
#endif

            if(rc < 0){
                log_error("Error in modbus receive");
                //ToDo
                //	return -1;
            }

            log_trace("SLAVE: regs[] =\t");
            for(int i = 1; i != 11; i++) { // looks like 1..n index
                log_trace("%d ", mb_mapping->tab_registers[i]);
            }
            log_trace("\n");
            log_trace("Request received rc= %d\n",rc);

            update_temps(mb_mapping);
            ret = modbus_reply(ctx,request,rc,mb_mapping);//rc, request size must be given back to modbus_reply as well as "request" data
            if(ret < 0){
                log_error("modbus reply error");
                //return -1;
            }
        }

        //Clean up everything:
        modbus_mapping_free(mb_mapping);
        modbus_close(ctx);
        modbus_free(ctx);
    }
}

int main(int argc, char *argv[]){
    log_set_quiet(LOG_SILIENCE);

    if ( argc > 1 ) {
        //parse all the ini files
        for ( int j = 1; j < argc; j++ ) {
            //log_trace("argv[%d]=%s\t", j, argv[j]);
            if (ini_parse( argv[j], handler, &(Configs[j]) ) < 0) {
                log_trace("Can't load 'test.ini', using defaults\n");
            } else {
                pthread_t thread_id[MAX_THREAD];
                log_trace("before create pthread\n");
                pthread_create(&(thread_id[j-1]), NULL, thread, &Configs[j-1]);
                dump_config(&Config);
                log_trace("Connection is:%s\n", Config.connection_type);
            }
        }
    }

    while(1) {
        sleep(1);
    }

    //phread_join
    return 0;
}
