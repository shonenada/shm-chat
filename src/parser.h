#ifndef _PARSER_H
#define _PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "structs.h"

Response* RegHandler(ServerEnv* env, Protocol* protocol) {
    int i, j, flag;
    User newUser;
    Response* response = (Response*) malloc(sizeof(Response));
    response->msg_type = MSG_TYPE_COMMON;
    response->type = RESPONSE_TYPE_REG;
    
    // parse username
    i = 0;
    j = 4;
    while(protocol->msg[j] != ' ') {
        newUser.username[i] = protocol->msg[j];
        i++;
        j++;
    }
    newUser.username[i] = '\0';

    if (isUsernameExist(env, newUser.username)) {
        response->state = REG_USERNAME_EXIST;
        sprintf(response->msg, "Username exists, please login.\n");
        return response;
    }

    // parse password
    i = 0;
    j += 1;
    while(protocol->msg[j] != ' ' && protocol->msg[j] != '\n') {
        newUser.password[i] = protocol->msg[j];
        i++;
        j++;
    }
    newUser.password[i] = '\0';

    flag = regUser(env, newUser);

    if (flag == -1) {
        response->state = REG_MAX_USER;
        sprintf(response->msg, "Max User\n");
        return response;
    }
    else if (flag == 1) {
        response->state = REG_SUCCESS;
        sprintf(response->msg, "Reg Success\n");
        return response;
    }
    else {
        response->state = REG_UNSUCCESS;
        sprintf(response->msg, "Reg Unsuccess\n");
        return response;
    }

    response->state = REG_UNKNOWN;
    sprintf(response->msg, "Reg Unknown\n");
    return response;
}

Response* LoginHandler(ServerEnv* env, Protocol* protocol) {
    int i, j, flag;
    char username[32];
    char password[32];
    Response* response = (Response*) malloc(sizeof(Response));
    response->msg_type = MSG_TYPE_COMMON;
    response->type = RESPONSE_TYPE_LOG;

    i = 0;
    j = 4;
    while(protocol->msg[j] != ' ') {
        username[i] = protocol->msg[j];
        i++;
        j++;
    }
    username[i] = '\0';

    i = 0;
    j += 1;
    while(protocol->msg[j] != ' ' && protocol->msg[j] != '\n') {
        password[i] = protocol->msg[j];
        i++;
        j++;
    }
    password[i] = '\0';

    flag = loginUser(env, username, password, protocol->pid);
    if (flag == 2) {
        response->state = LOG_USERNAME_NOT_EXIST;
        sprintf(response->msg, "Username not exists\n");
        return response;
    }
    else if (flag == 1) {
        response->state = LOG_SUCCESS;
        sprintf(response->msg, "%s: Login successfully.\n", username);
        return response;
    }
    else if (flag == 0) {
        response->state = LOG_UNSUCCESS;
        sprintf(response->msg, "Wrong username or password\n");
        return response;
    }

    response->state = LOG_UNKNOWN;
    sprintf(response->msg, "Unknown Error\n");
    return response;
}

Response* IndirectChatHandler(ServerEnv* env, Protocol* protocol) {
    int i, j;
    char msg[250];
    Response* response = (Response*) malloc(sizeof(Response));
    response->msg_type = MSG_TYPE_COMMON;
    response->type = RESPONSE_TYPE_CHT;

    i = 0;
    j = 4;
    while(protocol->msg[j] != ' ' && protocol->msg[j] != '\n') {
        msg[i] = protocol->msg[j];
        i++;
        j++;
    }
    msg[i] = '\0';

    User* user = findUserByPid(env, protocol->pid);
    if (user == NULL) {
        response->state = CHT_USER_NOT_LOGIN;
        sprintf(response->msg, "User not login\n");
        return response;
    }
    int msgid;
    int client_pid;
    Response* chatResponse = (Response*) malloc(sizeof(Response));
    chatResponse->msg_type = MSG_TYPE_COMMON;
    chatResponse->type = RESPONSE_TYPE_CHT;
    chatResponse->state = CHT_TALK;
    sprintf(chatResponse->msg, "\033[47;31m%s\033[0m say: \033[32m%s\033[0m\n", user->username, msg);
    for (i=0; i<env->userCount; ++i) {
        client_pid = env->online[i];
        if (client_pid > 0 && client_pid != protocol->pid) {
            msgid = msgget((key_t) client_pid, 0666);
            if (msgsnd(msgid, (void*)chatResponse, SIZE_OF_RESPONSE, 0) == -1) {
                fprintf(stderr, "MSGSND failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    response->state = CHT_SUCCESS;
    sprintf(response->msg, "Success.\n");
    free(chatResponse);
    return response;
}

Response* DirectChatHandler(ServerEnv* env, Protocol* protocol) {
    int i, j;
    char msg[250];
    char directUsername[32];
    Response* response = (Response*) malloc(sizeof(Response));
    response->msg_type = MSG_TYPE_COMMON;
    response->type = RESPONSE_TYPE_CHT;

    i = 0;
    j = 5;
    while(protocol->msg[j] != ' ' && i < 31) {
        directUsername[i] = protocol->msg[j];
        i++;
        j++;
    }
    directUsername[i] = '\0';

    i = 0;
    j += 1;
    while(protocol->msg[j] != ' ' && protocol->msg[j] != '\n') {
        msg[i] = protocol->msg[j];
        i++;
        j++;
    }
    msg[i] = '\0';
    
    User *fromUser = findUserByPid(env, protocol->pid);
    int toUserId = findUserIdByUsername(env, directUsername);
    if (toUserId == -1) {
        response->state = CHT_USERNAME_NOT_EXIST;
        printf("Username not exists.\n");
        sprintf(response->msg, "Username not exists.\n");
        return response;
    }
    int msgid;
    int client_pid;
    char pipe[200];
    Response* chatResponse = (Response*) malloc(sizeof(Response));
    chatResponse->msg_type = MSG_TYPE_COMMON;
    chatResponse->type = RESPONSE_TYPE_CHT;
    chatResponse->state = CHT_TALK;
    client_pid = env->online[toUserId];
    printf("%d\n", client_pid);
    sprintf(chatResponse->msg, "\033[47;34m%s\033[0m talk to you: \033[33m%s\033[0m\n", fromUser->username, msg);
    msgid = msgget((key_t) client_pid, 0666);
    if (msgsnd(msgid, (void*)chatResponse, SIZE_OF_RESPONSE, 0) == -1) {
        fprintf(stderr, "MSGSND failed\n");
        exit(EXIT_FAILURE);
    }
    free(chatResponse);

    response->state = CHT_SUCCESS;
    sprintf(response->msg, "Success.\n");
    return response;
}

Response* Logout(ServerEnv* env, Protocol* protocol) {
    int i;
    Response* response = (Response*) malloc(sizeof(Response));
    response->msg_type = MSG_TYPE_COMMON;
    response->type = RESPONSE_TYPE_OUT;
    for(i=0; i<env->userCount; ++i) {
        if (env->online[i] == protocol->pid) {
            env->online[i] = 0;
            response->state = OUT_SUCCESS;
            sprintf(response->msg, "Logout Success.\n");
            return response;
        }
    }
}

Response* parse(ServerEnv* env, Protocol* protocol) {
    int ret;
    char firstChar = protocol->msg[0];
    switch (firstChar) {
        case 'R':
            return RegHandler(env, protocol);
        break;
        case 'L':
            return LoginHandler(env, protocol);
        break;
        case 'C':
            if (protocol->msg[4] == '@') {
                return DirectChatHandler(env, protocol);
            }
            else {
                return IndirectChatHandler(env, protocol);
            }
        break;
        case 'O':
            return Logout(env, protocol);
        break;
    }
}

#endif // _PARSER_H
