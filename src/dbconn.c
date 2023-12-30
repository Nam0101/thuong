#include "dbconn.h"
#include <stdio.h>

// Function to open a connection to the SQLite database
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
    return db;
}

void closeDatabase(sqlite3 *db)
{
    sqlite3_close(db);
}
