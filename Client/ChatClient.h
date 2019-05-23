#ifndef CHAT_CLIENT_H_
#define CHAT_CLIENT_H_
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include "PacketHandler.h"
#include "ConsoleHandler.h"

#pragma comment(lib, "Ws2_32.lib")

class ChatClient {
public:
	ChatClient();
	~ChatClient();
	bool start();
	void gotoLobby();
	void messagePrompt();
	void doCredentials(bool retryPassword = false);
	void disconnect();
	std::string getUsername();
	std::string getPassword();
	ConsoleHandler * getConsoleRenderer();
private:
	unsigned int port;
	std::string ip;
	SOCKET cSocket;
	std::thread readInstance;
	std::thread inputInstance;
	PacketHandler * packetHandler;
	std::string username;
	std::string password;
	bool connected;
	ConsoleHandler * consoleRenderer;

};
#endif //CHAT_CLIENT_H_