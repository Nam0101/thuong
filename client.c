#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <json-c/json.h>
#include "./include/type.h"
#define SERVER_ADDRESS "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

void login_func(int client_socket)
{
    // send Json
    struct json_object *jobj = json_object_new_object();
    struct json_object *jtype = json_object_new_int(LOGIN);
    struct json_object *jusername = json_object_new_string("admin");
    struct json_object *jpassword = json_object_new_string("admin");
    json_object_object_add(jobj, "type", jtype);
    json_object_object_add(jobj, "username", jusername);
    json_object_object_add(jobj, "password", jpassword);
    const char *json_string = json_object_to_json_string(jobj);
    send(client_socket, json_string, strlen(json_string), 0);
    // receive Json
}
int main()
{
    // create socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        printf("Create socket failed!\n");
        exit(1);
    }
    // connect to server
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr);
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        printf("Connect failed!\n");
        exit(1);
    }
    while (1)
    {
        printf("CLIENT EXAMPLE!\n");
        printf("1. Login\n");
        printf("2. Register\n");
        printf("3. Exit\n");
        printf("Your choice: ");
        int choice;
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            login_func(client_socket);
            break;

        default:
            break;
        }
    }
}