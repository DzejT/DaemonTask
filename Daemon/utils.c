#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <memory.h>
#include <iotp_device.h>
#include <argp.h>
#include <syslog.h>
#include "watson.h"


void create_daemon(pid_t *pid, char *PID_FILE){
    FILE *fp;
    if ( (pid = fork()) == -1 )
    {
        syslog (LOG_ERR, "Can't fork");
        exit(1);
    }
    else if ( (pid != 0) )
    {
        exit(0);
    }

    setsid(); 

    if ( (pid = fork()) == -1 )
    {
        syslog (LOG_ERR, "Can't fork");
        exit(1);
    }

    else if ( pid > 0 )
    {
        if ( (fp = fopen(PID_FILE, "w")) == NULL )
        {
            syslog (LOG_ERR, "Can't open file");
            exit(1);
        }
        fprintf(fp, "%d\n", (int)pid); 
        fclose(fp);
        exit(0);
    }
}


void close_program(IoTPDevice *device, IoTPConfig *config){
    if(device != NULL){
        IoTPDevice_disconnect(device);
        IoTPDevice_destroy(device);
    }
    if(config != NULL)
        IoTPConfig_clear(config);
    exit(1);
}

