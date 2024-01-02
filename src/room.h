#ifndef ROOM_H
#define ROOM_H
#include <json-c/json.h>

void handleCreateRoom(int clientSocket, struct json_object *parsedJson);
#endif