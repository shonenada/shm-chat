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
    int res, client_fd;
    Protocol protocol;
    protocol.msg_type = MSG_TYPE_COMMON;
    Response* response;
    serverEnv.userCount = 0;

    loadFromFile();

    signal(SIGKILL, beforeExit);
    signal(SIGINT, beforeExit);
    signal(SIGTERM, beforeExit);

    serverEnv.serverMSGID = msgget((key_t) SERVER_MSGID, 0666 | IPC_CREAT);
    if (serverEnv.serverMSGID == -1) {
        fprintf(stderr, "Msgget(server) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("\nServer is runing!\n");

    while(1) {
        if (msgrcv(serverEnv.serverMSGID, (void*)&protocol, SIZE_OF_PROTOCOL, MSG_TYPE_COMMON, 0) == -1) {
            fprintf(stderr, "msgrcv failed with errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        else {
            printf("Protocol: %d - %s", protocol.pid, protocol.msg);
            response = parse(&serverEnv, &protocol);

            int msgid = msgget((key_t) protocol.pid, 0666);

            if (msgsnd(msgid, (void*)response, SIZE_OF_RESPONSE, 0) == -1) {
                fprintf(stderr, "MSGSND failed\n");
                exit(EXIT_FAILURE);
            }

            free(response);
        }
    }

    return 0;
}