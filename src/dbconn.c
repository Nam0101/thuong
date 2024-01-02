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
        sqlite3_close(db); // Close the database in case of an error
        return NULL;
    }
    return db;
}

void closeDatabase(sqlite3 *db)
{
    if (db)
    {
        // Attempt to finalize any remaining prepared statements
        sqlite3_stmt *stmt;
        while ((stmt = sqlite3_next_stmt(db, NULL)) != NULL)
        {
            sqlite3_finalize(stmt);
        }

        int rc = sqlite3_close(db);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "Error closing database: %s\n", sqlite3_errmsg(db));
        }
    }
}
