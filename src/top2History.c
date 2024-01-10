#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <sqlite3.h>
#include "dbconn.h"
#include "../include/type.h"
void handleGetTopScore(int clientSocket, struct json_object *parsed_json)
{
    sqlite3 *db = openDatabase();
    char *sql = "SELECT username,score FROM user ORDER BY score DESC LIMIT 10";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Error: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    json_object *jarray = json_object_new_array();
    while (1)
    {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE)
        {
            break;
        }
        if (rc != SQLITE_ROW)
        {
            printf("Error: %s\n", sqlite3_errmsg(db));
            closeDatabase(db);
            return;
        }
        json_object *jobj = json_object_new_object();
        json_object *jusername = json_object_new_string((char *)sqlite3_column_text(stmt, 0));
        json_object *jscore = json_object_new_int(sqlite3_column_int(stmt, 1));
        json_object_object_add(jobj, "username", jusername);
        json_object_object_add(jobj, "score", jscore);
        json_object_array_add(jarray, jobj);
    }
    json_object *jresponse = json_object_new_object();
    json_object_object_add(jresponse, "type", json_object_new_int(GET_TOP_SCORE));
    json_object_object_add(jresponse, "data", jarray);
    const char *response = json_object_to_json_string(jresponse);
    send(clientSocket, response, strlen(response), 0);
}
void handleGetHistory(int clientSocket, struct json_object *parsed_json)
{
    sqlite3 *db = openDatabase();
    char *sql = "SELECT game.id AS game_id,\
        white_user.username AS white_username,\
        black_user.username AS black_username,\
        CASE\
            WHEN game.winner = 1 then 'WIN' WHEN game.winner = 0 then 'DRAW' else 'LOSS' END AS result\
        FROM game JOIN user AS white_user ON game.white_id = white_user.id\
        JOIN user AS black_user ON game.black_id = black_user.id;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Error: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    json_object *jarray = json_object_new_array();
    while (1)
    {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE)
        {
            break;
        }
        if (rc != SQLITE_ROW)
        {
            printf("Error: %s\n", sqlite3_errmsg(db));
            closeDatabase(db);
            return;
        }
        json_object *jobj = json_object_new_object();
        json_object *jgame_id = json_object_new_int(sqlite3_column_int(stmt, 0));
        json_object *jwhite_username = json_object_new_string((char *)sqlite3_column_text(stmt, 1));
        json_object *jblack_username = json_object_new_string((char *)sqlite3_column_text(stmt, 2));
        json_object *jresult = json_object_new_string((char *)sqlite3_column_text(stmt, 3));
        json_object_object_add(jobj, "game_id", jgame_id);
        json_object_object_add(jobj, "white_username", jwhite_username);
        json_object_object_add(jobj, "black_username", jblack_username);
        json_object_object_add(jobj, "result", jresult);
        json_object_array_add(jarray, jobj);
    }
    json_object *jresponse = json_object_new_object();
    json_object_object_add(jresponse, "type", json_object_new_int(GET_HISTORY));
    json_object_object_add(jresponse, "data", jarray);
    const char *response = json_object_to_json_string(jresponse);
    send(clientSocket, response, strlen(response), 0);
}