#include "Server.h"
#include "User.h"
#include "Room.h"
#include "Constants.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "StartupException.h"

/* Library required for winsock usage. */
#pragma comment (lib, "Ws2_32.lib")
using namespace std;

/* Entry point for the server, takes in the port to run at and constructs and begins running the server. */
int main() {
	unsigned short portNum = 0;
	string port = "";
	cout << "Welcome to Drocsid." << endl << endl;
	cout << "Please enter the port that the server will be hosted at: ";
	while (portNum == 0) {
		try {
			cin >> port;
			portNum = stoi(port);
			cin.ignore();
		}
		catch (invalid_argument &) {
			cout << "Please enter a valid port number: ";
		}
	}
	Server server(portNum);
	try {
		if(server.start())
			server.doListen();
	} catch (StartupException & e) {
		cerr << "Error starting the server: " << e.what() << endl;
	}
	return 0;
}

Server::Server(unsigned int port) : port(port), userList{ nullptr }, roomList{ nullptr }, sSocket(INVALID_SOCKET), listening(true), roomCount(0) {}

/* Starts the server
	winsock code implemented using information from https://docs.microsoft.com/en-us/windows/desktop/winsock/winsock-server-application
*/
bool Server::start() {
	cout << "Server starting." << endl;
	WSADATA wsaData;
	struct addrinfo socketSpecifications;
	struct addrinfo * socketResult = nullptr;
	int wsaResult;

	wsaResult = WSAStartup(DESIRED_WINSOCK_VERSION, &wsaData);
	if (wsaResult != 0) {
		throw StartupException("WSAStartup failed: ", wsaResult);
	}

	ZeroMemory(&socketSpecifications, sizeof(socketSpecifications));
	socketSpecifications.ai_family = AF_INET;
	socketSpecifications.ai_socktype = SOCK_STREAM;
	socketSpecifications.ai_protocol = IPPROTO_TCP;
	socketSpecifications.ai_flags = AI_PASSIVE;

	wsaResult = getaddrinfo(NULL, to_string(port).c_str(), &socketSpecifications, &socketResult);
	if (wsaResult != 0) {
		throw StartupException("getaddrinfo failed with error: ", wsaResult);
	}
	
	sSocket = socket(socketResult->ai_family, socketResult->ai_socktype, socketResult->ai_protocol);
	if (sSocket == INVALID_SOCKET) {
		throw StartupException("socket failed with error: ", WSAGetLastError());
	}

	wsaResult = ::bind(sSocket, socketResult->ai_addr, (int)socketResult->ai_addrlen);
	freeaddrinfo(socketResult);
	if (wsaResult == SOCKET_ERROR) {
		throw StartupException("bind failed with error: ", WSAGetLastError());
	}

	wsaResult = listen(sSocket, SOMAXCONN);
	if (wsaResult == SOCKET_ERROR) {
		throw StartupException("bind failed with error: ", WSAGetLastError());
	}
	cout << "Server now listening." << endl;
	return true;
}

/* Listens for connections and upon a connection constructs a user. */
void Server::doListen() {
	while (listening) {
		unsigned short slot = -1;
		SOCKET userSocket = accept(sSocket, NULL, NULL);
		if (userSocket == INVALID_SOCKET) {
			cout << "accept failed with error: " << WSAGetLastError();
			continue;
		}

		for (unsigned short i = 0; i < MAX_USERS; i++) {
			if (userList[i] == nullptr) {
				slot = i;
				break;
			}
		}
		if (slot != -1) {
			userList[slot] = new User(this, slot, userSocket);
			cout << "New user connected." << endl;
		} else {
			cout << "User attempted a connection but the server is full." << endl;
			closesocket(userSocket);
		}
	}
}

/* Returns the servers room list array. */
Room ** Server::getRoomList() {
	return roomList;
}

/* Attempts to make a room
	- Returns nullptr if there is no room spaces left.
	- Returns address of room if a room was successfully made. */
Room * Server::makeRoom(User * owner, string roomName) {
	unsigned short slot = -1;
	for (unsigned short i = 0; i < MAX_ROOMS; i++) {
		if (roomList[i] == nullptr) {
			slot = i;
			break;
		}
	}
	if (slot != -1) {
		roomCount++;
		return (roomList[slot] = new Room(this, owner, roomName));
	}
	return nullptr;
}

/* Destroys the room, first ensures everyone has left it. */
void Server::destroyRoom(Room * room) {
	for (unsigned short i = 0; i < MAX_ROOMS; i++) {
		if (roomList[i] == room) {
			roomList[i]->ensureEmpty();
			delete roomList[i];
			roomList[i] = nullptr;
			roomCount--;
		}
	}
	updateRoomList();
}

/* Sends a room list update to everyone connected. */
void Server::updateRoomList() {
	for (unsigned short i = 0; i < MAX_USERS; i++) {
		if (userList[i] == nullptr)
			continue;
		updateRoomList(userList[i]);
	}
}

/* Sends a room list update to a specific user. */
void Server::updateRoomList(User * user) {
	Packet * p = user->getPacketHandler()->constructPacket(ROOM_STATUS_UPDATE_PACKET_ID);
	*p << roomCount;
	for (unsigned short i2 = 0; i2 < MAX_ROOMS; i2++) {
		if (roomList[i2] == nullptr)
			continue;
		*p << roomList[i2]->getName();
	}
	user->getPacketHandler()->finializePacket(p);
}

/* Removes the user from the user list and deletes them. */
void Server::removeUser(User * user) {
	cout << "User disconnected." << endl;
	userList[user->getUserId()] = nullptr;
	delete user;
}

/* Returns the user list array. */
User ** Server::getUserList() {
	return userList;
}

/* Finds the user in the user list by the specified name.
	- Returns nullptr if no one was found by that name.
	- Returns the user pointer if the name was found. */
User * Server::getUserByName(string name) {
	for (unsigned short i = 0; i < MAX_USERS; i++) {
		if (userList[i] == nullptr)
			continue;
		if (userList[i]->getUsername() == name)
			return userList[i];
	}
	return nullptr;
}

/* Returns the array of registered names. */
std::vector<std::string> Server::getRegisteredNameList() {
	return registeredNameList;
}

/* Sees if the username exists within the registered name array. */
bool Server::doesRegisteredUsernameExist(std::string username) {
	for (string s : registeredNameList) {
		if (s == username) {
			return true;
		}
	}
	return false;
}

/* Adds a name to the registered username list. */
void Server::addRegisteredUsername(std::string username) {
	registeredNameList.push_back(username);
}

Server::~Server() {
	if (sSocket != INVALID_SOCKET) {
		closesocket(sSocket);
	}
	WSACleanup();
	for (unsigned short i = 0; i < MAX_USERS; i++) {
		delete userList[i];
	}
	for (unsigned short i = 0; i < MAX_ROOMS; i++) {
		delete roomList[i];
	}
}