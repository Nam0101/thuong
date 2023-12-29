#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/type.h"
#include "authentication.h"

void handleLogin(int client_socket, struct json_object *parsed_json)
{
    printf("LOGIN\n");
    struct json_object *jusername;
    struct json_object *jpassword;
    json_object_object_get_ex(parsed_json, "username", &jusername);
    json_object_object_get_ex(parsed_json, "password", &jpassword);
    const char *username = json_object_get_string(jusername);
    const char *password = json_object_get_string(jpassword);
    printf("Username: %s\n", username);
    printf("Password: %s\n",password);
}