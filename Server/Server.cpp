#include "Server.h"
#include "User.h"
#include "Room.h"
#include "Constants.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "Exception/StartupException.h"
#include <fstream>

/* Library required for winsock usage. */
#pragma comment (lib, "Ws2_32.lib")
using namespace std;

/* Entry point for the server, takes in the port to run at and constructs and begins running the server. */
int main() {
	if(!CreateDirectoryA(string(SAVE_DIRECTORY).c_str(), NULL)) {
		DWORD lastError = GetLastError();
		if(lastError != ERROR_ALREADY_EXISTS) {
			cout << "Error #" << lastError << " making the save directory: " << SAVE_DIRECTORY << endl;
			return 1;
		}
	}

	unsigned short portNum = 0;
	string port = "";
	cout << "Welcome to Drocsid." << endl << endl;
	cout << "Please enter the port that the server will be hosted at: ";
	while(portNum == 0) {
		try {
			cin >> port;
			portNum = stoi(port);
			cin.ignore();
		} catch(invalid_argument&) {
			cout << "Please enter a valid port number: ";
		}
	}
	Server server(portNum);
	try {
		if(server.start())
			server.doListen();
	} catch(StartupException& e) {
		cerr << "Error starting the server: " << e.what() << endl;
	}
	return 0;
}

Server::Server(unsigned int port) :
	port(port),
	userList{nullptr},
	roomList{nullptr},
	sSocket(INVALID_SOCKET),
	listening(true),
	roomCount(0),
	allowedUsernameChars("^[a-zA-Z0-9]*$") {
	logFile.open(LOG_DIRECTORY + (string)"log.txt", ofstream::app);
	if(!logFile.is_open()) {
		throw exception("Could not open log file.");
	}
}

/* Starts the server
winsock code implemented using information from https://docs.microsoft.com/en-us/windows/desktop/winsock/winsock-server-application
*/
bool Server::start() {
	cout << "Server starting." << endl;
	WSADATA wsaData;
	struct addrinfo socketSpecifications;
	struct addrinfo* socketResult = nullptr;
	int wsaResult;

	wsaResult = WSAStartup(DESIRED_WINSOCK_VERSION, &wsaData);
	if(wsaResult != 0) {
		throw StartupException("WSAStartup failed: ", wsaResult);
	}

	ZeroMemory(&socketSpecifications, sizeof(socketSpecifications));
	socketSpecifications.ai_family = AF_INET;
	socketSpecifications.ai_socktype = SOCK_STREAM;
	socketSpecifications.ai_protocol = IPPROTO_TCP;
	socketSpecifications.ai_flags = AI_PASSIVE;

	wsaResult = getaddrinfo(NULL, to_string(port).c_str(), &socketSpecifications, &socketResult);
	if(wsaResult != 0) {
		throw StartupException("getaddrinfo failed with error: ", wsaResult);
	}

	sSocket = socket(socketResult->ai_family, socketResult->ai_socktype, socketResult->ai_protocol);
	if(sSocket == INVALID_SOCKET) {
		throw StartupException("socket failed with error: ", WSAGetLastError());
	}

	wsaResult = ::bind(sSocket, socketResult->ai_addr, (int)socketResult->ai_addrlen);
	freeaddrinfo(socketResult);
	if(wsaResult == SOCKET_ERROR) {
		throw StartupException("bind failed with error: ", WSAGetLastError());
	}

	wsaResult = listen(sSocket, SOMAXCONN);
	if(wsaResult == SOCKET_ERROR) {
		throw StartupException("bind failed with error: ", WSAGetLastError());
	}
	cout << "Server now listening." << endl;
	return true;
}

/* Listens for connections and upon a connection constructs a user. */
void Server::doListen() {
mainLoop: while(listening) {
	SOCKET userSocket = accept(sSocket, NULL, NULL);
	if(userSocket == INVALID_SOCKET) {
		cerr << "accept failed with error: " << WSAGetLastError();
		continue;
	}

	for(unsigned short i = 0; i < MAX_USERS; i++) {
		if(userList[i] == nullptr) {
			userList[i] = new User(this, i, userSocket);
			goto mainLoop;
		}
	}
	cerr << "User attempted a connection but the server is full." << endl;
	closesocket(userSocket);
}
}

/* Returns the servers room list array. */
Room** const Server::getRoomList() {
	return roomList;
}

/* Attempts to make a room
- Returns nullptr if there is no room spaces left.
- Returns address of room if a room was successfully made. */
Room* const Server::makeRoom(User* const owner, string roomName) {
	for(unsigned short i = 0; i < MAX_ROOMS; i++) {
		if(roomList[i] == nullptr) {
			roomCount++;
			log(owner->getUsername() + " created room " + roomName + ".");
			return (roomList[i] = new Room(this, owner, roomName));
		}
	}
	return nullptr;
}

/* Destroys the room, first ensures everyone has left it. */
void Server::destroyRoom(Room* const room) {
	for(unsigned short i = 0; i < MAX_ROOMS; i++) {
		if(roomList[i] == room) {
			log("Room " + room->getName() + " destroyed.");
			roomList[i]->ensureEmpty();
			delete roomList[i];
			roomList[i] = nullptr;
			roomCount--;
			break;
		}
	}
	updateRoomList();
}

/* Sends a room list update to everyone connected. */
void Server::updateRoomList() {
	for(unsigned short i = 0; i < MAX_USERS; i++) {
		if(userList[i] == nullptr)
			continue;
		updateRoomList(userList[i]);
	}
}

/* Sends a friend status update to anyone that is friends with the specified user. */
void Server::handleFriendStatusUpdate(User* const user) {
	User** userList = getUserList();
	for(unsigned short i = 0; i < MAX_USERS; i++) {
		if(userList[i] == nullptr)
			continue;
		Friend* friendEntry = userList[i]->getFriend(user->getUsername());
		if(friendEntry != nullptr) {
			userList[i]->updateFriendStatus(user, friendEntry);
		}
	}
}

/* Sends a room list update to a specific user. */
void Server::updateRoomList(User* user) {
	Packet* p = user->getPacketHandler()->constructPacket(ROOM_STATUS_UPDATE_PACKET_ID);
	*p << roomCount;
	for(unsigned short i2 = 0; i2 < MAX_ROOMS; i2++) {
		if(roomList[i2] == nullptr)
			continue;
		*p << roomList[i2]->getName();
	}
	user->getPacketHandler()->finializePacket(p);
}

/* Removes the user from the user list and deletes them. */
void Server::removeUser(User* user) {
	log((user->getUsername().empty() ? user->getIp() : user->getUsername()) + " disconnected.");
	userList[user->getUserId()] = nullptr;
	delete user;
}

/* Returns the user list array. */
User** const Server::getUserList() {
	return userList;
}

/* Finds the user in the user list by the specified name.
- Returns nullptr if no one was found by that name.
- Returns the user pointer if the name was found. */
User* const Server::getUserByName(string name) {
	if(name.empty())
		return nullptr;
	transform(name.begin(), name.end(), name.begin(), ::tolower);
	for(unsigned short i = 0; i < MAX_USERS; i++) {
		if(userList[i] == nullptr)
			continue;
		if(userList[i]->isAuthenticated() && userList[i]->getUsernameLowercase() == name)
			return userList[i];
	}
	return nullptr;
}

/* Checks to see if the username contains invalid characters. Return true if it is acceptable, otherwise false. */
bool Server::isValidUsername(string username) {
	static smatch matches;
	return regex_search(username, matches, allowedUsernameChars);
}

/* Sees if the user is registered. */
bool Server::doesRegisteredUsernameExist(string username) {
	if(!isValidUsername(username))
		return false;
	transform(username.begin(), username.end(), username.begin(), ::tolower);
	ifstream saveFile;
	saveFile.open(SAVE_DIRECTORY + username + ".txt");
	bool exists = saveFile.is_open();
	saveFile.close();
	return exists;
}

/* Checks to see if the username is registered by attempting to load the user save file, and if so returns the proper case of that username. Otherwise returns an empty string. */
string Server::getProperUsernameCase(string username) {
	transform(username.begin(), username.end(), username.begin(), ::tolower);
	ifstream userFile;
	try {
		userFile.open(SAVE_DIRECTORY + username + ".txt");
		if(userFile.is_open()) {
			string properName;
			unsigned short version = 0;
			userFile >> version;
			switch(version) {
				case 2:
					userFile >> properName;
					break;
				default:
					throw exception("Unsupported file version");
			}
			userFile.close();
			return properName;
		} else {
			return "";
		}
	} catch(exception e) {
		cout << "Failure loading " << username << "'s proper username from save: " << e.what();
	}
	return "";
}

/* Appends the line to the log file and also outputs it to the console. */
void Server::log(string line) {
	cout << line << endl;
	logFile << line << endl;
}

Server::~Server() {
	if(sSocket != INVALID_SOCKET) {
		closesocket(sSocket);
	}
	WSACleanup();
	for(unsigned short i = 0; i < MAX_USERS; i++) {
		delete userList[i];
	}
	for(unsigned short i = 0; i < MAX_ROOMS; i++) {
		delete roomList[i];
	}
}