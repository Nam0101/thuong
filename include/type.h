#ifndef TYPE_H
#define TYPE_H
typedef enum
{
    LOGIN,       // login request for both client->server and server->client
    REGISTER,    // register request for both client->server and server->client
    LOGOUT,      // logout request for both client->server
    CREATE_ROOM, // create room request for both client->server
    JOIN_ROOM,
    GET_ROOM_LIST,
    GET_TOP_SCORE,
    MOVE,
    SURRENDER,
    OUT_ROOM,
    END_GAME,   // end game for both user, both user send this request
    UPDATE_SCORE,
    NOTIFI_JOIN_ROOM,
    CHAT,
    GET_HISTORY,
    ADD_FRIEND,
    CHALLENGE,
    NOTIFI_CHALLENGE,
    START_GAME_BY_CHALLENGE,
    GET_LIST_ONLINE_USER
} type;
typedef enum
{
    LOSE,
    WIN,
    DRAW,
} result;
#endif // TYPE_H