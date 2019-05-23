#include "Room.h"
#include "User.h"
#include "Server.h"


Room::Room(Server * server, User * owner, std::string roomName) :
	server(server),
	owner(owner),
	roomName(roomName),
	userList{ nullptr },
	userCount(0) {
}

/* Relays a message to every user within the room. */
void Room::sendMessage(User * user, std::string message) {
	for (unsigned short i = 0; i < MAX_ROOM_USERS; i++) {
		if (userList[i] == nullptr)
			continue;
		userList[i]->sendMessage(user, message, false);
	}
}

/* Adds the user to the room and updates everyones room user list. */
void Room::joinRoom(User * user) {
	if (userCount >= MAX_ROOM_USERS)
		return;
	unsigned short slot = -1;
	for (unsigned short i = 0; i < MAX_ROOM_USERS; i++) {
		if (userList[i] == nullptr) {
			slot = i;
			break;
		}
	}
	Packet * p = user->getPacketHandler()->constructPacket(ATTEMPT_JOIN_ROOM_PACKET_ID);
	*p << (unsigned short)(slot == -1 ? ATTEMPT_JOIN_ROOM_FAILURE : ATTEMPT_JOIN_ROOM_SUCCESS);
	user->getPacketHandler()->finializePacket(p);

	if (slot == -1) {
		return;
	} else {
		userList[slot] = user;
		userCount++;
		user->setRoom(this);
	}
	for (unsigned short i = 0; i < MAX_ROOM_USERS; i++) {
		if (userList[i] == nullptr)
			continue;
		userList[i]->sendMessage(user, "has joined the room.", false);
	}
	updateRoomList();
	server->updateRoomList();
}

/* Removes the user from the room and updates everyones room user list.
	Also once the owner leaves the room will be destroyed.
*/
void Room::leaveRoom(User * user) {
	Packet * p = user->getPacketHandler()->constructPacket(LEAVE_ROOM_PACKET_ID);
	user->getPacketHandler()->finializePacket(p);
	user->setRoom(nullptr);

	for (unsigned short i = 0; i < MAX_ROOM_USERS; i++) {
		if (userList[i] == nullptr)
			continue;
		if (userList[i] == user) {
			userList[i] = nullptr;
			userCount--;
		} else {
			userList[i]->sendMessage(user, "has left the room.", false);
		}
	}
	if (user == owner || userCount == 0) {
		server->destroyRoom(this);
	} else {
		updateRoomList();
		server->updateRoomList();
	}
}

/* Kicks everyone from the room. */
void Room::ensureEmpty() {
	for (unsigned short i = 0; i < MAX_ROOM_USERS; i++) {
		if (userList[i] == nullptr)
			continue;
		Packet * p = userList[i]->getPacketHandler()->constructPacket(LEAVE_ROOM_PACKET_ID);
		userList[i]->getPacketHandler()->finializePacket(p);
		userList[i]->setRoom(nullptr);
		userList[i] = nullptr;
	}
}

/* Sends a room update to everyone inside the room. */
void Room::updateRoomList() {
	for (unsigned short i = 0; i < MAX_ROOM_USERS; i++) {
		if (userList[i] == nullptr)
			continue;
		updateRoomList(userList[i]);
	}
}

/* Sends the list of who is inside the room to a user. */
void Room::updateRoomList(User * user) {
	Packet * p = user->getPacketHandler()->constructPacket(UPDATE_ROOM_LIST_PACKET_ID);
	*p << userCount;
	for (unsigned short i2 = 0; i2 < MAX_ROOM_USERS; i2++) {
		if (userList[i2] == nullptr)
			continue;
		*p << (user->isFriend(userList[i2]->getUsername()) ? (unsigned short)FRIEND_COLOR : userList[i2]->getUserNameColor());
		*p << userList[i2]->getUsername();
	}
	user->getPacketHandler()->finializePacket(p);
}

/* Returns the list of users inside the room. */
User ** Room::getUserList() {
	return userList;
}

/* Returns the user that is the owner of this room. */
User * Room::getOwner() {
	return owner;
}

/* Returns the name of this room. */
std::string Room::getName() {
	return roomName;
}

/* Returns the number of users within this room. */
unsigned short Room::getUserCount() {
	return userCount;
}

Room::~Room() {

}