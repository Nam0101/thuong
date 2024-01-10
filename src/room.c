#include "room.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    addWhiteUser(roomId, whiteUser);
}
void handleJoinRoom(int clientSocket, struct json_object *parsedJson)
{
    printf("%s\n", json_object_to_json_string(parsedJson));
    int roomId;
    struct json_object *jroomId;
    json_object_object_get_ex(parsedJson, "room_id", &jroomId);
    roomId = json_object_get_int(jroomId);
    struct json_object *juserId;
    json_object_object_get_ex(parsedJson, "user_id", &juserId);
    int userId = json_object_get_int(juserId);
    room *current_room = getRoomById(roomId);

    if (current_room == NULL)
    {
        sendResponse(clientSocket, JOIN_ROOM, 0, "Room not found");
        return;
    }
    if (current_room->status == 1)
    {
        sendResponse(clientSocket, JOIN_ROOM, 0, "Room is full");
        return;
    }
    current_room->status = 1;
    user *blackUser = getUserByID(userId);
    if(blackUser == NULL){
        sendResponse(clientSocket, JOIN_ROOM, 0, "SERVER ERROR");
        return;
    }
    // send infomation of white user to black user
    struct json_object *jwhiteUser = json_object_new_object();
    json_object_object_add(jwhiteUser, "username", json_object_new_string(current_room->white_user->username));
    json_object_object_add(jwhiteUser, "user_id", json_object_new_int(current_room->white_user->user_id));
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(JOIN_ROOM));
    addBlackUser(current_room->room_id, blackUser);
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "white_user", jwhiteUser);
    const char *json_string = json_object_to_json_string(jobj);
    send(clientSocket, json_string, strlen(json_string), 0);
    // send infomation of black user to white user
    struct json_object *jblackUser = json_object_new_object();
    json_object_object_add(jblackUser, "username", json_object_new_string(current_room->black_user->username));
    json_object_object_add(jblackUser, "user_id", json_object_new_int(current_room->black_user->user_id));
    jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(JOIN_ROOM));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "black_user", jblackUser);
    json_string = json_object_to_json_string(jobj);
    send(current_room->white_user->socket, json_string, strlen(json_string), 0);
}
void handleGetRoomList(int clientSocket, struct json_object *parsedJson)
{   
    printf("%s\n", json_object_to_json_string(parsedJson));
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(GET_ROOM_LIST));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "room_list", json_object_new_array());
    struct json_object *jarray = json_object_object_get(jobj, "room_list");
    pthread_mutex_lock(&roomListMutex);
    room *tmp = roomList;
    while (tmp != NULL)
    {
        struct json_object *jroom = json_object_new_object();
        json_object_object_add(jroom, "room_id", json_object_new_int(tmp->room_id));
        json_object_object_add(jroom, "status", json_object_new_int(tmp->status));
        // check if black user or white user is NULL then add NULL to json
        if(tmp->black_user == NULL){
            json_object_object_add(jroom, "black_user", json_object_new_string("NULL"));
        }else{
            json_object_object_add(jroom, "black_user", json_object_new_string(tmp->black_user->username));
        }
        if(tmp->white_user == NULL){
            json_object_object_add(jroom, "white_user", json_object_new_string("NULL"));
        }else{
            json_object_object_add(jroom, "white_user", json_object_new_string(tmp->white_user->username));
        }
        json_object_array_add(jarray, jroom);
        tmp = tmp->next;
    }
    pthread_mutex_unlock(&roomListMutex);
    const char *json_string = json_object_to_json_string(jobj);
    printf("%s\n", json_string);
    send(clientSocket, json_string, strlen(json_string), 0);

}