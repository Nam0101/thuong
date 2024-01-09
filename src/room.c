#include "room.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "dbconn.h"
#include "../include/type.h"
#include <json-c/json.h>
#include <sqlite3.h>
#include "authentication.h"
#include <pthread.h>
room *roomList = NULL;
pthread_mutex_t roomListMutex = PTHREAD_MUTEX_INITIALIZER;
void addRoom(room *newRoom)
{
    pthread_mutex_lock(&roomListMutex);
    if (roomList == NULL)
    {
        roomList = newRoom;
        pthread_mutex_unlock(&roomListMutex);
        return;
    }
    room *tmp = roomList;
    while (tmp->next != NULL)
    {
        tmp = tmp->next;
    }
    tmp->next = newRoom;
    pthread_mutex_unlock(&roomListMutex);
}
void removeRoom(int roomId)
{
    pthread_mutex_lock(&roomListMutex);
    if (roomList == NULL)
    {        
        pthread_mutex_unlock(&roomListMutex);
        return;
    }
    if (roomList->room_id == roomId)
    {
        room *tmp = roomList;
        roomList = roomList->next;
        free(tmp);
        pthread_mutex_unlock(&roomListMutex);
        return;
    }
    room *tmp = roomList;
    while (tmp->next != NULL)
    {
        if (tmp->next->room_id == roomId)
        {
            room *tmp2 = tmp->next;
            tmp->next = tmp->next->next;
            free(tmp2);
            break;
        }
        tmp = tmp->next;
    }
    pthread_mutex_unlock(&roomListMutex);
}
void addBlackUser(int roomId, user *user)
{
    pthread_mutex_lock(&roomListMutex);
    room *tmp = roomList;
    while (tmp != NULL)
    {
        if (tmp->room_id == roomId)
        {
            break;
        }
        tmp = tmp->next;
    }
    if (tmp == NULL)
    {
        pthread_mutex_unlock(&roomListMutex);
        return;
    }
    tmp->black_user = user;
    pthread_mutex_unlock(&roomListMutex);
}
void addWhiteUser(int roomId, user *user)
{
    pthread_mutex_lock(&roomListMutex);
    room *tmp = roomList;
    while (tmp != NULL)
    {
        if (tmp->room_id == roomId)
        {
            break;
        }
        tmp = tmp->next;
    }
    if (tmp == NULL)
    {
        pthread_mutex_unlock(&roomListMutex);
        return;
    }
    tmp->white_user = user;
    pthread_mutex_unlock(&roomListMutex);
}
room *getRoomById(int roomId)
{
    pthread_mutex_lock(&roomListMutex);
    room *tmp = roomList;
    while (tmp != NULL)
    {
        if (tmp->room_id == roomId)
        {
            break;
        }
        tmp = tmp->next;
    }
    pthread_mutex_unlock(&roomListMutex);
    return tmp;
}
int insertRoom2db(int userId)
{
    sqlite3 *db = openDatabase();
    char *sql = "INSERT INTO game (white_id, status) VALUES (?, 0)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_int(stmt, 1, userId);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        printf("Cannot step statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    int roomId = sqlite3_last_insert_rowid(db);
    closeDatabase(db);
    return roomId;
}
room *createRoom(int roomId, int status, user *black_user, user *white_user)
{
    room *newRoom = (room *)malloc(sizeof(room));
    newRoom->room_id = roomId;
    newRoom->status = status;
    newRoom->black_user = black_user;
    newRoom->white_user = white_user;
    newRoom->next = NULL;
    return newRoom;
}
void handleCreateRoom(int clientSocket, struct json_object *parsedJson)
{
    // get user_id
    struct json_object *juserId;
    json_object_object_get_ex(parsedJson, "user_id", &juserId);
    int userId = json_object_get_int(juserId);
    int roomId = insertRoom2db(userId);
    if (roomId == -1)
    {
        printf("Cannot create room\n");
        sendResponse(clientSocket, CREATE_ROOM, 0, "SERVER ERROR");
        return;
    }
    // trả về room_id
    struct json_object *jroomId = json_object_new_int(roomId);
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(CREATE_ROOM));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "room_id", jroomId);
    const char *json_string = json_object_to_json_string(jobj);
    send(clientSocket, json_string, strlen(json_string), 0);
    user *whiteUser = getUserByID(userId);
    if(whiteUser == NULL){
        printf("Cannot get user\n");
        sendResponse(clientSocket, CREATE_ROOM, 0, "SERVER ERROR");
        return;
    }
    // add room to roomList
    room *newRoom = createRoom(roomId, 0, NULL, NULL);
    if(newRoom == NULL){
        printf("Cannot create room\n");
        sendResponse(clientSocket, CREATE_ROOM, 0, "SERVER ERROR");
        return;
    }
    addRoom(newRoom);
    printf("white user: %s\n", whiteUser->username);
    printf("add room %d to roomList\n", roomId);
    addWhiteUser(roomId, whiteUser);
}
void handleJoinRoom(int clientSocket, struct json_object *parsedJson)
{
    printf("%s\n", json_object_to_json_string(parsedJson));
    printf("handleJoinRoom\n");
    int roomId;
    struct json_object *jroomId;
    json_object_object_get_ex(parsedJson, "room_id", &jroomId);
    roomId = json_object_get_int(jroomId);
    struct json_object *juserId;
    json_object_object_get_ex(parsedJson, "user_id", &juserId);
    printf("userId: %d\n", json_object_get_int(juserId));
    printf("roomId: %d\n", roomId  );
    int userId = json_object_get_int(juserId);
    room *current_room = getRoomById(roomId);

    if (current_room == NULL)
    {
        printf("Room not found\n");
        sendResponse(clientSocket, JOIN_ROOM, 0, "Room not found");
        return;
    }
    printf("current_room->status: %d\n", current_room->status);
    if (current_room->status == 1)
    {
        printf("Room is full\n");
        sendResponse(clientSocket, JOIN_ROOM, 0, "Room is full");
        return;
    }
    current_room->status = 1;
    user *blackUser = getUserByID(userId);
    if(blackUser == NULL){
        printf("Cannot get user\n");
        sendResponse(clientSocket, JOIN_ROOM, 0, "SERVER ERROR");
        return;
    }
    addBlackUser(userId, blackUser);
    printf("white user: %s\n", current_room->white_user->username);
    printf("black user: %s\n", current_room->black_user->username);
    sendResponse(clientSocket, JOIN_ROOM, 1, "Join room successfully");
}