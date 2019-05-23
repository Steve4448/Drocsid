#ifndef SERVER_H_
#define SERVER_H_
#include "Constants.h"
#include <string>
#include <winsock2.h>
#include <vector>
class User;
class Room;

class Server {
public:
	Server(unsigned int port);
	~Server();
	bool start();
	void doListen();
	void removeUser(User * user);
	User ** getUserList();
	User * getUserByName(std::string name);
	Room ** getRoomList();
	Room * makeRoom(User * owner, std::string roomName);
	void destroyRoom(Room * room);
	void updateRoomList();
	void updateRoomList(User * user);
	std::vector<std::string> getRegisteredNameList();
	bool doesRegisteredUsernameExist(std::string username);
	void addRegisteredUsername(std::string username);
private:
	unsigned short roomCount;
	unsigned int port;
	SOCKET sSocket;
	bool listening;
	Room * roomList[MAX_ROOMS];
	User * userList[MAX_USERS];
	std::vector<std::string> registeredNameList;
};
#endif //SERVER_H_