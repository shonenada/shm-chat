#ifndef _STRUCT_H
#define _STRUCT_H

// *** settings ***
#define MAX_USER 20
#define SERVER_MSGID 14621
#define USER_LIST_FILENAME "userlist.db"
// *** end settings ***

const long int MSG_TYPE_COMMON = 1;
const long int MSG_TYPE_REG_RESPONSE = 2;
const long int MSG_TYPE_LOGIN_RESPONSE = 3;
const long int MSG_TYPE_CHT = 4;

const int RESPONSE_TYPE_REG = 1;
const int RESPONSE_TYPE_LOG = 2;
const int RESPONSE_TYPE_CHT = 3;
const int RESPONSE_TYPE_OUT = 4;

const int REG_SUCCESS = 1;
const int REG_USERNAME_EXIST = 2;
const int REG_UNSUCCESS = 3;
const int REG_UNKNOWN = 4;
const int REG_MAX_USER = 5;

const int LOG_SUCCESS = 1;
const int LOG_UNSUCCESS = 2;
const int LOG_UNKNOWN = 3;
const int LOG_USERNAME_NOT_EXIST = 4;

const int CHT_TALK = 0;
const int CHT_SUCCESS = 1;
const int CHT_USERNAME_NOT_EXIST = 2;
const int CHT_USER_NOT_LOGIN = 3;

const int OUT_SUCCESS = 1;

/**
 * 请求协议定义
 *  首 3 个字符为 REG LOG CHT OUT 分别表示，注册、登录、发送信息、退出
 *  第 4 个字符为 空格
 *  随后的字符根据不同的 method 有不同的定义。
 *   REG：第 5 个字符起到末尾(\n) 表示欲注册的用户名和密码。用空格分隔开。
 *   LOG：第 5 个字符起到末尾(\n) 表示欲登录的用户名和密码。用空格分隔开。
 *   CHT：第 5 个字符起到末尾，表示发送的信息。
 *   OUT：无需其他信息。
 **/
typedef struct {
    long int msg_type;
    int pid;
    char msg[1024];
} Protocol;
const int SIZE_OF_PROTOCOL = sizeof(int) + sizeof(char) * 1024;

/**
 * 响应协议定义
 *  type 类型(REG, LOG, CHT)
 *  state 响应的状态。
 *  msg 表示响应的信息。
 **/
typedef struct {
    long int msg_type;
    int type;
    int state;
    char msg[1024];
} Response;
const int SIZE_OF_RESPONSE = sizeof(int) * 2 + sizeof(char) * 1024;

/**
 * 定义 User 模型
 **/
typedef struct {
    long int msg_type;
    char username[32];
    char password[32];
} User;
const int SIZE_OF_USER = sizeof(char) * (32 + 32);

/**
 * Server Environment
 */
typedef struct {
    long int msg_type;
    int serverMSGID;
    int userCount;
    int onlineCount;
    int online[MAX_USER];
    User userList[MAX_USER];
} ServerEnv;

/**
 * Client Environment
 **/
typedef struct {
    long int msg_type;
    int pid;
    int serverMSGID;
    int clientMSGID;
    char username[32];
} ClientEnv;

// Method for User Model

// Find an instnace of User by username
// Return the index of userlist.
// Return -1 if username not found
int findUserIdByUsername(ServerEnv* env, char* username) {
    int i;
    for(i=0; i<env->userCount; ++i) {
        if (strcmp(username, env->userList[i].username) == 0) {
            return i;
        }
    }
    return -1;
}

int findUserIdByPid(ServerEnv* env, int pid) {
    int i;
    for (i=0;i<env->userCount;++i) {
        if (pid == env->online[i]) {
            return i;
        }
    }
    return -1;
}

User* findUserByPid(ServerEnv* env, int pid) {
    int idx = findUserIdByPid(env, pid);
    if (idx == -1) {
        return NULL;
    }
    return &(env->userList[idx]);
}

// Check if the input username is exist.
int isUsernameExist(ServerEnv* env, char* username) {
    int idx = findUserIdByUsername(env, username);
    if (idx == -1)
        return 0;
    return 1;
}

// Register User.
int regUser(ServerEnv* env, User user) {
    if (env->userCount >= MAX_USER) {
        printf("Reach max user.");
        return -1;
    }
    memcpy(&(env->userList[env->userCount]), &user, sizeof(User));
    env->userCount += 1;
    return 1;
}

// Login
int loginUser(ServerEnv* env, char* username, char* password, int pid) {
    int idx;
    User user;
    idx = findUserIdByUsername(env, username);
    if (idx == -1) {
        return 2;
    }
    else if (idx >= 0 && idx < MAX_USER) {
        user = env->userList[idx];
        if (strcmp(password, user.password) == 0) {
            env->online[idx] = pid;
            env->onlineCount++;
            return 1;
        }
        return 0;
    }
    return -1;
}

#endif // _STRUCT_H
