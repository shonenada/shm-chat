#ifndef _STRUCT_H
#define _STRUCT_H

// *** settings ***
#define MAX_USER 20
#define SERVER_SHMID 14621
#define USER_LIST_FILENAME "userlist.db"
// *** end settings ***

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
    int isNew;
    int pid;
    char msg[1024];
} Protocol;

/**
 * 响应协议定义
 *  type 类型(REG, LOG, CHT, OUT)
 *  state 响应的状态。
 *  msg 表示响应的信息。
 **/
typedef struct {
    int isNew;
    int type;
    int state;
    char msg[1024];
} Response;

/**
 * 定义 User 模型
 **/
typedef struct {
    char username[32];
    char password[32];
} User;

/**
 * Server Environment
 */
typedef struct {
    int serverShmID;
    int userCount;
    int onlineCount;
    int online[MAX_USER];
    Protocol* protocol;
    User userList[MAX_USER];
} ServerEnv;

/**
 * Client Environment
 **/
typedef struct {
    int pid;
    int serverShmID;
    int clientShmID;
    char username[32];
    Protocol* protocol;
    Response* response;
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
