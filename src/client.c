#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "structs.h"

ClientEnv clientEnv;

void showHelp() {
    printf("************* H E L P *************\n");
    printf("*   1. Talk to online user        *\n");
    printf("*      Just Say                   *\n");
    printf("*   2. Talk to particular user    *\n");
    printf("*      @username message          *\n");
    printf("***********************************\n");
}

void Logout() {
    ClientEnv* env = &clientEnv;
    Protocol protocol;
    protocol.msg_type = MSG_TYPE_COMMON;
    protocol.pid = getpid();
    sprintf(protocol.msg, "OUT\n");
    if (msgsnd(env->serverMSGID, (void*)&protocol, SIZE_OF_PROTOCOL, 0) == -1) {
        fprintf(stderr, "MSGSND failed\n");
        exit(EXIT_FAILURE);
    }
}
 
int RegController(ClientEnv* env) {
    char username[32];
    char password[32];
    Protocol protocol;
    protocol.msg_type = MSG_TYPE_COMMON;
    protocol.pid = getpid();
    printf("Please input your username: ");
    scanf("%s", username);
    printf("Please input your password: ");
    scanf("%s", password);
    sprintf(protocol.msg, "REG %s %s\n", username, password);
    if (msgsnd(env->serverMSGID, (void*)&protocol, SIZE_OF_PROTOCOL, 0) == -1) {
        fprintf(stderr, "MSGSND failed\n");
        exit(EXIT_FAILURE);
    }
    return 1;
}

void* WaitChatResponse(void* param) {
    Response response;
    ClientEnv* env = (ClientEnv*) param;
    while(1) {
        if (msgrcv(env->clientMSGID, (void*)&response, SIZE_OF_RESPONSE, MSG_TYPE_COMMON, 0) == -1) {
            fprintf(stderr, "msgrcv failed with errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        if (response.type == RESPONSE_TYPE_CHT) {
            if (response.state == CHT_TALK) {
                printf("\033[9D\033[K%s>>> Say: ", response.msg);
                fflush(stdout);
            }
        }
        else if (response.type == RESPONSE_TYPE_OUT) {
            if (response.state == OUT_SUCCESS) {
                printf("Logout Successfully.\n");
                exit(0);
            }
        }
    }
}

int WaitRegResponse(ClientEnv* env) {
    Response response;
    while(1) {
        if (msgrcv(env->clientMSGID, (void*)&response, SIZE_OF_RESPONSE, MSG_TYPE_COMMON, 0) == -1) {
            fprintf(stderr, "msgrcv failed with errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        if (response.type == RESPONSE_TYPE_REG) {
            printf("%d %s", response.state, response.msg);
            return 1;
        }
    }
}

int LoginController(ClientEnv* env) {
    char username[32];
    char password[32];
    Protocol protocol;
    protocol.msg_type = MSG_TYPE_COMMON;
    protocol.pid = getpid();
    printf("Please input your username: ");
    scanf("%s", username);
    printf("Please input your password: ");
    scanf("%s", password);
    sprintf(protocol.msg, "LOG %s %s\n", username, password);
    if (msgsnd(env->serverMSGID, (void*)&protocol, SIZE_OF_PROTOCOL, 0) == -1) {
        fprintf(stderr, "MSGSND failed\n");
        exit(EXIT_FAILURE);
    }
    return 1;
}

int WaitLoginResponse(ClientEnv* env) {
    int i;
    Response response;
    while(1) {
        if (msgrcv(env->clientMSGID, (void*)&response, SIZE_OF_RESPONSE, MSG_TYPE_COMMON, 0) == -1) {
            fprintf(stderr, "msgrcv failed with errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        if (response.type == RESPONSE_TYPE_LOG) {
            if (response.state == LOG_SUCCESS) {
                i = 0;
                while(response.msg[i] != ':' && i < 31) {
                    env->username[i] = response.msg[i];
                    i++;
                }
                env->username[i] = '\0';
                printf("Login Successfully\n");
                LoopChat(env);
            }
            else {
                printf("%d %s", response.state, response.msg);
            }
            return 1;
        }
    }
}

int LoopChat(ClientEnv* env) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, &WaitChatResponse, env);
    Protocol protocol;
    protocol.msg_type = MSG_TYPE_COMMON;
    protocol.pid = getpid();
    char buffer[512];
    while(1) {
        setbuf(stdin, NULL);
        printf(">>> Say: ");
        fgets(buffer, 512, stdin);
        if (strcmp(buffer, "help\n") == 0) {
            showHelp();
            continue;
        }
        sprintf(protocol.msg, "CHT %s", buffer);
        if (msgsnd(env->serverMSGID, (void*)&protocol, SIZE_OF_PROTOCOL, 0) == -1) {
            fprintf(stderr, "MSGSND failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

int showTips() {
    int cmdInput;
    printf("*****************************\n");
    printf("*  Please Input the number  *\n");
    printf("* 1. Register               *\n");
    printf("* 2. Login                  *\n");
    printf("* 3. Exit                   *\n");
    printf("*****************************\n");
    scanf("%d", &cmdInput);
    return cmdInput;
}

void doChoose(ClientEnv* env);
void parseInput(ClientEnv* env, int input) {
    switch(input) {
        case 1:
        RegController(env);
        WaitRegResponse(env);
        doChoose(env);
        break;
        case 2:
        LoginController(env);
        WaitLoginResponse(env);
        break;
        case 3:
        exit(0);
        break;
        default:
        doChoose(env);
        break;
    }
}

void doChoose(ClientEnv* env) {
    int chose;
    chose = showTips();
    parseInput(env, chose);
}

void beforeExit(int sig) {
    Logout();
    exit(sig);    
}

int main (int argc, char* argv[]) {
    // Initialize
    clientEnv.pid = getpid();

    signal(SIGKILL, beforeExit);
    signal(SIGINT, beforeExit);
    signal(SIGTERM, beforeExit);

    clientEnv.serverMSGID = msgget((key_t) SERVER_MSGID, 0666 | IPC_CREAT);
    if (clientEnv.serverMSGID == -1) {
        fprintf(stderr, "Msgget(server) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    clientEnv.clientMSGID = msgget((key_t) clientEnv.pid, 0666 | IPC_CREAT);
    if (clientEnv.clientMSGID == -1) {
        fprintf(stderr, "Msgget(client) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    doChoose(&clientEnv);

    return 0;
}
