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
int insertRoom2db(int userId)
{
    // tạo một room mới: và trả về id của room đó
    // insert vào bảng room, white_user_id là userId, status là 0(đang chờ)
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
    //trả về room_id 
    struct json_object *jroomId = json_object_new_int(roomId);
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(CREATE_ROOM));
    json_object_object_add(jobj, "status", json_object_new_int(1));
    json_object_object_add(jobj, "room_id", jroomId);
    const char *json_string = json_object_to_json_string(jobj);
    send(clientSocket, json_string, strlen(json_string), 0);
}