#ifndef ROOM_H
#define ROOM_H
#include <json-c/json.h>
typedef struct room
{
    int room_id;
    int status;
    struct user *black_user;
    struct user *white_user;
    struct room *next;
} room;

void handleLeaveRoom(int clientSocket, struct json_object *parsedJson);
void handleJoinRoom(int clientSocket, struct json_object *parsedJson);
void handleCreateRoom(int clientSocket, struct json_object *parsedJson);
void handleGetRoomList(int clientSocket, struct json_object *parsedJson);
void handleMove(int clientSocket, struct json_object *parsedJson);
void handleEndGame(int clientSocket, struct json_object *parsedJson);
void handleJoinRoom(int clientSocket, struct json_object *parsedJson);
void handleLeaveRoom(int clientSocket, struct json_object *parsedJson);
void handleStartGame(int clientSocket, struct json_object *parsedJson);
void handleChat(int clientSocket, struct json_object *parsedJson);
void handleSurrender(int clientSocket, struct json_object *parsedJson);
#endif