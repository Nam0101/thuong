#include "room.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include "dbconn.h"
#include "../include/type.h"
#include <json-c/json.h>
#include <sqlite3.h>
#include "authentication.h"
#include <pthread.h>
#include <time.h>
#define DEFAULT_K 32
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
    if (whiteUser == NULL)
    {
        printf("Cannot get user\n");
        sendResponse(clientSocket, CREATE_ROOM, 0, "SERVER ERROR");
        return;
    }
    // add room to roomList
    room *newRoom = createRoom(roomId, 0, NULL, NULL);
    if (newRoom == NULL)
    {
        printf("Cannot create room\n");
        sendResponse(clientSocket, CREATE_ROOM, 0, "SERVER ERROR");
        return;
    }
    addRoom(newRoom);
    addWhiteUser(roomId, whiteUser);
}
void handleJoinRoom(int clientSocket, struct json_object *parsedJson)
{
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
    if (blackUser == NULL)
    {
        sendResponse(clientSocket, JOIN_ROOM, 0, "SERVER ERROR");
        return;
    }
    if (current_room->white_user == NULL)
    {
        addWhiteUser(roomId, blackUser);
    }
    else
    {
        addBlackUser(roomId, blackUser);
    }
    struct json_object *jwhiteUser = json_object_new_object();
    json_object_object_add(jwhiteUser, "username", json_object_new_string(current_room->white_user->username));
    json_object_object_add(jwhiteUser, "user_id", json_object_new_int(current_room->white_user->user_id));
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(NOTIFI_JOIN_ROOM));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "white_user", jwhiteUser);
    const char *json_string = json_object_to_json_string(jobj);
    send(current_room->black_user->socket, json_string, strlen(json_string), 0);

    struct json_object *jblackUser = json_object_new_object();
    json_object_object_add(jblackUser, "username", json_object_new_string(current_room->black_user->username));
    json_object_object_add(jblackUser, "user_id", json_object_new_int(current_room->black_user->user_id));
    jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(NOTIFI_JOIN_ROOM));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "black_user", jblackUser);
    json_string = json_object_to_json_string(jobj);
    send(current_room->white_user->socket, json_string, strlen(json_string), 0);
}
void handleLeaveRoom(int client_socket, struct json_object *parsedJson)
{
    int roomId, userId;
    struct json_object *jroomId;
    json_object_object_get_ex(parsedJson, "room_id", &jroomId);
    roomId = json_object_get_int(jroomId);
    struct json_object *juserId;
    json_object_object_get_ex(parsedJson, "user_id", &juserId);
    userId = json_object_get_int(juserId);
    room *current_room = getRoomById(roomId);
    if (current_room == NULL)
    {
        sendResponse(client_socket, OUT_ROOM, 0, "Room not found");
        return;
    }
    if (current_room->white_user->user_id == userId)
    {
        current_room->white_user = NULL;
    }
    else if (current_room->black_user->user_id == userId)
    {
        current_room->black_user = NULL;
    }
    else
    {
        sendResponse(client_socket, OUT_ROOM, 0, "User not found");
        return;
    }
    if (current_room->white_user == NULL && current_room->black_user == NULL)
    {
        removeRoom(roomId);
    }
    if (current_room->white_user != NULL)
    {
        // send message to white user that black user leave room
        struct json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "type", json_object_new_int(OUT_ROOM));
        json_object_object_add(jobj, "status", json_object_new_int(1));
        json_object_object_add(jobj, "message", json_object_new_string("Black user leave room"));
        const char *json_string = json_object_to_json_string(jobj);
        send(current_room->white_user->socket, json_string, strlen(json_string), 0);
    }
    if (current_room->black_user != NULL)
    {
        // send message to black user that white user leave room
        struct json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "type", json_object_new_int(OUT_ROOM));
        json_object_object_add(jobj, "status", json_object_new_int(1));
        json_object_object_add(jobj, "message", json_object_new_string("White user leave room"));
        const char *json_string = json_object_to_json_string(jobj);
        send(current_room->black_user->socket, json_string, strlen(json_string), 0);
    }
    current_room->status = 0;
    sendResponse(client_socket, OUT_ROOM, 1, "Leave room success");
}
void handleGetRoomList(int clientSocket, struct json_object *parsedJson)
{
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
        if (tmp->black_user == NULL)
        {
            json_object_object_add(jroom, "black_user", json_object_new_string("NULL"));
        }
        else
        {
            json_object_object_add(jroom, "black_user", json_object_new_string(tmp->black_user->username));
        }
        if (tmp->white_user == NULL)
        {
            json_object_object_add(jroom, "white_user", json_object_new_string("NULL"));
        }
        else
        {
            json_object_object_add(jroom, "white_user", json_object_new_string(tmp->white_user->username));
        }
        json_object_array_add(jarray, jroom);
        tmp = tmp->next;
    }
    pthread_mutex_unlock(&roomListMutex);
    const char *json_string = json_object_to_json_string(jobj);
    send(clientSocket, json_string, strlen(json_string), 0);
}
void handleMove(int clientSocket, struct json_object *parsedJson)
{
    printf("%s\n", json_object_to_json_string(parsedJson));
    int roomId;
    struct json_object *jroomId;
    json_object_object_get_ex(parsedJson, "room_id", &jroomId);
    roomId = json_object_get_int(jroomId);
    room *current_room = getRoomById(roomId);
    if (current_room == NULL)
    {
        sendResponse(clientSocket, MOVE, 0, "Room not found");
        return;
    }
    struct json_object *juserId;
    json_object_object_get_ex(parsedJson, "user_id", &juserId);
    int userId = json_object_get_int(juserId);
    if (current_room->black_user->user_id == userId)
    {
        send(current_room->white_user->socket, json_object_to_json_string(parsedJson), strlen(json_object_to_json_string(parsedJson)), 0);
    }
    else if (current_room->white_user->user_id == userId)
    {
        send(current_room->black_user->socket, json_object_to_json_string(parsedJson), strlen(json_object_to_json_string(parsedJson)), 0);
    }
    else
    {
        sendResponse(clientSocket, MOVE, 0, "User not found");
        return;
    }
}
void updateStartGame(room *room)
{
    int roomId = room->room_id;
    int white_id = room->white_user->user_id;
    int black_id = room->black_user->user_id;
    // get current time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char *time = asctime(timeinfo);
    time[strlen(time) - 1] = '\0';
    // update on db
    sqlite3 *db = openDatabase();
    char *sql = "UPDATE game SET status = 1, time_start = ?, black_id = ?, white_id = ? WHERE id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    sqlite3_bind_text(stmt, 1, time, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, black_id);
    sqlite3_bind_int(stmt, 3, white_id);
    sqlite3_bind_int(stmt, 4, roomId);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        printf("Cannot step statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    closeDatabase(db);
}
void handleStartGame(int clientSocket, struct json_object *parsedJson)
{
    int roomId;
    struct json_object *jroomId;
    json_object_object_get_ex(parsedJson, "room_id", &jroomId);
    roomId = json_object_get_int(jroomId);
    room *current_room = getRoomById(roomId);
    if (current_room == NULL)
    {
        sendResponse(clientSocket, START_GAME, 0, "Room not found");
        return;
    }
    // update on db
    updateStartGame(current_room);
    struct json_object *juserId;
    json_object_object_get_ex(parsedJson, "user_id", &juserId);
    int userId = json_object_get_int(juserId);
    if (current_room->black_user->user_id == userId)
    {
        send(current_room->white_user->socket, json_object_to_json_string(parsedJson), strlen(json_object_to_json_string(parsedJson)), 0);
    }
    else if (current_room->white_user->user_id == userId)
    {
        send(current_room->black_user->socket, json_object_to_json_string(parsedJson), strlen(json_object_to_json_string(parsedJson)), 0);
    }
    else
    {
        sendResponse(clientSocket, START_GAME, 0, "Room is not enough player");
        return;
    }
}
void calculationScore(double *Ra, double *Rb, double Sa, double Sb, double K)
{
    double Ea = 1 / (1 + pow(10, (*Rb - *Ra) / 400));
    double Eb = 1 / (1 + pow(10, (*Ra - *Rb) / 400));

    *Ra = *Ra + K * (Sa - Ea);
    *Rb = *Rb + K * (Sb - Eb);
}

void handleEndGame(int client_socket, struct json_object *parsedJson)
{
    int user_id, room_id;
    result result;
    struct json_object *juserId;
    json_object_object_get_ex(parsedJson, "user_id", &juserId);
    user_id = json_object_get_int(juserId);
    struct json_object *jroomId;
    json_object_object_get_ex(parsedJson, "room_id", &jroomId);
    room_id = json_object_get_int(jroomId);
    struct json_object *jresult;
    json_object_object_get_ex(parsedJson, "result", &jresult);
    result = json_object_get_int(jresult);
    room *current_room = getRoomById(room_id);
    if (current_room == NULL)
    {
        sendResponse(client_socket, END_GAME, 0, "Room not found");
        return;
    }
    double white_score = current_room->white_user->score;
    double black_score = current_room->black_user->score;
    double K = DEFAULT_K;
    double Sa, Sb;
    switch (result)
    {
    case WIN:
        if (user_id == current_room->white_user->user_id) {
            Sa = 1;
            Sb = 0;
        } else {
            Sa = 0;
            Sb = 1;
        }
        break;
    case LOSE:
        if (user_id == current_room->white_user->user_id) {
            Sa = 0;
            Sb = 1;
        } else {
            Sa = 1;
            Sb = 0;
        }
        break;
    case DRAW:
        Sa = 0.5;
        Sb = 0.5;
        break;
    default:
        return;
    }
    calculationScore(&white_score, &black_score, Sa, Sb, K);
    int int_white_score = (int)white_score;
    int int_black_score = (int)black_score;
    current_room->white_user->score = int_white_score;
    current_room->black_user->score = int_black_score;
    // update on db
    sqlite3 *db = openDatabase();
    char *sql = "UPDATE user SET score = ? WHERE id = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, int_white_score);
    sqlite3_bind_int(stmt, 2, current_room->white_user->user_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        printf("Cannot step statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, int_black_score);
    sqlite3_bind_int(stmt, 2, current_room->black_user->user_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        printf("Cannot step statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    closeDatabase(db);
    // send message to white user
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(UPDATE_SCORE));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "score", json_object_new_double(white_score));
    const char *json_string = json_object_to_json_string(jobj);
    send(current_room->white_user->socket, json_string, strlen(json_string), 0);
    // send message to black user
    jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(UPDATE_SCORE));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "score", json_object_new_double(black_score));
    json_string = json_object_to_json_string(jobj);
    send(current_room->black_user->socket, json_string, strlen(json_string), 0);

    //update end time and status on db
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char *time = asctime(timeinfo);
    time[strlen(time) - 1] = '\0';
    db = openDatabase();
    sql = "UPDATE game SET status = 2, time_end = ?,winner=? WHERE id = ?";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    sqlite3_bind_text(stmt, 1, time, -1, SQLITE_STATIC);
    int winner_id;
    if(result == WIN){
        winner_id = user_id;
    }else if(result == LOSE){
        winner_id = user_id;
    }
    else{
        winner_id = 0;
    }
    sqlite3_bind_int(stmt, 2, winner_id);
    sqlite3_bind_int(stmt, 3, room_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        printf("Cannot step statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    closeDatabase(db);
}