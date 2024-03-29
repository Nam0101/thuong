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
#include "./src/authentication.h"
#include "./src/room.h"
#include "./src/top2History.h"
#define PORT 8080
#define MAXCLIENT 100
#define BACKLOG 100
#define BUFFER_SIZE 4096

struct ThreadArgs
{
    int client_socket;
};

void *handleClient(void *arg)
{
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)arg;
    int client_socket = threadArgs->client_socket;

    char *buffer = (char *)malloc(BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);

    while (1)
    {
        int len = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (len == -1)
        {
            printf("Receive failed!\n");
            break;
        }
        if (len == 0)
        {
            printf("Client disconnected!\n");
            break;
        }

        struct json_object *parsed_json = json_tokener_parse(buffer);
        struct json_object *jtype;
        json_object_object_get_ex(parsed_json, "type", &jtype);
        int type = json_object_get_int(jtype);

        switch (type)
        {
        case LOGIN:
            handleLogin(client_socket, parsed_json);
            break;
        case REGISTER:
            handleRegister(client_socket, parsed_json);
            break;
        case LOGOUT:
            handleLogout(client_socket, parsed_json);
            break;
        case CREATE_ROOM:
            handleCreateRoom(client_socket, parsed_json);
            break;
        case JOIN_ROOM:
            handleJoinRoom(client_socket, parsed_json);
            break;
        case OUT_ROOM:
            handleLeaveRoom(client_socket, parsed_json);
            break;
        case GET_ROOM_LIST:
            handleGetRoomList(client_socket, parsed_json);
            break;
        case GET_TOP_SCORE:
            handleGetTopScore(client_socket, parsed_json);
            break;
        case MOVE:
            handleMove(client_socket, parsed_json);
            break;
        case END_GAME:
            handleEndGame(client_socket, parsed_json);
            break;
        case CHAT:
            handleChat(client_socket, parsed_json);
            break;
        case SURRENDER:
            handleSurrender(client_socket, parsed_json);
            break;
        case GET_HISTORY:
            handleGetHistory(client_socket, parsed_json);
            break;
        case CHALLENGE:
            handleChallenge(client_socket, parsed_json);
            break;
        case NOTIFI_CHALLENGE:
            handleNotifiChallenge(client_socket, parsed_json);
            break;
        case GET_LIST_ONLINE_USER:
            handleGetListOnlineUser(client_socket, parsed_json);
            break;
        default:
            break;
        }
    }

    free(buffer);
    close(client_socket);

    return NULL;
}

int main()
{
    int server_socket;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        printf("Create socket failed!\n");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        printf("Bind failed!\n");
        exit(1);
    }

    if (listen(server_socket, BACKLOG) == -1)
    {
        printf("Listen failed!\n");
        exit(1);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1)
    {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        printf("client_socket connect: %d\n", client_socket);
        if (client_socket == -1)
        {
            printf("Accept failed!\n");
            exit(1);
        }

        printf("New connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        struct ThreadArgs *threadArgs = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
        threadArgs->client_socket = client_socket;

        pthread_t tid;
        pthread_create(&tid, NULL, handleClient, threadArgs);
        pthread_detach(tid);
    }

    close(server_socket);

    return 0;
}
