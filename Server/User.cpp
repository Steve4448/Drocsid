#include "User.h"
#include "Room.h"
#include "Server.h"
#include "Constants.h"
#include <iostream>
#include <winsock2.h>
#include <fstream>

using namespace std;

User::User(Server* server, unsigned short userId, SOCKET socket) :
	server(server),
	room(nullptr),
	userId(userId),
	username(""),
	usernameLowercase(""),
	password(""),
	verified(false),
	authenticated(false),
	friendsList{nullptr},
	userNameColor(DEFAULT_COLOR),
	userChatColor(DEFAULT_CHAT_COLOR),
	packetHandler(new PacketHandler(server, this, socket)),
	replyUsername("") {
	sockaddr_in socketAddr;
	int addrLen = sizeof(socketAddr);
	if(getsockname(socket, (LPSOCKADDR)&socketAddr, &addrLen) == SOCKET_ERROR) {
		cerr << "getsockname failed with: " << WSAGetLastError();
		ip = "error";
	} else {
		ip.resize(addrLen);
		inet_ntop(AF_INET, &socketAddr.sin_addr, const_cast<char*>(ip.c_str()), addrLen);
		size_t firstSpace = ip.find('\0');
		if(firstSpace != string::npos) {
			ip = ip.substr(0, firstSpace);
		}
	}
	server->log(ip + " connected.");
	readThreadInstance = thread(&PacketHandler::readLoop, packetHandler);
	writeThreadInstance = thread(&PacketHandler::writeLoop, packetHandler);
}

/* Returns the user's id. */
unsigned short User::getUserId() const {
	return userId;
}

/* Returns the user's name. */
string User::getUsername() const {
	return username;
}

/* Returns the user's name in lowercase */
string User::getUsernameLowercase() const {
	return usernameLowercase;
}

/* Set the user's name. */
void User::setUsername(string username) {
	this->username = username;
	this->usernameLowercase.resize(username.size());
	transform(username.begin(), username.end(), usernameLowercase.begin(), ::tolower);
}

/* Returns the user's password. */
string User::getPassword() const {
	return password;
}

/* Set the user's password */
void User::setPassword(string password) {
	this->password = password;
	save();
}

/* Returns if the user is verified. */
bool User::isVerified() const {
	return verified;
}

/* Set if the user is verified. */
void User::setVerified(bool verified) {
	this->verified = verified;
}

/* Set if the user is authenticated. */
void User::setAuthenticated(bool authenticated) {
	this->authenticated = authenticated;
}

/* Returns if the user is authenticated. */
bool User::isAuthenticated() const {
	return authenticated;
}

/* Set the user's name color. */
void User::setUserNameColor(unsigned short userNameColor) {
	this->userNameColor = userNameColor;
	save();

}

/* Set the user's chat color. */
void User::setUserChatColor(unsigned short userChatColor) {
	this->userChatColor = userChatColor;
	save();
}

/* Return the user's name color. */
unsigned short User::getUserNameColor() const {
	return userNameColor;
}

/* Return the user's chat color. */
unsigned short User::getUserChatColor() const {
	return userChatColor;
}

/* Return the packet handler instance. */
PacketHandler* User::getPacketHandler() {
	return packetHandler;
}

/* Return the room the user is inside (nullptr if not in one)*/
Room* User::getRoom() const {
	return room;
}

/* Set the room the user is inside. */
void User::setRoom(Room* const room) {
	this->room = room;
}

/* Returns true if the name is on the user's friends list, otherwise false. */
bool User::isFriend(std::string name) {
	transform(name.begin(), name.end(), name.begin(), ::tolower);
	for(int i = 0; i < MAX_FRIENDS; i++) {
		if(friendsList[i] == nullptr)
			continue;
		if(name == friendsList[i]->getLowercaseName())
			return true;
	}
	return false;
}

/* Returns the user's friend is found, otherwise nullptr. */
Friend* const User::getFriend(std::string name) {
	transform(name.begin(), name.end(), name.begin(), ::tolower);
	for(int i = 0; i < MAX_FRIENDS; i++) {
		if(friendsList[i] == nullptr)
			continue;
		if(name == friendsList[i]->getLowercaseName())
			return friendsList[i];
	}
	return nullptr;
}

/* Attempts to add a friend
	- Returns true if the friend was successfully added. (Also updates the room list incase their friend is inside)
	- Returns false if the friends list was full.
*/
bool User::addFriend(std::string name) {
	for(int i = 0; i < MAX_FRIENDS; i++) {
		if(friendsList[i] == nullptr) {
			friendsList[i] = new Friend(server->getUserByName(name), name);
			Packet* p = packetHandler->constructPacket(ADD_FRIEND_PACKET_ID);
			*p << friendsList[i]->getName();
			*p << friendsList[i]->isOnline();
			packetHandler->finializePacket(p);
			if(getRoom() != nullptr)
				getRoom()->updateRoomList(this);
			save();
			return true;
		}
	}
	return false;
}

/* Attempts to remove a friend.
	- Returns false if the friend wasn't on the user's friends list.
	- Returns true if the friend was removed from their list.  (Also updates the room list incase their friend is inside)
*/
bool User::removeFriend(std::string name) {
	transform(name.begin(), name.end(), name.begin(), ::tolower);
	for(int i = 0; i < MAX_FRIENDS; i++) {
		if(friendsList[i] != nullptr && friendsList[i]->getLowercaseName() == name) {
			delete friendsList[i];
			friendsList[i] = nullptr;
			Packet* p = packetHandler->constructPacket(REMOVE_FRIEND_PACKET_ID);
			*p << name;
			packetHandler->finializePacket(p);
			if(getRoom() != nullptr)
				getRoom()->updateRoomList(this);
			save();
			return true;
		}
	}
	return false;
}

void User::updateFriendStatus(User* const friendUser, Friend* const friendEntry) {
	friendEntry->setActiveUser(friendUser->getPacketHandler()->isConnected() ? friendUser : nullptr);

	Packet* p = packetHandler->constructPacket(FRIEND_STATUS_PACKET_ID);
	*p << friendEntry->getName();
	*p << friendEntry->isOnline();
	packetHandler->finializePacket(p);
	sendMessage(friendUser, "has just " + (string)(friendEntry->isOnline() ? "logged on." : "logged off."), true, false, false);
}

void User::sendFriendsList() {
	Packet* p = packetHandler->constructPacket(FRIENDS_LIST_PACKET_ID);
	*p << (unsigned short)MAX_FRIENDS;
	for(int i = 0; i < MAX_FRIENDS; i++) {
		if(friendsList[i] != nullptr) {
			*p << friendsList[i]->getName();
			*p << friendsList[i]->isOnline();
		} else {
			*p << "";
			*p << false;
		}
	}
	packetHandler->finializePacket(p);
}

/* Return the user's friends list. */
Friend** const User::getFriends() {
	return friendsList;
}

/* Disconnect the user from the server.
	We need to properly join or detach the threads depending on what thread called the disconnection.
	If an authenticated user disconnects it will send a friends list update to anyone that is the user's friend.
*/
void User::disconnect() {
	packetHandler->setConnected(false);
	if(readThreadInstance.joinable() && this_thread::get_id() != readThreadInstance.get_id())
		readThreadInstance.join();
	else
		readThreadInstance.detach();
	if(writeThreadInstance.joinable() && this_thread::get_id() != writeThreadInstance.get_id())
		writeThreadInstance.join();
	else
		writeThreadInstance.detach();
	if(isAuthenticated()) {
		save();
		if(getRoom() != nullptr)
			getRoom()->leaveRoom(this);
		server->handleFriendStatusUpdate(this);
	}
	server->removeUser(this);
}

/* Send a server message to the user. */
void User::sendServerMessage(string message, unsigned short messageColor) {
	Packet* p = packetHandler->constructPacket(SERVER_MESSAGE_PACKET_ID);
	*p << "<" + to_string(messageColor) + ">" + message;
	packetHandler->finializePacket(p);
}

/* Send a room/private message from another user to the user. */
void User::sendMessage(User* const from, string message, bool statusMessage, bool personalMessage, bool isSender) {
	Packet* p = packetHandler->constructPacket(MESSAGE_PACKET_ID);
	/**p << "<" + to_string((isFriend(from->getUsername()) ? (unsigned short)FRIEND_COLOR : from->getUserNameColor())) + ">" + from->getUsername() +
		"<" + to_string(DEFAULT_COLOR) + ">" + ": " +
		"<" + to_string(from->getUserChatColor()) + ">" + message;*/
	*p << from->getUsername();
	*p << from->getUserNameColor();
	*p << from->getUserChatColor();
	*p << statusMessage;
	*p << personalMessage;
	*p << isSender;
	*p << message;
	packetHandler->finializePacket(p);
}

/* Attempts to load the users data.
	Returns LOAD_SUCCESS (0) if the user was loaded successfully.
	Returns LOAD_FAILURE (1) if an error occured while loading.
	Returns LOAD_NEW_USER (2) if the user file hasn't yet been created (new user, creates a new file for them.)
*/
unsigned short User::load(string username) {
	unsigned short code = LOAD_FAILURE;
	if(username.empty() || !isVerified())
		return LOAD_FAILURE;
	transform(username.begin(), username.end(), username.begin(), ::tolower);
	ifstream userFile;
	try {
		userFile.open(SAVE_DIRECTORY + username + ".txt");
		if(userFile.is_open()) {
			unsigned short version = 0;
			userFile >> version;
			switch(version) {
				case 2:
					userFile >> this->username;
					usernameLowercase.resize(this->username.size());
					transform(this->username.begin(), this->username.end(), usernameLowercase.begin(), ::tolower);
					userFile >> userNameColor;
					userFile >> userChatColor;
					userFile >> password;
					for(unsigned short i = 0; i < MAX_FRIENDS; i++) {
						string friendsName;
						userFile >> friendsName;
						if(!friendsName.empty())
							friendsList[i] = new Friend(server->getUserByName(friendsName), friendsName);
					}
					break;
				default:
					throw exception("Unsupported file version");
			}
			userFile.close();
			code = LOAD_SUCCESS;
		} else {
			code = LOAD_NEW_USER;
		}
	} catch(exception e) {
		code = LOAD_FAILURE;
		cout << "Failure loading " << username << "'s user file: " << e.what();
	}
	return code;
}

/* Saves the user data. */
void User::save() {
	if(usernameLowercase.empty() || !isVerified())
		return;
	ofstream userFile;
	try {
		userFile.open(SAVE_DIRECTORY + usernameLowercase + ".txt");
		if(!userFile.is_open()) {
			throw exception("Could not open file to save.");
		}
		userFile << (unsigned short)2 << endl; //File version
		userFile << username << endl;
		userFile << userNameColor << endl;
		userFile << userChatColor << endl;
		userFile << password << endl;
		for(unsigned short i = 0; i < MAX_FRIENDS; i++) {
			userFile << (friendsList[i] == nullptr ? "" : friendsList[i]->getName()) << endl;
		}
		userFile.close();
	} catch(exception e) {
		cout << e.what();
	}
}

/* Returns the user's IP address. */
string User::getIp() {
	return ip;
}

string User::getReplyUsername() {
	return replyUsername;
}

void User::setReplyUsername(string name) {
	replyUsername = name;
}

User::~User() {
	for(unsigned short i = 0; i < MAX_FRIENDS; i++) {
		delete friendsList[i];
	}
	delete packetHandler;
}
