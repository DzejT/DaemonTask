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

#define CONFIG_FILE "config.yaml"
#define PID_FILE "/var/run/my-daemon.pid"


const char *argp_program_version = "daemon 1.0";
const char *argp_program_bug_address = "<your e-mail address>";



static struct argp_option options[] = {
    {"deviceId", 'd', "DEVICE", 0, "override default device id"},
    {"orgId", 'o', "ORG", 0, "override default organization id"},
    {"typeId", 't', "TYPE", 0, "override default type id"},
    {"authToken", 'a', "TOKEN", 0, "override default authentication token"},
    { 0 }
};

struct arguments{
    char *args[4];
    char *device_id;
    char *org_id;
    char *type_id;
    char *auth_token;
};

int run_loop = 0;
char *progname = "daemon";
int testCycle = 0;

void handle_log_in(struct arguments arguments, char cwd[]);
void publish_data(IoTPDevice *device);
void override_config(struct arguments args, IoTPConfig *config);
void sigHandler(int signo);
static error_t parse_opt(int key, char *arg, struct argp_state *state);

static struct argp argp = { options, parse_opt, 0, 0};


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
    FILE *fp;

    struct arguments arguments;

    arguments.device_id = NULL;
    arguments.org_id = NULL;
    arguments.type_id = NULL;
    arguments.auth_token = NULL;

    argp_parse (&argp, argc, argv, 0, 0, &arguments);


    if ( (pid = fork()) == -1 )
    {
        syslog (LOG_ERR, "Can't fork");
        return 1;
    }
    else if ( (pid != 0) )
    {
        exit(0);
    }

    setsid(); 

    if ( (pid = fork()) == -1 )
    {
        syslog (LOG_ERR, "Can't fork");
        return 1;
    }

    else if ( pid > 0 )
    {
        if ( (fp = fopen(PID_FILE, "w")) == NULL )
        {
            syslog (LOG_ERR, "Can't open file");
            return 1;
        }
        fprintf(fp, "%d\n", pid); 
        fclose(fp);
        exit(0);
    }


    handle_log_in(arguments, cwd);
    closelog ();
    
    return 0;

}

void handle_log_in(struct arguments arguments, char cwd[]){
    int rc = 0;

    IoTPConfig *config = NULL;
    IoTPDevice *device = NULL;


    rc = IoTPConfig_setLogHandler(IoTPLog_FileDescriptor, stdout);
    if ( rc != 0 ) {
        syslog (LOG_ERR, "WARN: Failed to set IoTP Client log handler: rc=%d\n", rc);
        exit(1);
    }

    char path[256];
    sprintf(path, "%s/%s", cwd, CONFIG_FILE);
    rc = IoTPConfig_create(&config, path);
    if ( rc != 0 ) {
        syslog (LOG_ERR, "ERROR: Failed to initialize configuration: rc=%d\n", rc);
        exit(1);
    }

    override_config(arguments, config);

    rc = IoTPDevice_create(&device, config);
    if ( rc != 0 ) {
        syslog (LOG_ERR, "ERROR: Failed to configure IoTP device: rc=%d\n", rc);
        exit(1);
    }

    rc = IoTPDevice_connect(device);
    if ( rc != 0 ) {
        syslog (LOG_ERR, "ERROR: Failed to connect to Watson IoT Platform: rc=%d\n", rc);
        syslog (LOG_ERR, "ERROR: Returned error reason: %s\n", IOTPRC_toString(rc));

        exit(1);
    }

    publish_data(device);

    rc = IoTPDevice_disconnect(device);
    if ( rc != IOTPRC_SUCCESS ) {
        syslog (LOG_ERR, "ERROR: Failed to disconnect from  Watson IoT Platform: rc=%d\n", rc);
    }

    IoTPDevice_destroy(device);
    IoTPConfig_clear(config);

}

void publish_data(IoTPDevice *device){

    
    int rc = 0;
    int cycle = 0;
    char *data = "{\"data\" : {\"Test\": \"Test\", \"data\": 20 }}";

    while(!run_loop)
    {
        syslog (LOG_INFO,  "Send status event\n");
        rc = IoTPDevice_sendEvent(device,"status", data, "json", QoS0, NULL);
        syslog (LOG_INFO,  "RC from publishEvent(): %d\n", rc);


        if ( testCycle > 0 ) {
            cycle += 1;
            if ( cycle >= testCycle ) {
                break;
            }
        }
        sleep(10);
    }
}

void sigHandler(int signo) {
    signal(SIGINT, NULL);
    syslog (LOG_INFO,  "Received signal: %d\n", signo);
    run_loop = 1;
}

void override_config(struct arguments args, IoTPConfig *config){
    if(args.org_id != NULL)
        IoTPConfig_setProperty(config, "identity.orgId", args.org_id);
    if(args.type_id != NULL)
        IoTPConfig_setProperty(config, "identity.typeId", args.type_id);
    if(args.device_id != NULL)
        IoTPConfig_setProperty(config, "identity.deviceId", args.device_id);
    if(args.auth_token != NULL)
        IoTPConfig_setProperty(config, "auth.token", args.auth_token);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state){
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

        case ARGP_KEY_ARG:
            if (state->arg_num >= 4)
                argp_usage (state);
            arguments->args[state->arg_num] = arg;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    
    }

    return 0;
}





