//For pthread_setname_np
//#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
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

#include<pthread.h>

#include "ini.h"
#include "log.h"

#include <errno.h>

int header_length;
#define LOG_SILIENCE FALSE

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

void update_holding(modbus_mapping_t *mb_mapping) {

    mb_mapping->tab_registers[8] = 0x99;
    mb_mapping->tab_registers[9] = 0x88;
}

void update_temps(modbus_mapping_t *mb_mapping) {
    FILE *f1,*f2;
    uint32_t t1,t2;
    //size_t s;
    int ret = -1;

#define TEMP_0 1
#define TEMP_1 2
    f1 = fopen("/sys/class/thermal/thermal_zone0/temp","r");
    ret = fscanf(f1,"%"PRIu32,&t1);
    t1 = t1/1000;
    mb_mapping->tab_registers[TEMP_0] =(uint16_t) t1;

    //iMX6 has no zone1
#if 0
    /sys/bus/cpu/devices/cpu0/cpufreq/cpuinfo_cur_freq
    f2 = fopen("/sys/class/thermal/thermal_zone1/temp","r");
    ret = fscanf(f2,"%"PRIu32,&t2);
    t2 = t2/1000;
    mb_mapping->tab_registers[TEMP_1] =(uint16_t) t2;
#endif

    //imx6ul cpu freq
    f2 = fopen("/sys/bus/cpu/devices/cpu0/cpufreq/cpuinfo_cur_freq","r");
    ret = fscanf(f2,"%"PRIu32,&t2);
    t2 = t2/1000;
    mb_mapping->tab_registers[TEMP_1] =(uint16_t) t2;
    //mb_mapping->tab_registers[TEMP_1] =(uint16_t) t1;

    fclose(f1);
    fclose(f2);
}

int model_handler(modbus_mapping_t *mb_mapping, uint8_t *request, int request_len, int header_length) {
    log_info("model handler:header_length:%d, request_len:%d type:%d", header_length, request_len, request[header_length]);
    //Reading holding register
    static unsigned int temp;
    static unsigned int humi;
    if(0 < humi && humi < 1000) {
        humi+=5;
    } else {
        humi = 500;
    }

    if(0 < temp && temp < 100) {
        temp++;
    } else {
        temp = 50;
    }
    if(0x03 == request[header_length]) {
        //Reading the temperature, starting address 0x00
        if(0x00 == request[2 + header_length] && 0x00 == request[1 + header_length]) {
            log_info("Master read 03:00 holding registger, simu temp value");
            //Temp
            //mb_mapping->tab_registers[0] = 0x292;
            //mb_mapping->tab_registers[1] = 0xff9b;
            mb_mapping->tab_registers[0] = humi;
            mb_mapping->tab_registers[1] = temp;
            //mb_mapping->tab_registers[1] = 0x2;
            //H
            mb_mapping->tab_registers[2] = 0xff;
            mb_mapping->tab_registers[3] = 0x9b;
        }
    }
}

#define TCP 1
#define RTU 2

/*********************************************************************
 * Set thread name
 *********************************************************************/
//To large would fail, such as 22, so we use 19
//Connected: 255rtuttyUSB127-con , max 26
#define MAX_P_NAME 19
int set_phtread_name(char *port, char* slaveid, char* type, int type_tcp_rtu, int connected, char* thread_name) {
    int ret = -1;
    /*********************************************************************
     * Set thread name: connected
     *********************************************************************/
    char name[MAX_P_NAME] = {0};
    char *device_path = strdup(port);
    if(device_path == NULL) {
        log_error("strdup error for device path");
    }
    //char str[] ="1,2,3,4,5";
    char *pt;
    if (type_tcp_rtu == RTU) {
        pt = strtok (device_path,"/");
        while (pt != NULL) {
            //int a = atoi(pt);
            //printf("%d\n", a);
            if(connected) {
                snprintf(name, MAX_P_NAME, "%s%s%s-con", slaveid, type, pt);
            } else {
                snprintf(name, MAX_P_NAME, "%s%s%s", slaveid, type, pt);
            }
            pt = strtok (NULL, "/");
        }
    } else {
            if(connected) {
                snprintf(name, MAX_P_NAME, "%s%s%s-con", slaveid, type, port);
            } else {
                snprintf(name, MAX_P_NAME, "%s%s%s", slaveid, type, port);
            }
    }
	ret = pthread_setname_np(pthread_self(), name);
    //int err = errno;
	if (ret == 0) {
		//log_trace("Thread set name success, tid=%llu", pthread_self());
		log_trace("Thread set name(%s) success", name);
        ret = 0;
    } else {
        log_error("Thread set name Error:%s, errno:%d!!", name, ret);
        ret = -1;
    }
    memcpy(thread_name, name, MAX_P_NAME);
    free(device_path);
    return ret;
}

void *thread(void *para) {
    config *p = (config *)(para);

#if 0
    printf("------thread---------\n");
    printf("type: %s\n", p->connection_type);
    printf("port: %s\n", p->connection_port);
    printf("slaveid: %s\n", p->connection_slaveid);
    printf("---------------\n");
#endif
    /*********************************************************************
    * get type
    *********************************************************************/
    int type = -1;
    //log_trace("type: %s\n", p->connection_type );
    log_trace("type: %s", p->connection_type );
    if (!strcmp("tcp", p->connection_type)) {
        log_trace("supported type:tcp");
        type = TCP;
    } else if (!strcmp("rtu", p->connection_type)) {
        log_trace("supported type:rtu");
        type = RTU;
    } else {
        log_error("unsupport type!");
        goto THREAD_EXIT;
    }

    /*********************************************************************
    * new mapping
    *********************************************************************/
    uint8_t *request = NULL;//Will contain internal libmodubs data from a request that must be given back to answer
    modbus_t *ctx = NULL;
    modbus_mapping_t *mb_mapping = NULL;
    int socket = -1;
    //Will be used in reconnection
    int port = 502;
    int slaveid = -1;
    int ret,rc;
    char name[MAX_P_NAME] = {0};
RECONNECTION:
    rc = set_phtread_name(p->connection_port, p->connection_slaveid, p->connection_type, type, 0, name);
    if(rc < 0) {
        log_error("Set pthread name:name failed!", name);
    } else {
        log_info("Set pthread name:name OK", name);
    }
    //log_trace("beging loop:%s", name);
    if (type == TCP) {
        log_trace("Before new ctx...");
        port = atoi(p->connection_port);
        ctx = modbus_new_tcp("0.0.0.0", port);
        //ctx = modbus_new_tcp_pi("0.0.0.0", port);
        //ctx = modbus_new_tcp_pi("0.0.0.0", p->connection_port);
        if (ctx == NULL) {
            log_error("Unable to create the libmodbus context:%s", strerror( errno ));
            goto THREAD_EXIT;
        }

        if(NULL == request) {
            request = malloc(MODBUS_TCP_MAX_ADU_LENGTH);
            if(NULL == request) {
                log_error("TCP malloc Error");
                goto THREAD_EXIT;
            } else {
                log_trace("malloc OK");
            }
        }

        slaveid = atoi(p->connection_slaveid);
        ret = modbus_set_slave(ctx, slaveid);//Set slave address
        if(ret < 0){
            log_error("modbus_set_slave error");
            goto THREAD_EXIT;
        }  else {
            log_info("modbus_set_slave:%d OK", slaveid);
        }
        if(NULL == mb_mapping) {
            //Init the modbus mapping structure, will contain the data
            //that will be read/write by a client.
            mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0, MODBUS_MAX_READ_REGISTERS, 0);
            //mb_mapping = modbus_mapping_new(500, 500, 500, 500);
            if(mb_mapping == NULL){
                log_error("Cannot allocate mb_mapping");
                goto THREAD_EXIT;
            } else {
                log_info("%d mapping new OK", slaveid);
            }
        }
        //Wait for connection
        socket  = modbus_tcp_listen(ctx, 1);
        if(-1 == socket) {
            log_error("tcp listen port:%d error:%d [%s]", port, errno, strerror( errno ));
            goto THREAD_EXIT;
        } else {
            log_info("%d tcp listen OK", slaveid);
        }

        int socket_connected = modbus_tcp_accept(ctx, &socket);
        if( socket_connected == -1 ) {
            log_error("tcp_accept error!!");
        } else {
            log_info("tcp connected");
        }
    }
    //Set uart configuration and store it into the modbus context structure
    if ( type == RTU ) {
        //ctx = modbus_new_rtu(p->connection_port, p->connection_baud, 'N', 8, 1);
        int baud = atoi(p->connection_baud);
        int databit = atoi(p->connection_databit);
        int stopbit = atoi(p->connection_stopbit);
        char check = 'N';
        log_trace("UART checking is:%s", p->connection_check);
        if(! strcmp("N", p->connection_check)) {
            check = 'N';
        } else if(! strcmp("O", p->connection_check)) {
            check = 'O';
        } else if(! strcmp("E", p->connection_check)) {
            check = 'E';
        } else {
            log_error("UART checking should be in N/E/O !");
            goto THREAD_EXIT;
        }

        ctx = modbus_new_rtu(p->connection_port, baud, check, databit, stopbit);
        if (ctx == NULL) {
            log_error("Unable to create the libmodbus context");
            goto THREAD_EXIT;
        }

        //request= malloc(MODBUS_RTU_MAX_ADU_LENGTH * 2);
        request= malloc(MODBUS_RTU_MAX_ADU_LENGTH);
        if(NULL == request) {
            log_error("RTU malloc Error");
        } else {
            log_trace("RTU malloc OK");
        }
        modbus_set_debug(ctx, TRUE);

        slaveid = atoi(p->connection_slaveid);
        ret = modbus_set_slave(ctx, slaveid);//Set slave address
        if(ret < 0){
            log_error("modbus_set_slave error");
            goto THREAD_EXIT;
        } else {
            log_info("modbus_set_slave:%d OK", slaveid);
        }
        //Init the modbus mapping structure, will contain the data
        //that will be read/write by a client.
        mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0, MODBUS_MAX_READ_REGISTERS, 0);
        //mb_mapping = modbus_mapping_new(500, 500, 500, 500);
        if(mb_mapping == NULL){
            log_error("Cannot allocate mb_mapping");
            goto THREAD_EXIT;
        } else {
            log_info("mb_mapping OK");
        }
        //HeXiongjun: This is a must, otherwise would show timeout error : ERROR Connection timed out: select
        //  Ref:
        //      https://github.com/stephane/libmodbus/issues/382
        //      https://stackoverflow.com/questions/34387820/multiple-rs485-slaves-with-libmodbus
        //usleep(0.005 * 1000000);
        sleep(1);
        ret = modbus_connect(ctx);
        if(ret < 0){
            log_error("modbus_connect error");
            goto THREAD_EXIT;
        } else {
            log_trace("Connected OK");
        }
        {
            //Works fine without it, I got a "Bad file descriptor" with it, likely
            //because my uart is RS232 only...
            if ( !strcmp("1", p->connection_rs232) ) {
                //ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS232);
                log_trace("RTU is RS232 Mode, ignore the setting");
            } else {
                ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);
                log_trace("RTU RS485 Mode Set");
            }
            if(ret < 0){
                int errnum = errno;
                log_error("modbus_rtu_set_serial_mode error :%d, error string:%s", errno, strerror( errnum ));
#if 0
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
                fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
#endif
                //log_trace("Error:%d, error string:%s", errno, strerror( errnum ));

                //ToBeVerify: When is RS232, this may fail, but still working
                goto THREAD_EXIT;
            }
        }
    }
    //Set debug log data
    if (strcmp("1", p->connection_datadebug)) {
        modbus_set_debug(ctx, TRUE);
        log_trace("Enable Debug Mode");
    } else if(strcmp("0", p->connection_datadebug)){
        modbus_set_debug(ctx, FALSE);
        log_trace("Disable Debug Mode");
    } else {
        log_error("Invalid Debug Mode config, set to 0 or 1, will set to true as default !!");
        modbus_set_debug(ctx, TRUE);
    }
        modbus_set_debug(ctx, TRUE);
    /*********************************************************************
     * Set thread name: connected
     *********************************************************************/
    set_phtread_name(p->connection_port, p->connection_slaveid, p->connection_type, type, 1, name);

    header_length = modbus_get_header_length(ctx);
    log_trace("header_length:%d", header_length);
    ret = modbus_set_error_recovery(ctx,
                          MODBUS_ERROR_RECOVERY_LINK |
                          MODBUS_ERROR_RECOVERY_PROTOCOL);
    if ( ret ) {
        log_error("set_error_recovery failed:%d\n", ret);
    }
    sleep(1);
    uint8_t count = 0;
    while(1) {
        count ++;
        //log_trace("modbus_receive begin");
        rc = modbus_receive(ctx, request);
        //log_trace("received rc:%d", rc);


        //Peer Disconnected
        if(rc == -1){
            log_error("Error in modbus receive:%d, errno:%d:[%s]", rc, errno, strerror( errno ));
            ret = modbus_set_error_recovery(ctx,
                    MODBUS_ERROR_RECOVERY_LINK |
                    MODBUS_ERROR_RECOVERY_PROTOCOL);
            if ( ret ) {
                log_error("set_error_recovery failed:%d\n", ret);
            }
            if(type == TCP) {
                modbus_flush(ctx);
                log_error("TCP need to be restart the connection...");
                close(socket);
                modbus_close(ctx);
                modbus_free(ctx);
                goto RECONNECTION;
            } else if(type == RTU){
                set_phtread_name(p->connection_port, p->connection_slaveid, p->connection_type, type, 0, name);
            } else {

            }
            //goto THREAD_EXIT;
        }

#if 1
        printf("Hex tab_regs[]=\t");
        //for(int i = 1; i < 9; i++) { // looks like 1..n index
        for(int i = 0; i < 20; i++) { // looks like 1..n index
            printf("%02x ", mb_mapping->tab_registers[i]);
        }
        printf("\n");
#endif
#if 0
        mb_mapping->tab_registers[0] = count;
        mb_mapping->tab_registers[1] = count;
        mb_mapping->tab_registers[2] = count;
        mb_mapping->tab_registers[3] = count;
        update_holding(mb_mapping);
#endif
        mb_mapping->tab_registers[4] = count;
#if 1
        update_temps(mb_mapping);
#endif
        printf("%s: request received length=%d\t",name, rc);
        printf("Hex request[] =\t");
        for(int i = 0; i < rc; i++) { // looks like 1..n index
            printf("%02x ", request[i]);
        }
        printf("\n");

        model_handler(mb_mapping, request, rc, header_length);
        ret = modbus_reply(ctx,request,rc,mb_mapping);//rc, request size must be given back to modbus_reply as well as "request" data
        if(ret < 0){
            log_error("modbus reply error:%d, continue next receive... !!!", ret);
            //goto THREAD_EXIT;
        }
    }
THREAD_EXIT:
        //Clean up everything:
        modbus_mapping_free(mb_mapping);
        modbus_close(ctx);
        modbus_free(ctx);
        free(request);
        pthread_exit(NULL);
}

void sig_handler(int signum){

  //Return type of the handler function should be void
  log_trace("\nGot signal interrupt, exit...\n");
  exit(0);
}

int main(int argc, char* argv[]) {
     signal(SIGINT,sig_handler); // Register signal handler
    log_set_quiet(LOG_SILIENCE);
#if 0
    log_trace("Argc is:%d", argc);
    if (ini_parse(argv[1], handler, &Config) < 0) {
            printf("Can't load 'test.ini', using defaults\n");
        }
    dump_config(&Config);
    printf("---------------\n");
    printf("type: %s\n", Config.connection_type);
    printf("port: %s\n", Config.connection_port);
    printf("slaveid: %s\n", Config.connection_slaveid);
    printf("---------------\n");
#endif

    pthread_t thread_id[MAX_THREAD];
    if ( argc > 1 ) {
        //parse all the ini files
        for ( int j = 1; j < argc; j++ ) {
            log_trace("argv[%d]=%s", j, argv[j]);
            if (ini_parse( argv[j], handler, &(Configs[j-1]) ) < 0) {
                log_trace("----- Can't load:%s, using defaults -------", argv[j]);
            } else {
                //log_trace("INI parse %s Done.", argv[j]);
                //log_trace("before create pthread\n");
                pthread_create(&(thread_id[j-1]), NULL, thread, &(Configs[j-1]));
                config *p = &(Configs[j-1]);
#if 0
                dump_config(p);
                log_trace("Connection is:%s\n", p->connection_type);
                printf("------test---------\n");
                printf("type: %s\n", p->connection_type);
                printf("port: %s\n", p->connection_port);
                printf("slaveid: %s\n", p->connection_slaveid);
                printf("---------------\n");
#endif
            }
        }
    }

    while(1) {

        for ( int j = 1; j < argc; j++ ) {
            pthread_join(thread_id[j-1],NULL);
        }

        sleep(1);
    }
    return 0;
}
