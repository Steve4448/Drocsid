//Thread code implemented using information from http://www.cplusplus.com/reference/thread/thread/
#include "User.h"
#include "Room.h"
#include "Server.h"
#include "Constants.h"
#include <iostream>
#include <winsock2.h>

using namespace std;

User::User(Server * server, unsigned short userId, SOCKET socket) :
	server(server),
	room(nullptr),
	userId(userId),
	username(""),
	password(""),
	verified(false),
	authenticated(false),
	friendsList{""},
	friendsListCount(0),
	userNameColor(DEFAULT_COLOR),
	userChatColor(DEFAULT_CHAT_COLOR),
	packetHandler(new PacketHandler(server, this, socket)) {
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

/* Set the user's name. */
void User::setUsername(string username) {
	this->username = username;
}

/* Returns the user's password. */
string User::getPassword() const {
	return password;
}

/* Set the user's password */
void User::setPassword(string password) {
	this->password = password;
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

}

/* Set the user's chat color. */
void User::setUserChatColor(unsigned short userChatColor) {
	this->userChatColor = userChatColor;
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
PacketHandler * User::getPacketHandler() {
	return packetHandler;
}

/* Return the room the user is inside (nullptr if not in one)*/
Room * User::getRoom() const {
	return room;
}

/* Set the room the user is inside. */
void User::setRoom(Room * room) {
	this->room = room;
}

/* Return the number of friends. */
unsigned short User::getFriendsListCount() const {
	return friendsListCount;
}

/* Returns true if the name is on the user's friends list, otherwise false. */
bool User::isFriend(std::string name) {
	for (int i = 0; i < MAX_FRIENDS; i++) {
		if (friendsList[i].empty())
			continue;
		if (name == friendsList[i])
			return true;
	}
	return false;
}

/* Attempts to add a friend
	- Returns true if the friend was successfully added. (Also updates the room list incase their friend is inside)
	- Returns false if the friends list was full.
*/
bool User::addFriend(std::string name) {
	for (int i = 0; i < MAX_FRIENDS; i++) {
		if (friendsList[i].empty()) {
			friendsList[i] = name;
			friendsListCount++;
			updateFriendsList();
			if(getRoom() != nullptr)
				getRoom()->updateRoomList(this);
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
	for (int i = 0; i < MAX_FRIENDS; i++) {
		if (!friendsList[i].empty() && friendsList[i] == name) {
			friendsList[i] = "";
			friendsListCount--;
			updateFriendsList();
			if (getRoom() != nullptr)
				getRoom()->updateRoomList(this);
			return true;
		}
	}
	return false;
}

/* Sends the friends list to the user. */
void User::updateFriendsList() {
	Packet * p = packetHandler->constructPacket(UPDATE_FRIENDS_LIST_PACKET_ID);
	*p << friendsListCount;
	for (int i = 0; i < MAX_FRIENDS; i++) {
		if (friendsList[i].empty())
			continue;
		User * friendUser = server->getUserByName(friendsList[i]);
		*p << (unsigned short)(friendUser != nullptr && friendUser->getPacketHandler()->isConnected() ? 1 : 0);
		*p << friendsList[i];
	}
	packetHandler->finializePacket(p);
}

/* Return the user's friends list. */
string * User::getFriends() {
	return friendsList;
}

/* Disconnect the user from the server.
	We need to properly join or detach the threads depending on what thread called the disconnection.
	If an authenticated user disconnects it will send a friends list update to anyone that is the user's friend.
*/
void User::disconnect() {
	packetHandler->setConnected(false);
	if (readThreadInstance.joinable() && this_thread::get_id() != readThreadInstance.get_id())
		readThreadInstance.join();
	else
		readThreadInstance.detach();
	if (writeThreadInstance.joinable() && this_thread::get_id() != writeThreadInstance.get_id())
		writeThreadInstance.join();
	else
		writeThreadInstance.detach();
	if (isAuthenticated()) {
		if (getRoom() != nullptr)
			getRoom()->leaveRoom(this);
		User ** userList = server->getUserList();
		for (unsigned short i = 0; i < MAX_USERS; i++) {
			if (userList[i] == nullptr)
				continue;
			if (userList[i]->isFriend(getUsername()))
				userList[i]->updateFriendsList();
		}
	}
	server->removeUser(this);
}

/* Send a server message to the user. */
void User::sendServerMessage(string message, unsigned short messageColor) {
	Packet * p = packetHandler->constructPacket(SERVER_MESSAGE_PACKET_ID);
	*p << "<" + to_string(messageColor) + ">" + message;
	packetHandler->finializePacket(p);

}

/* Send a room/private message from another user to the user. */
void User::sendMessage(User * const from, string message, bool personalMessage) {
	Packet * p = packetHandler->constructPacket(MESSAGE_PACKET_ID);
	/**p << (isFriend(from->getUsername()) ? (unsigned short)FRIEND_COLOR : from->getUserNameColor());
	*p << from->getUserChatColor();
	*p << (unsigned short)(personalMessage ? 1 : 0);
	*p << from->getUsername();
	*p << message;*/

	*p << "<" + to_string((isFriend(from->getUsername()) ? (unsigned short)FRIEND_COLOR : from->getUserNameColor())) + ">" + from->getUsername() +
		"<" + to_string(DEFAULT_COLOR) + ">" + ": " +
		"<" + to_string(from->getUserChatColor()) + ">" + message;
	packetHandler->finializePacket(p);
}

User::~User() {
	delete packetHandler;
}
