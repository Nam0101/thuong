#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <json-c/json.h>
typedef struct user
{
    int user_id;
    int status; // 0: offline, 1: online, 2: playing, 3: waiting
    char username[20];
    int score;
    int socket;
    struct user *next;
} user;
void handleLogin(int client_socket, struct json_object *parsed_json);
void handleRegister(int client_socket, struct json_object *parsed_json);
void sendResponse(int client_socket, int type, int status, const char *message);
void handleLogout(int client_socket, struct json_object *parsed_json);
user *getListUser();
user *getUserByID(int user_id);
#endif
