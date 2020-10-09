#ifndef SERVER_H_
#define SERVER_H_
#include "Constants.h"
#include <string>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <regex>
#include <fstream>
class User;
class Room;

class Server {
public:
	Server(unsigned int port);
	~Server();
	bool start();
	void doListen();
	void removeUser(User* const user);
	User** const getUserList();
	User* const getUserByName(std::string name);
	Room** const getRoomList();
	Room* const makeRoom(User* owner, std::string roomName);
	void destroyRoom(Room* room);
	void handleFriendStatusUpdate(User* const user);
	void updateRoomList();
	void updateRoomList(User* const user);
	bool doesRegisteredUsernameExist(std::string username);
	std::string getProperUsernameCase(std::string username);
	bool isValidUsername(std::string username);
	void log(std::string line);
private:
	unsigned short roomCount;
	unsigned int port;
	SOCKET sSocket;
	bool listening;
	Room* roomList[MAX_ROOMS];
	User* userList[MAX_USERS];
	std::regex allowedUsernameChars;
	std::ofstream logFile;
};
#endif //SERVER_H_