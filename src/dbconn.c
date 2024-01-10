#include "dbconn.h"
#include <stdio.h>


sqlite3 *openDatabase()
{
    sqlite3 *db;
    int rc = sqlite3_open(DB_NAME, &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db); 
        return NULL;
    }
    //open on WAL mode
    char *zErrMsg = 0;
    rc = sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db); 
        return NULL;
    }
    return db;
}

void closeDatabase(sqlite3 *db)
{
    if (db)
    {
        sqlite3_close(db);
    }
}
