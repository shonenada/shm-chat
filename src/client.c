#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
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
    while(1) {
        if (env->protocol->pid == 0) {
            env->protocol->pid = env->pid;
            sprintf(env->protocol->msg, "OUT %s\n", env->username);
            env->protocol->isNew = 1;
            break;
        }else {
            sleep(1);
        }
    }
}
 
int RegController(ClientEnv* env) {
    char username[32];
    char password[32];
    printf("Please input your username: ");
    scanf("%s", username);
    printf("Please input your password: ");
    scanf("%s", password);
    while(1) {
        if(env->protocol->pid == 0) {
            env->protocol->pid = env->pid;
            sprintf(env->protocol->msg, "REG %s %s\n", username, password);
            env->protocol->isNew = 1;
            break;
        }else {
            sleep(1);
        }
    }
    return 1;
}

void* WaitChatResponse(void* param) {
    ClientEnv* env = (ClientEnv*) param;
    while(1) {
        if (env->response->isNew == 1) {
            if (env->response->type == RESPONSE_TYPE_CHT) {
                if (env->response->state == CHT_TALK) {
                    printf("\033[9D\033[K%s>>> Say: ", env->response->msg);
                    fflush(stdout);
                }
            }
            else if (env->response->type == RESPONSE_TYPE_OUT) {
                if (env->response->state == OUT_SUCCESS) {
                    printf("Logout Successfully.\n");
                    exit(0);
                }
            }
            env->response->isNew = 0;
        } else {
            sleep(1);
        }
    }
}

int WaitRegResponse(ClientEnv* env) {
    while(1) {
        if (env->response->isNew == 1) {
            if (env->response->type == RESPONSE_TYPE_REG) {
                printf("%d %s", env->response->state, env->response->msg);
                env->response->isNew = 0;
                return 1;
            }
            env->response->isNew = 0;
        } else {
            sleep(1);
        }
    }
}

int LoginController(ClientEnv* env) {
    char username[32];
    char password[32];
    printf("Please input your username: ");
    scanf("%s", username);
    printf("Please input your password: ");
    scanf("%s", password);
    while(1) {
        if (env->protocol->pid == 0) {
            env->protocol->pid = env->pid;
            sprintf(env->protocol->msg, "LOG %s %s\n", username, password);
            env->protocol->isNew = 1;
            break;
        }else {
            sleep(1);
        }
    }
    return 1;
}

int WaitLoginResponse(ClientEnv* env) {
    int i;
    while(1) {
        if (env->response->isNew == 1) {
            if (env->response->type == RESPONSE_TYPE_LOG) {
                if (env->response->state == LOG_SUCCESS) {
                    i = 0;
                    while(env->response->msg[i] != ':' && i < 31) {
                        env->username[i] = env->response->msg[i];
                        i++;
                    }
                    env->username[i] = '\0';
                    printf("Login Successfully\n");
                    LoopChat(env);
                }
                else {
                    printf("%d %s", env->response->state, env->response->msg);
                }
                env->response->isNew = 0;
                return 1;
            }
        } else {
            sleep(1);
        }
    }
}

int LoopChat(ClientEnv* env) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, &WaitChatResponse, env);
    char buffer[512];
    while(1) {
        setbuf(stdin, NULL);
        printf(">>> Say: ");
        fgets(buffer, 512, stdin);
        if (strcmp(buffer, "help\n") == 0) {
            showHelp();
            continue;
        }
        if (strcmp(buffer, "quit\n") == 0) {
            exit(0);
            continue;
        }
        while(1) {
            if (env->protocol->pid == 0) {
                env->protocol->pid = env->pid;
                sprintf(env->protocol->msg, "CHT %s", buffer);
                env->protocol->isNew = 1;
                break;
            } else {
                sleep(1);
            }
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

    void* protocolMEM = (void*) NULL;
    void* responseMEM = (void*) NULL;
    clientEnv.pid = getpid();
    clientEnv.protocol = NULL;
    clientEnv.response = NULL;

    signal(SIGKILL, beforeExit);
    signal(SIGINT, beforeExit);
    signal(SIGTERM, beforeExit);

    clientEnv.serverShmID = shmget((key_t) SERVER_SHMID, sizeof(Protocol), 0666 | IPC_CREAT);
    if (clientEnv.serverShmID == -1) {
        fprintf(stderr, "shmget(server) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    protocolMEM = shmat(clientEnv.serverShmID, (void*) NULL, 0);
    if (protocolMEM == (void*) -1) {
        fprintf(stderr, "shmat(server) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    clientEnv.clientShmID = shmget((key_t) clientEnv.pid, sizeof(Response), 0666 | IPC_CREAT);
    if (clientEnv.clientShmID == -1) {
        fprintf(stderr, "shmget(client) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    responseMEM = shmat(clientEnv.clientShmID, (void*) NULL, 0);
    if (responseMEM == (void*) -1) {
        fprintf(stderr, "shmget(client) failed with errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    clientEnv.protocol = (Protocol*) protocolMEM;
    clientEnv.response = (Response*) responseMEM;

    doChoose(&clientEnv);

    return 0;
}
