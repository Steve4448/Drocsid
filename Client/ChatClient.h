#ifndef CHAT_CLIENT_H_
#define CHAT_CLIENT_H_
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include "Packet/PacketHandler.h"
#include "UI/ConsoleHandler.h"
#include "Friend.h"

#pragma comment(lib, "Ws2_32.lib")

class ChatClient {
public:
	ChatClient();
	~ChatClient();
	void connectionDetailsPrompt();
	void start();
	void gotoLobby();
	void messagePrompt();
	void doCredentials(bool retryPassword = false);
	void disconnect();
	std::string getUsername();
	std::string getPassword();
	ConsoleHandler* getConsoleRenderer();
	void setInRoom(bool inRoom);
	bool isInRoom();
	void setFriendsList(unsigned short friendsListSize, Friend** friendsList);
	Friend** getFriendsList();
	bool isFriend(std::string name);
	unsigned short getFriendsListSize();
private:
	unsigned int port;
	std::string ip;
	SOCKET cSocket;
	std::thread readInstance;
	std::thread inputInstance;
	PacketHandler* packetHandler;
	std::string username;
	std::string password;
	unsigned short friendsListSize;
	Friend** friendsList;
	bool connected;
	bool inRoom;
	ConsoleHandler* consoleRenderer;

};
#endif //CHAT_CLIENT_H_