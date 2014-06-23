#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "structs.h"
#include "parser.h"
 
ServerEnv serverEnv;

int loadFromFile() {
    int i;
    ServerEnv* env = &serverEnv;
    FILE* file = NULL;

    if ((file = fopen(USER_LIST_FILENAME, "r")) == NULL) {
        perror("Error Raised.\n");
        exit(EXIT_FAILURE);
    }

    fread(&(env->userCount), sizeof(int), 1, file);
    fread(env->userList, sizeof(User), env->userCount, file);

    fclose(file);

    return 1;
}

int saveToFile() {
    FILE* file = NULL;
    ServerEnv* env = &serverEnv;

    if ((file = fopen(USER_LIST_FILENAME, "w+")) == NULL) {
        perror("Error Raised.\n");
        exit(EXIT_FAILURE);
    }

    fwrite(&(env->userCount), sizeof(int), 1, file);
    fwrite(env->userList, sizeof(User), env->userCount, file);

    fclose(file);

    return 1;
}

void beforeExit(int sig) {
    saveToFile();
    exit(sig);
}

int main(int argc, char * argv[]) {
    
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        perror("Fork\n");
        exit(EXIT_FAILURE);
    }
    if (pid != 0) {
        exit(0);
    }

    pid = setsid();
    if (pid < -1) {
        perror("Setsid\n");
        exit(EXIT_FAILURE);
    }

    int dev_fd = open("/dev/null", O_RDWR, 0);
    if (dev_fd == -1) {
        dup2(dev_fd, STDIN_FILENO);
        dup2(dev_fd, STDOUT_FILENO);
        dup2(dev_fd, STDERR_FILENO);
        if (dev_fd > 2) {
            close(dev_fd);
        }
    }
    umask(0000);

    int shmid;
    void* responseMEM = (void*) NULL;
    void* protocolMEM = (void*) NULL;
    Response* response = NULL;
    Protocol* protocol = NULL;

    serverEnv.userCount = 0;

    loadFromFile();

    signal(SIGKILL, beforeExit);
    signal(SIGINT, beforeExit);
    signal(SIGTERM, beforeExit);

    serverEnv.serverShmID = shmget((key_t) SERVER_SHMID, sizeof(Protocol), 0666 | IPC_CREAT);
    if (serverEnv.serverShmID == -1) {
        fprintf(stderr, "shmget(server) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    protocolMEM = shmat(serverEnv.serverShmID, (void*) 0, 0);
    if (protocolMEM == (void*) -1) {
        fprintf(stderr, "shmat(server) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    serverEnv.protocol = (Protocol*) protocolMEM;
    serverEnv.protocol->isNew = 0;
    serverEnv.protocol->pid = 0;
    serverEnv.protocol->msg[0] = '\0';

    printf("\nServer is runing!\n");

    while(1) {
        if (serverEnv.protocol->pid > 0 && serverEnv.protocol->isNew == 1) {
            printf("Protocol: %d - %s", serverEnv.protocol->pid, serverEnv.protocol->msg);
            shmid = shmget((key_t) serverEnv.protocol->pid, sizeof(Response), 0666 | IPC_CREAT);
            responseMEM = shmat(shmid, (void*) NULL, 0);
            response = (Response*) responseMEM;
            if (parse(&serverEnv, response)) {
                serverEnv.protocol->pid = 0;
            }
        }
    }

    return 0;
}