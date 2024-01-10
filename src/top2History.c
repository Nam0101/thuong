#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <sqlite3.h>
#include "dbconn.h"
#include "../include/type.h"
void handleGetTopScore(int clientSocket, struct json_object *parsed_json){
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