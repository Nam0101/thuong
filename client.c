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

int is_login = 0;
char USERNAME[100];
int USER_ID;
int SCORE;

void login_func(int client_socket)
{
    // send Json
    struct json_object *jobj = json_object_new_object();
    struct json_object *jtype = json_object_new_int(LOGIN);
    printf("Username: ");
    char username[100];
    scanf("%s", username);
    struct json_object *jusername = json_object_new_string(username);
    printf("Password: ");
    char password[100];
    scanf("%s", password);
    struct json_object *jpassword = json_object_new_string(password);
    json_object_object_add(jobj, "type", jtype);
    json_object_object_add(jobj, "username", jusername);
    json_object_object_add(jobj, "password", jpassword);
    const char *json_string = json_object_to_json_string(jobj);
    send(client_socket, json_string, strlen(json_string), 0);
}
void register_func(int client_socket)
{
    // send Json
    struct json_object *jobj = json_object_new_object();
    struct json_object *jtype = json_object_new_int(REGISTER);
    printf("Username: ");
    char username[100];
    scanf("%s", username);
    struct json_object *jusername = json_object_new_string(username);
    printf("Password: ");
    char password[100];
    scanf("%s", password);
    struct json_object *jpassword = json_object_new_string(password);
    json_object_object_add(jobj, "type", jtype);
    json_object_object_add(jobj, "username", jusername);
    json_object_object_add(jobj, "password", jpassword);
    const char *json_string = json_object_to_json_string(jobj);
    send(client_socket, json_string, strlen(json_string), 0);
}
void logout_func(int client_socket)
{
    // send Json
    struct json_object *jobj = json_object_new_object();
    struct json_object *jtype = json_object_new_int(LOGOUT);
    json_object_object_add(jobj, "type", jtype);
    json_object_object_add(jobj, "user_id", json_object_new_int(USER_ID));
    const char *json_string = json_object_to_json_string(jobj);
    send(client_socket, json_string, strlen(json_string), 0);
    exit(0);
}
void create_room(int client_socket)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jtype = json_object_new_int(CREATE_ROOM);
    json_object_object_add(jobj, "type", jtype);
    json_object_object_add(jobj, "user_id", json_object_new_int(USER_ID));
    const char *json_string = json_object_to_json_string(jobj);
    send(client_socket, json_string, strlen(json_string), 0);
}
void join_room(int client_socket)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jtype = json_object_new_int(JOIN_ROOM);
    json_object_object_add(jobj, "type", jtype);
    json_object_object_add(jobj, "user_id", json_object_new_int(USER_ID));
    printf("Room id: ");
    int room_id;
    scanf("%d", &room_id);
    json_object_object_add(jobj, "room_id", json_object_new_int(room_id));
    const char *json_string = json_object_to_json_string(jobj);
    send(client_socket, json_string, strlen(json_string), 0);
    printf("%s\n", json_string);

}
void *send_func(void *arg)
{
    int client_socket = *(int *)arg;
    while (1)
    {
        printf("CLIENT EXAMPLE!\n");
        printf("1. Login\n");
        printf("2. Register\n");
        printf("3. Create room\n");
        printf("4. Join room\n");
        printf("10. Exit\n");
        printf("Your choice: ");
        int choice;
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            login_func(client_socket);
            break;
        case 2:
            register_func(client_socket);
            break;
        case 3:
            create_room(client_socket);
            break;
        case 4:
            join_room(client_socket);
            break;
        case 10:
            logout_func(client_socket);

        default:
            break;
        }
    }
}
void *receive_func(void *arg)
{
    int client_socket = *(int *)arg;
    char *buffer = (char *)malloc(BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);
    while (1)
    {
        int len = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (len == -1)
        {
            printf("Receive failed!\n");
            exit(1);
        }
        printf("\n%s\n", buffer);
        // get type:
        struct json_object *parsed_json = json_tokener_parse(buffer);
        struct json_object *jtype;
        json_object_object_get_ex(parsed_json, "type", &jtype);
        int type = json_object_get_int(jtype);
        struct json_object *jstatus;
        switch (type)
        {
        case LOGIN:
            json_object_object_get_ex(parsed_json, "status", &jstatus);
            int status = json_object_get_int(jstatus);
            if (status == 1)
            {
                // login success
                is_login = 1;
                struct json_object *jusername;
                json_object_object_get_ex(parsed_json, "username", &jusername);
                strcpy(USERNAME, json_object_get_string(jusername));
                struct json_object *juser_id;
                json_object_object_get_ex(parsed_json, "user_id", &juser_id);
                USER_ID = json_object_get_int(juser_id);
                struct json_object *jscore;
                json_object_object_get_ex(parsed_json, "score", &jscore);
                SCORE = json_object_get_int(jscore);
            }
            else
            {
                // login failed
                is_login = 0;
                // print reason
                struct json_object *jmessage;
                json_object_object_get_ex(parsed_json, "message", &jmessage);
                printf("%s\n", json_object_get_string(jmessage));
            }
            break;
        case REGISTER:

            break;
        case LOGOUT:
            is_login = 0;

            break;
        }
        memset(buffer, 0, BUFFER_SIZE);
    }
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
    pthread_t send_thread;
    pthread_create(&send_thread, NULL, send_func, (void *)&client_socket);
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_func, (void *)&client_socket);
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    close(client_socket);
    return 0;
}