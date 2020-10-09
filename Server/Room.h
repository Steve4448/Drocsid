#ifndef ROOM_H_
#define ROOM_H_
#include "Constants.h"
#include <string>
class User;
class Server;

class Room {
public:
	Room(Server* server, User* owner, std::string roomName);
	~Room();
	void joinRoom(User* user);
	void leaveRoom(User* user);
	User* getOwner();
	std::string getName();
	User** getUserList();
	void updateRoomList();
	void updateRoomList(User* user);
	unsigned short getUserCount();
	void ensureEmpty();
	void sendMessage(User* user, std::string message);
private:
	Server* server;
	User* owner;
	std::string roomName;
	User* userList[MAX_ROOM_USERS];
	unsigned short userCount;
};
#endif