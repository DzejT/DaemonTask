void create_daemon(pid_t *pid, char *PID_FILE);
void close_program(IoTPDevice *device, IoTPConfig *config);

struct arguments{
    char *args[5];
    char *device_id;
    char *org_id;
    char *type_id;
    char *auth_token;
    int daemon;
};