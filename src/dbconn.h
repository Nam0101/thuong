#ifndef DBCONN_H
#define DBCONN_H

#include <sqlite3.h>
#define DB_NAME "src/db/chess.db"
// Function to open a connection to the SQLite database
sqlite3 *openDatabase();

// Function to close the SQLite database connection
void closeDatabase(sqlite3 *db);


#endif
