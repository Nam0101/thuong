#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <json-c/json.h>

void handleLogin(int client_socket, struct json_object *parsed_json);
void handleRegister(int client_socket, struct json_object *parsed_json);
void sendResponse(int client_socket, int type, int status, const char *message);
void handleLogout(int client_socket, struct json_object *parsed_json);
#endif
