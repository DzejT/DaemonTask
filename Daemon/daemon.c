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
#include "utils.h"
#include "watson.h"
#define PID_FILE "/var/run/my-daemon.pid"

int run_loop = 0;

const char *argp_program_version = "daemon 1.0";
const char *argp_program_bug_address = "<your e-mail address>";


struct argp_option options[] = {
    {"deviceId", 'd', "DEVICE", 0, "override default device id"},
    {"orgId", 'o', "ORG", 0, "override default organization id"},
    {"typeId", 't', "TYPE", 0, "override default type id"},
    {"authToken", 'a', "TOKEN", 0, "override default authentication token"},
    {"daemon", 'D', 0, 0, "checks whether to make it into daemon or not"},
    { 0 }
};

char *progname = "daemon";

void sigHandler(int signo);
error_t parse_opt(int key, char *arg, struct argp_state *state);

struct argp argp = { options, parse_opt, 0, 0};


int main(int argc, char *argv[])
{  
    openlog ("Daemon", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);


    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        syslog (LOG_INFO, "Current working dir: %s\n", cwd);
    }
    else {
        syslog (LOG_ERR, "getcwd() error");
        return 1;
    }

    pid_t pid;

    struct arguments args;

    args.device_id = NULL;
    args.org_id = NULL;
    args.type_id = NULL;
    args.auth_token = NULL;
    args.daemon = 0;

    argp_parse (&argp, argc, argv, 0, 0, &args);


    if(args.daemon == 1){
        create_daemon(&pid, PID_FILE);
    }
    
    handle_log_in(args, cwd, &run_loop);
    closelog ();
    
    return 0;

}

void sigHandler(int signo) {
    signal(SIGINT, NULL);
    syslog (LOG_INFO,  "Received signal: %d\n", signo);
    run_loop = 1;
}


error_t parse_opt(int key, char *arg, struct argp_state *state){
    struct arguments *arguments = state->input;
    switch(key){
        case 'd':
            arguments->device_id = arg;
            break;
        case 'o':
            arguments->org_id = arg;
            break;
        case 't':
            arguments->type_id = arg;
            break;
        case 'a':
            arguments->auth_token = arg;
            break;
        case 'D':
            arguments->daemon = 1;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 5)
                argp_usage (state);
            arguments->args[state->arg_num] = arg;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    
    }

    return 0;
}





