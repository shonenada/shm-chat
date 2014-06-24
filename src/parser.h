#ifndef _PARSER_H
#define _PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "structs.h"

void broadcast(ServerEnv* env, char* msg) {
    int i;
    int shmid;
    int client_pid;
    void* responseMEM = (void*) NULL;
    Response* response;
    for (i=0; i<env->userCount; ++i) {
        client_pid = env->online[i];
        if (client_pid > 0 && client_pid != env->protocol->pid) {
            shmid = shmget((key_t) client_pid, sizeof(Response), 0666 | IPC_CREAT);
            responseMEM = shmat(shmid, (void*) NULL, 0);
            response = (Response*) responseMEM;
            response->type = RESPONSE_TYPE_CHT;
            response->state = CHT_TALK;
            strcpy(response->msg, msg);
            response->isNew = 1;
        }
    }
}

void RegHandler(ServerEnv* env, Response* response) {
    while(1) {
        if (response->isNew == 0) {
            break;
        }
    }
    int i, j, flag;
    User newUser;
    response->type = RESPONSE_TYPE_REG;

    // parse username
    i = 0;
    j = 4;
    while(env->protocol->msg[j] != ' ') {
        newUser.username[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    newUser.username[i] = '\0';

    if (isUsernameExist(env, newUser.username)) {
        response->state = REG_USERNAME_EXIST;
        sprintf(response->msg, "Username exists, please login.\n");
    }

    // parse password
    i = 0;
    j += 1;
    while(env->protocol->msg[j] != ' ' && env->protocol->msg[j] != '\n') {
        newUser.password[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    newUser.password[i] = '\0';

    flag = regUser(env, newUser);

    if (flag == -1) {
        response->state = REG_MAX_USER;
        sprintf(response->msg, "Max User\n");
        response->isNew = 1;
        return ;
    }
    else if (flag == 1) {
        response->state = REG_SUCCESS;
        sprintf(response->msg, "Reg Success\n");
        response->isNew = 1;
        return ;
    }
    else {
        response->state = REG_UNSUCCESS;
        sprintf(response->msg, "Reg Unsuccess\n");
        response->isNew = 1;
        return ;
    }

    response->state = REG_UNKNOWN;
    sprintf(response->msg, "Reg Unknown\n");
    response->isNew = 1;
    return ;
}

void LoginHandler(ServerEnv* env, Response* response) {
    while(1) {
        if (response->isNew == 0) {
            break;
        }
    }
    int i, j, flag;
    char username[32];
    char password[32];
    response->type = RESPONSE_TYPE_LOG;

    i = 0;
    j = 4;
    while (env->protocol->msg[j] != ' ') {
        username[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    username[i] = '\0';

    i = 0;
    j += 1;
    while (env->protocol->msg[j] != ' ' && env->protocol->msg[j] != '\n') {
        password[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    password[i] = '\0';

    flag = loginUser(env, username, password, env->protocol->pid);
    if (flag == 2) {
        response->state = LOG_USERNAME_NOT_EXIST;
        sprintf(response->msg, "Username not exists\n");
        response->isNew = 1;
        return ;
    }
    else if (flag == 1) {
        char msg[512];
        response->state = LOG_SUCCESS;
        sprintf(response->msg, "%s: Login successfully.\n", username);
        sprintf(msg, "\033[47;31mSystem Info: %s onlined.\033[0m\n", username);
        broadcast(env, msg);
        response->isNew = 1;
        return ;
    }
    else if (flag == 0) {
        response->state = LOG_UNSUCCESS;
        sprintf(response->msg, "Wrong username or password\n");
        response->isNew = 1;
        return ;
    }

    response->state = LOG_UNKNOWN;
    sprintf(response->msg, "Unknown Error\n");
    response->isNew = 1;
    return ;
}

void IndirectChatHandler(ServerEnv* env, Response* response) {
    while(1) {
        if (response->isNew == 0) {
            break;
        }
    }
    int i, j;
    char msg[250];
    response->type = RESPONSE_TYPE_CHT;

    i = 0;
    j = 4;
    while(env->protocol->msg[j] != ' ' && env->protocol->msg[j] != '\n') {
        msg[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    msg[i] = '\0';

    User* user = findUserByPid(env, env->protocol->pid);
    if (user == NULL) {
        response->state = CHT_USER_NOT_LOGIN;
        sprintf(response->msg, "User not login\n");
        response->isNew = 1;
        return ;
    }

    char responseMSG[1024];
    sprintf(responseMSG, "\033[47;31m%s\033[0m say: \033[32m%s\033[0m\n", user->username, msg);
    broadcast(env, responseMSG);
    response->state = CHT_SUCCESS;
    sprintf(response->msg, "Success.\n");
    response->isNew = 1;
    return ;
}

void DirectChatHandler(ServerEnv* env, Response* response) {
    int i, j;
    char msg[250];
    char directUsername[32];
    response->type = RESPONSE_TYPE_CHT;

    i = 0;
    j = 5;
    while(env->protocol->msg[j] != ' ' && i < 31) {
        directUsername[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    directUsername[i] = '\0';

    i = 0;
    j += 1;
    while (env->protocol->msg[j] != ' ' && env->protocol->msg[j] != '\n') {
        msg[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    msg[i] = '\0';
    
    User *fromUser = findUserByPid(env, env->protocol->pid);
    int toUserId = findUserIdByUsername(env, directUsername);
    if (toUserId == -1) {
        response->state = CHT_USERNAME_NOT_EXIST;
        printf("Username not exists.\n");
        sprintf(response->msg, "Username not exists.\n");
        response->isNew = 1;
        return ;
    }

    int shmid;
    int client_pid;
    void* responseMEM = (void*) NULL;
    Response* chatResponse;
    client_pid = env->online[toUserId];
    shmid = shmget((key_t) client_pid, sizeof(Response), 0666 | IPC_CREAT);
    responseMEM = shmat(shmid, (void*) NULL, 0);
    chatResponse = (Response*) responseMEM;
    chatResponse->type = RESPONSE_TYPE_CHT;
    chatResponse->state = CHT_TALK;
    printf("%d\n", client_pid);
    sprintf(chatResponse->msg, "\033[47;34m%s\033[0m talk to you: \033[33m%s\033[0m\n", fromUser->username, msg);
    chatResponse->isNew = 1;
    response->state = CHT_SUCCESS;
    sprintf(response->msg, "Success.\n");
    response->isNew = 1;
    return ;
}

void Logout(ServerEnv* env, Response* response) {
    int i, j;
    char msg[512];
    char username[32];
    i = 0;
    j = 4;
    while(env->protocol->msg[j] != ' ' && env->protocol->msg[j] != '\n') {
        username[i] = env->protocol->msg[j];
        i++;
        j++;
    }
    username[i] = '\0';
    sprintf(msg, "\033[47;31mSystem Info: %s offlined.\033[0m\n", username);
    for(i=0; i<env->userCount; ++i) {
        if (env->online[i] == env->protocol->pid) {
            env->online[i] = 0;
            response->state = OUT_SUCCESS;
            response->type = RESPONSE_TYPE_OUT;
            sprintf(response->msg, "Logout Success.\n");
            broadcast(env, msg);
            response->isNew = 1;
            return ;
        }
    }
}

int parse(ServerEnv* env, Response* response) {
    int ret;
    char firstChar = env->protocol->msg[0];
    switch (firstChar) {
        case 'R':
            RegHandler(env, response);
            return 1;
        break;
        case 'L':
            LoginHandler(env, response);
            return 1;
        break;
        case 'C':
            if (env->protocol->msg[4] == '@') {
                DirectChatHandler(env, response);
                return 1;
            }
            else {
                IndirectChatHandler(env, response);
                return 1;
            }
        break;
        case 'O':
            Logout(env, response);
            return 1;
        break;
    }
}

#endif // _PARSER_H
