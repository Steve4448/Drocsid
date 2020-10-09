#ifndef ROOM_H_
#define ROOM_H_
#include "Constants.h"
#include <string>
class User;
class Server;

class Room {
public:
	Room(Server* const server, User* const owner, std::string roomName);
	~Room();
	void joinRoom(User* const user);
	void leaveRoom(User* const user);
	User* const getOwner();
	std::string getName();
	User** const getUserList();
	void updateRoomList();
	void updateRoomList(User* const user);
	unsigned short getUserCount();
	void ensureEmpty();
	void sendMessage(User* const user, std::string message);
private:
	Server* const server;
	User* const owner;
	std::string roomName;
	User* userList[MAX_ROOM_USERS];
	unsigned short userCount;
};
#endif