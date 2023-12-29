#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <json-c/json.h>

void handleLogin(int client_socket, struct json_object *parsed_json);

#endif
