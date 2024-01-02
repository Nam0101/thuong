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
#include <openssl/sha.h>
#include <string.h>
#include "../include/type.h"
#include "authentication.h"
#include "dbconn.h"
#define DEFAULT_SCORE 1000
void sha256(const char *input, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, strlen(input));
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }

    outputBuffer[64] = 0;
}
void handleLogin(int client_socket, struct json_object *parsed_json)
{
    printf("LOGIN\n");
    struct json_object *jusername;
    struct json_object *jpassword;
    json_object_object_get_ex(parsed_json, "username", &jusername);
    json_object_object_get_ex(parsed_json, "password", &jpassword);
    const char *inputUsername = json_object_get_string(jusername);
    const char *password = json_object_get_string(jpassword);
    // query database
    sqlite3 *db = openDatabase();
    if (db == NULL)
    {
        printf("Cannot open database\n");
        sendResponse(client_socket, LOGIN, 0, "SERVER ERROR");
        return;
    }
    char hashed_password[65];
    sha256(password, hashed_password);
    char *sql = "SELECT * FROM user WHERE username = ? AND password = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        sendResponse(client_socket, LOGIN, 0, "SERVER ERROR");
        sqlite3_finalize(stmt); // Finalize the prepared statement
        closeDatabase(db);
        return;
    }
    sqlite3_bind_text(stmt, 1, inputUsername, strlen(inputUsername), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_password, strlen(hashed_password), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        char loggedInUsername[50];
        int score;
        int user_id;
        strcpy(loggedInUsername, sqlite3_column_text(stmt, 1));
        score = sqlite3_column_int(stmt, 3);
        user_id = sqlite3_column_int(stmt, 0);
        if (sqlite3_column_int(stmt, 4) == 1)
        {
            printf("USER ALREADY LOGGED IN\n");
            struct json_object *jobj = json_object_new_object();
            json_object_object_add(jobj, "type", json_object_new_int(LOGIN));
            json_object_object_add(jobj, "status", json_object_new_int(0));
            json_object_object_add(jobj, "message", json_object_new_string("USER ALREADY LOGGED IN"));
            const char *json_string = json_object_to_json_string(jobj);
            send(client_socket, json_string, strlen(json_string), 0);
            sqlite3_finalize(stmt); // Finalize the prepared statement
            closeDatabase(db);
            return;
        }
        struct json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "type", json_object_new_int(LOGIN));
        json_object_object_add(jobj, "status", json_object_new_int(1));
        json_object_object_add(jobj, "message", json_object_new_string("LOGIN SUCCESS"));
        json_object_object_add(jobj, "user_id", json_object_new_int(user_id));
        json_object_object_add(jobj, "username", json_object_new_string(loggedInUsername));
        json_object_object_add(jobj, "score", json_object_new_int(score));
        const char *json_string = json_object_to_json_string(jobj);
        send(client_socket, json_string, strlen(json_string), 0);
        sql = "UPDATE user SET status = 1 WHERE username = ?";
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt); // Finalize the prepared statement

            closeDatabase(db);
            return;
        }
        sqlite3_bind_text(stmt, 1, inputUsername, strlen(inputUsername), SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            printf("Cannot step statement: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt); // Finalize the prepared statement

            closeDatabase(db);
            return;
        }
        sqlite3_finalize(stmt); // Finalize the prepared statement
        closeDatabase(db);
        return;
    }
    else
    {
        struct json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "type", json_object_new_int(LOGIN));
        json_object_object_add(jobj, "status", json_object_new_int(0));
        json_object_object_add(jobj, "message", json_object_new_string("LOGIN FAILED"));
        const char *json_string = json_object_to_json_string(jobj);
        send(client_socket, json_string, strlen(json_string), 0);
        sqlite3_finalize(stmt); // Finalize the prepared statement
        closeDatabase(db);
        return;
    }
    sqlite3_finalize(stmt); // Finalize the prepared statement
    closeDatabase(db);
}

struct json_object *createResponseObject(int type, int status, const char *message)
{
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "type", json_object_new_int(type));
    json_object_object_add(jobj, "status", json_object_new_int(status));
    json_object_object_add(jobj, "message", json_object_new_string(message));
    return jobj;
}

void sendResponse(int client_socket, int type, int status, const char *message)
{
    struct json_object *jobj = createResponseObject(type, status, message);
    const char *json_string = json_object_to_json_string(jobj);
    send(client_socket, json_string, strlen(json_string), 0);
}

void handleRegister(int client_socket, struct json_object *parsed_json)
{

    struct json_object *jusername;
    struct json_object *jpassword;
    json_object_object_get_ex(parsed_json, "username", &jusername);
    json_object_object_get_ex(parsed_json, "password", &jpassword);

    const char *username = json_object_get_string(jusername);
    const char *password = json_object_get_string(jpassword);
    char hashed_password[65];
    sha256(password, hashed_password);

    // get database
    sqlite3 *db = openDatabase();
    if (db == NULL)
    {
        printf("Cannot open database\n");
        sendResponse(client_socket, REGISTER, 0, "SERVER ERROR");
        return;
    }

    // check if username exists
    char *sql = "SELECT * FROM user WHERE username = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        struct json_object *jobj = createResponseObject(REGISTER, 0, "SERVER ERROR");
        const char *json_string = json_object_to_json_string(jobj);
        send(client_socket, json_string, strlen(json_string), 0);
        closeDatabase(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        struct json_object *jobj = createResponseObject(REGISTER, 0, "USERNAME ALREADY EXISTS");
        const char *json_string = json_object_to_json_string(jobj);
        send(client_socket, json_string, strlen(json_string), 0);
        closeDatabase(db);
        return;
    }

    // insert new user
    sql = "INSERT INTO user (username, password,score,status) VALUES (?, ?,?,0)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        sendResponse(client_socket, REGISTER, 0, "SERVER ERROR");
        closeDatabase(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_password, strlen(hashed_password), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, DEFAULT_SCORE);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        sendResponse(client_socket, REGISTER, 0, "SERVER ERROR");
        closeDatabase(db);
        return;
    }

    sendResponse(client_socket, REGISTER, 1, "REGISTER SUCCESS");
    closeDatabase(db);
}
void handleLogout(int client_socket, struct json_object *parsed_json)
{
    printf("LOGOUT\n");
    struct json_object *juserid;
    // get userid
    json_object_object_get_ex(parsed_json, "user_id", &juserid);
    int user_id = json_object_get_int(juserid);

    // query database
    sqlite3 *db = openDatabase();
    if (db == NULL)
    {
        printf("Cannot open database\n");
        sendResponse(client_socket, LOGOUT, 0, "SERVER ERROR");
        return;
    }
    sqlite3_stmt *stmt;
    char *sql = "UPDATE user SET status = 0 WHERE id = ?";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("Cannot prepare statement: %s\n", sqlite3_errmsg(db));
        sendResponse(client_socket, LOGOUT, 0, "SERVER ERROR");
        closeDatabase(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, user_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        printf("Cannot step statement: %s\n", sqlite3_errmsg(db));
        closeDatabase(db);
        return;
    }
    printf("LOGOUT SUCCESS\n");
}