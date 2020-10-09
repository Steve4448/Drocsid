#include "ChatClient.h"
#include "Constants.h"
#include <iostream>
#include <string>
#include <exception>
#include "Packet/PacketHandler.h"
#include "Exception/StartupException.h"
#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;

/* Entry point for the client. */
int main() {
	ChatClient client;
	try {
		client.connectionDetailsPrompt();
		client.getConsoleRenderer()->pushBodyMessage("Disconnected from server.", ERROR_COLOR);
	} catch(StartupException& e) {
		cout << "Error starting the client: " << e.what();
		return 1;
	}
	return 0;
}

/* Client constructor, takes in the ip and the port that the client will attempt to connect to. */
ChatClient::ChatClient() :
	packetHandler(nullptr),
	cSocket(INVALID_SOCKET),
	username(""),
	password(""),
	ip(""),
	port(0),
	connected(false),
	consoleRenderer(new ConsoleHandler()),
	inRoom(nullptr),
	friendsList(nullptr),
	friendsListSize(0) {
}

void ChatClient::connectionDetailsPrompt() {
	unsigned short portNum = 0;
	string ipInput = "";
	string portInput = "";
	ipInput = consoleRenderer->getBlockingInput("Please enter the IP that the server is currently hosted at:");
	while(portNum == 0) {
		try {
			portInput = consoleRenderer->getBlockingInput("Please enter the port that the server is currently hosted at:");
			portNum = stoi(portInput);
		} catch(invalid_argument&) {
			consoleRenderer->pushBodyMessage("Please enter a valid port number.", ERROR_COLOR);
		}
	}
	if(ip.empty())
		ip = "127.0.0.1";
	ip = ipInput;
	port = portNum;

	start();
}

/* Starts the client and attempts to connect to the server
	Once connected it will start a thread to begin reading from the server.
	winsock code implemented using information from https://docs.microsoft.com/en-us/windows/desktop/winsock/winsock-server-application
*/
void ChatClient::start() {
	WSADATA wsaData;
	struct addrinfo socketSpecifications;
	struct addrinfo* socketResult = nullptr;
	int wsaResult;

	wsaResult = WSAStartup(DESIRED_WINSOCK_VERSION, &wsaData);
	if(wsaResult != 0) {
		throw StartupException("WSAStartup failed: ", wsaResult);
	}

	ZeroMemory(&socketSpecifications, sizeof(socketSpecifications));
	socketSpecifications.ai_family = AF_UNSPEC;
	socketSpecifications.ai_socktype = SOCK_STREAM;
	socketSpecifications.ai_protocol = IPPROTO_TCP;

	wsaResult = getaddrinfo(ip.c_str(), to_string(port).c_str(), &socketSpecifications, &socketResult);
	if(wsaResult != 0) {
		if(wsaResult == 11001) { //Invalid host name.
			consoleRenderer->pushBodyMessage("Invalid host.", ERROR_COLOR);
			connectionDetailsPrompt();
		} else {
			throw StartupException("getaddrinfo failed with error: ", wsaResult);
		}
	}

	cSocket = socket(socketResult->ai_family, socketResult->ai_socktype, socketResult->ai_protocol);
	if(cSocket == INVALID_SOCKET) {
		throw StartupException("socket failed with error: ", WSAGetLastError());
	}

	consoleRenderer->pushBodyMessage("Connecting...");

	wsaResult = connect(cSocket, socketResult->ai_addr, (int)socketResult->ai_addrlen);
	freeaddrinfo(socketResult);
	if(wsaResult == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		switch(errorCode) {
			case 10060: //Connection timed out.
				consoleRenderer->pushBodyMessage("Connection timed out.", ERROR_COLOR);
				connectionDetailsPrompt();
				return;
			case 10061: //Connection refused.
				consoleRenderer->pushBodyMessage("Connection actively refused.", ERROR_COLOR);
				connectionDetailsPrompt();
				return;
			default:
				throw StartupException("connect failed with error: ", WSAGetLastError());
		}
	}

	consoleRenderer->pushBodyMessage("Connected!");
	connected = true;
	packetHandler = new PacketHandler(this, cSocket);
	readInstance = thread(&PacketHandler::readLoop, packetHandler);
	Packet* p = packetHandler->constructPacket(HANDSHAKE_PACKET_ID);
	*p << VERSION_CODE;
	packetHandler->finializePacket(p, true);
	readInstance.join();
}

/* Sets the client to a disconnected state. */
void ChatClient::disconnect() {
	packetHandler->setConnected(false);
	connected = false;
}

/* Prompts the client for their username and password.
	If the person types in a space it will re-prompt their username.
	Once the input has been taken it will send the information to the server.
*/
void ChatClient::doCredentials(bool retryPassword) {
	if(retryPassword) {
		password = consoleRenderer->getBlockingInput("Please enter a password: ");
	} else {
		size_t spacePos = 0;
		while(spacePos != string::npos) {
			username = consoleRenderer->getBlockingInput("Please enter a username: ");

			spacePos = username.find(' ');
			if(spacePos != string::npos) {
				consoleRenderer->pushBodyMessage("You cannot use spaces in your username.", ERROR_COLOR);
			}
		}
		password = consoleRenderer->getBlockingInput("Please enter a password: ");
	}
	Packet* p = packetHandler->constructPacket(AUTHENTICATION_PACKET_ID);
	*p << username;
	*p << password;
	packetHandler->finializePacket(p, true);
}

/* Returns the client's username. */
string ChatClient::getUsername() {
	return username;
}

/* Returns the client's password. */
string ChatClient::getPassword() {
	return password;
}

/* Sets if the client is inside a room. */
void ChatClient::setInRoom(bool inRoom) {
	this->inRoom = inRoom;
}

/* Returns true if a user is in a room, otherwise false. */
bool ChatClient::isInRoom() {
	return inRoom;
}

/* Going to the lobby will begin the thread for input. */
void ChatClient::gotoLobby() {
	inputInstance = thread(&ChatClient::messagePrompt, this);
}

/* Loops continuously to take user input and sends it to the server. */
void ChatClient::messagePrompt() {
	while(connected) {
		string message = consoleRenderer->getBlockingInput();
		Packet* p = packetHandler->constructPacket(MESSAGE_PACKET_ID);
		*p << message;
		packetHandler->finializePacket(p, true);
	}
}

ConsoleHandler* const ChatClient::getConsoleRenderer() {
	return consoleRenderer;
}

void ChatClient::setFriendsList(unsigned short friendsListSize, Friend** const friendsList) {
	this->friendsListSize = friendsListSize;
	this->friendsList = friendsList;
}

bool ChatClient::isFriend(string name) {
	for(unsigned short i = 0; i < friendsListSize; i++) {
		if(friendsList[i] != nullptr && friendsList[i]->getName() == name) {
			return true;
		}
	}
	return false;
}

Friend** const ChatClient::getFriendsList() {
	return friendsList;
}

unsigned short ChatClient::getFriendsListSize() {
	return friendsListSize;
}

ChatClient::~ChatClient() {
	if(inputInstance.joinable())
		inputInstance.join();
	delete packetHandler;
	delete consoleRenderer;
	for(unsigned short i = 0; i < friendsListSize; i++) {
		delete friendsList[i];
	}
	delete[] friendsList;
	WSACleanup();
}