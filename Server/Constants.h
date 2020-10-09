#ifndef CONSTANTS_H_
#define CONSTANTS_H_
#define MAX_USERS 100
#define MAX_ROOMS 10
#define MAX_ROOM_USERS 10
#define MAX_FRIENDS 10
#define DESIRED_WINSOCK_VERSION MAKEWORD(2, 2)
#define BUFFER_LENGTH 4096
#define PACKET_COUNT 10
#define HANDSHAKE_PACKET_ID 0
#define AUTHENTICATION_PACKET_ID 1
#define AUTHENTICATION_INVALID_PASSWORD 0
#define AUTHENTICATION_NAME_IN_USE 1
#define AUTHENTICATION_SUCCESS 2
#define AUTHENTICATION_FAILURE 3
#define AUTHENTICATION_INVALID_USERNAME 4
#define MESSAGE_PACKET_ID 3
#define ATTEMPT_JOIN_ROOM_PACKET_ID 4
#define ATTEMPT_JOIN_ROOM_SUCCESS 0
#define ATTEMPT_JOIN_ROOM_FAILURE 1
#define LEAVE_ROOM_PACKET_ID 5
#define ROOM_STATUS_UPDATE_PACKET_ID 6
#define UPDATE_ROOM_LIST_PACKET_ID 7
#define SERVER_MESSAGE_PACKET_ID 8
#define ADD_FRIEND_PACKET_ID 9
#define REMOVE_FRIEND_PACKET_ID 10
#define FRIEND_STATUS_PACKET_ID 11
#define FRIENDS_LIST_PACKET_ID 12

#define LOG_DIRECTORY "./"
#define SAVE_DIRECTORY "./users/"
#define LOAD_SUCCESS 0
#define LOAD_FAILURE 1
#define LOAD_NEW_USER 2

#define ERROR_COLOR 12
#define FRIEND_COLOR 10
#define FRIEND_OFFLINE_COLOR 8
#define DEFAULT_COLOR 15
#define DEFAULT_CHAT_COLOR 11

#define VERSION_CODE "Drocsid 0.3"
#endif //CONSTANTS_H_