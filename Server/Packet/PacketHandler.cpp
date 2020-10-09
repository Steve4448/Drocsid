#include "PacketHandler.h"
#include <iostream>
#include "../Server.h"
#include "../User.h"
#include "../Room.h"
#include "../Exception/PacketException.h"
#include <chrono>

using namespace std;

PacketHandler::PacketHandler(Server* const server, User* const user, SOCKET socket) :
	server(server),
	user(user),
	socket(socket),
	connected(true),
	constructingPacket(nullptr),
	peeker(new DataStream(4)),
	stream(new DataStream(BUFFER_LENGTH)) {}

/* Continuously reads data from the socket.
	It first will peek to see how many bytes are available to read.
	As soon as at least 2 bytes are available it means that there is enough information to know how much will actually be coming from the client.
	Those two bytes indicate how many bytes will actually be sent as the packet payload.
		The main thought process behind this method is that it is designed to always have 2 bytes to represent how much data will actually need to be read first.
		Otherwise, it would be impossible to know if we have actually gotten enough data to represent anything logical.

		Once we know how long the payload will be, we can make sure we fully read all of it and stop at that point.

	Next, it will keep reading until it has fully read the length that was just received prior.
	Once it has fully read the packet payload, it will begin to process them.

	For processing, it will first read the packet id and then will handle the packet based on the id.

	If any error occurs, or the client loses connection, it will stop reading and disconnect the user from the server.
*/
void PacketHandler::readLoop() {
	int in = SOCKET_ERROR;
	int packetId = 0;
	unsigned short totalSize = 0;
	unsigned short totalRead = 0;
	try {
		while(connected) {
			//Peek to see if we have 2 or more bytes available to read to get how much data will be in the next chunk.
			if((in = recv(socket, peeker->getInputBuffer(), 2, MSG_PEEK)) >= 2) {
				recv(socket, peeker->getInputBuffer(), 2, 0); //Consume those 2 bytes.
				*peeker >> totalSize;
				if(totalSize <= 0)
					throw PacketException("Invalid payload size " + to_string(totalSize));
				else if(totalSize >= stream->getSize())
					throw PacketException("Too much data received!");
				while(connected && totalRead < totalSize) {
					if((in = recv(socket, stream->getInputBuffer() + totalRead, totalSize - totalRead, 0)) != SOCKET_ERROR) { //Fully read
						totalRead += in;
					} else {
						break;
					}
				}
				if(in == SOCKET_ERROR)
					break;
				if(totalRead == totalSize) {
					while(connected && totalRead > stream->getReadIndex().getPosition()) { //Process packets until all packets in the chunk of data are consumed.
						*stream >> packetId;
						switch(packetId) {
							case HANDSHAKE_PACKET_ID:
							{
								string versionCode = "";
								*stream >> versionCode;
								if(versionCode != VERSION_CODE) {
									throw PacketException("Client had invalid version code: " + versionCode);
								}
								Packet* p = constructPacket(HANDSHAKE_PACKET_ID);
								*p << VERSION_CODE;
								finializePacket(p);
								user->setVerified(true);
								break;
							}
							case AUTHENTICATION_PACKET_ID:
							{
								if(!user->isVerified()) {
									throw PacketAuthException("Unverified user trying to authenticate.");
								}
								string username = "";
								string password = "";
								*stream >> username;
								*stream >> password;
								unsigned short returnCode = AUTHENTICATION_FAILURE;
								if(username.empty() || !server->isValidUsername(username)) {
									returnCode = AUTHENTICATION_INVALID_USERNAME;
									server->log(user->getIp() + " tried to use invalid username: " + username);
								} else {
									if(server->getUserByName(username) != nullptr) {
										returnCode = AUTHENTICATION_NAME_IN_USE;
										server->log(user->getIp() + " tried to use username already in use: " + username);
									} else {
										switch(user->load(username)) {
											case LOAD_SUCCESS:
												returnCode = user->getPassword() == password ? AUTHENTICATION_SUCCESS : AUTHENTICATION_INVALID_PASSWORD;
												server->log(user->getPassword() == password ? user->getUsername() + " has logged in from " + user->getIp() + "." : user->getIp() + " has used an invalid password for user: " + user->getUsername() + ".");
												break;
											case LOAD_NEW_USER:
												returnCode = AUTHENTICATION_SUCCESS;
												user->setUsername(username);
												user->setPassword(password);
												server->log(user->getIp() + " has created a new username: " + user->getUsername());
												break;
											case LOAD_FAILURE:
											default:
												returnCode = AUTHENTICATION_FAILURE;
												break;
										}
									}
								}
								Packet* p = constructPacket(AUTHENTICATION_PACKET_ID);
								*p << returnCode;
								finializePacket(p);
								if(returnCode == AUTHENTICATION_SUCCESS) {
									user->setAuthenticated(true);
									user->sendServerMessage("Welcome to <11>Drocsid!", DEFAULT_COLOR);
									user->sendServerMessage("Type /joinroom [name] to join/create a room.", DEFAULT_COLOR);
									user->sendServerMessage("Type /help for more commands.", DEFAULT_COLOR);

									server->handleFriendStatusUpdate(user);
									user->sendFriendsList();
									server->updateRoomList(user);
								}
								break;
							}
							case MESSAGE_PACKET_ID:
							{
								if(!user->isAuthenticated()) {
									throw PacketAuthException("Unauthenticated user trying to send a message.");
								}
								string message = "";
								*stream >> message;
								if(message.length() > 0) {
									bool validCommand = true;
									if(message.at(0) == '/') {
										size_t spacePos = message.find(' ');
										string command = (spacePos == string::npos ? message.substr(1, message.length()) : message.substr(1, spacePos - 1));
										string arguments = (spacePos == string::npos ? "" : message.substr(spacePos + 1, message.length()));
										server->log((user->getRoom() != nullptr ? "<" + user->getRoom()->getName() + "> " : "") + user->getUsername() + " used command: " + command + " with arguments: " + arguments);
										if(command == "joinroom") {
											if(spacePos == string::npos) {
												user->sendServerMessage("Invalid command arguments.");
												user->sendServerMessage("Try as /joinroom [room name]");
												break;
											}
											if(arguments.length() == 0 || arguments.length() > 10) {
												user->sendServerMessage("Please specify a proper room name.");
												break;
											}
											if(user->getRoom() != nullptr)
												user->getRoom()->leaveRoom(user);
											bool foundRoom = false;
											Room** roomList = server->getRoomList();
											for(unsigned short i = 0; i < MAX_ROOMS; i++) {
												if(roomList[i] != nullptr) {
													if(roomList[i]->getName() == arguments) {
														roomList[i]->joinRoom(user);
														foundRoom = true;
														break;
													}
												}
											}
											if(!foundRoom) {
												Room* newRoom = server->makeRoom(user, arguments);
												if(newRoom == nullptr) {
													Packet* p = user->getPacketHandler()->constructPacket(ATTEMPT_JOIN_ROOM_PACKET_ID);
													*p << (unsigned short)ATTEMPT_JOIN_ROOM_FAILURE;
													user->getPacketHandler()->finializePacket(p);
												} else {
													newRoom->joinRoom(user);
												}
											}
										} else if(command == "leaveroom" || command == "leave") {
											if(user->getRoom() != nullptr) {
												user->getRoom()->leaveRoom(user);
											} else {
												user->sendServerMessage("You're not in a room.");
											}
										} else if(command == "addfriend") {
											if(spacePos == string::npos) {
												user->sendServerMessage("Invalid command arguments.");
												user->sendServerMessage("Try as /addfriend [username]");
												break;
											}
											if(!server->isValidUsername(arguments)) {
												user->sendServerMessage("Invalid username specified.");
												break;
											}
											string properUsername = server->getProperUsernameCase(arguments);
											if(properUsername.empty()) {
												user->sendServerMessage("No one exists with the name " + arguments + ".");
												break;
											}
											if(properUsername == user->getUsername()) {
												user->sendServerMessage("You cannot add yourself.");
												break;
											}
											if(user->isFriend(properUsername)) {
												user->sendServerMessage("You've already added " + properUsername + ".");
												break;
											}
											if(!user->addFriend(properUsername)) {
												user->sendServerMessage("You cannot add more than " + to_string(MAX_FRIENDS) + " friends.");
												break;
											}
										} else if(command == "removefriend") {
											if(spacePos == string::npos) {
												user->sendServerMessage("Invalid command arguments.");
												user->sendServerMessage("Try as /removefriend [username]");
												break;
											}
											if(!user->removeFriend(arguments)) {
												user->sendServerMessage("You don't have a friend with the name " + arguments + ".");
											}
										} else if(command == "friendslist") {
											Friend** friends = user->getFriends();
											for(unsigned short i = 0; i < MAX_FRIENDS; i++) {
												if(friends[i] == nullptr)
													continue;
												user->sendServerMessage(friends[i]->getName(), friends[i]->isOnline() ? FRIEND_COLOR : FRIEND_OFFLINE_COLOR);
											}
										} else if(command == "pm") {
											size_t nextSpacePos = arguments.find(' ');
											if(spacePos == string::npos || nextSpacePos == string::npos) {
												user->sendServerMessage("Invalid command arguments.");
												user->sendServerMessage("Try as /pm [username] [message]");
												break;
											}
											string pmName = message.substr(spacePos + 1, nextSpacePos);
											string actualMessage = arguments.substr(nextSpacePos + 1, arguments.length());
											User* pmUser = server->getUserByName(pmName);
											if(pmUser == nullptr) {
												user->sendServerMessage("Could not find " + pmName + ".");
												break;
											} else if(pmUser == user) {
												user->sendServerMessage("Surely you're not that lonely.");
												break;
											}
											user->sendMessage(pmUser, actualMessage, false, true, true);
											pmUser->sendMessage(user, actualMessage, false, true, false);
											pmUser->setReplyUsername(user->getUsername());
											user->setReplyUsername(pmUser->getUsername());
										} else if(command == "r" || command == "reply") {
											if(spacePos == string::npos) {
												user->sendServerMessage("Invalid command arguments.");
												user->sendServerMessage("Try as /reply [message]");
												break;
											}
											if(user->getReplyUsername().empty()) {
												user->sendServerMessage("You have nobody to reply to.");
												break;
											}
											User* pmUser = server->getUserByName(user->getReplyUsername());
											if(pmUser == nullptr) {
												user->sendServerMessage(user->getReplyUsername() + " is no longer online.");
												break;
											}
											user->sendMessage(pmUser, arguments, false, true, true);
											pmUser->sendMessage(user, arguments, false, true, false);
											pmUser->setReplyUsername(user->getUsername());
										} else if(command == "settextcolor") {
											if(spacePos == string::npos) {
												user->sendServerMessage("Invalid command arguments.");
												user->sendServerMessage("Try as /settextcolor [color number]");
												user->sendServerMessage("Type /colors for a list of available colors.");
												break;
											}
											try {
												unsigned short color = stoi(arguments);
												//TODO: Check/list colors from what are valid.
												user->setUserChatColor(color);
											} catch(invalid_argument&) {
												user->sendServerMessage("Please type a proper integer for your desired color.");
											}
										} else if(command == "setnamecolor") {
											if(spacePos == string::npos) {
												user->sendServerMessage("Invalid command arguments.");
												user->sendServerMessage("Try as /setnamecolor [color number]");
												user->sendServerMessage("Type /colors for a list of available colors.");
												break;
											}
											try {
												unsigned short color = stoi(arguments);
												//TODO: Check/list colors from what are valid.
												user->setUserNameColor(color);
											} catch(invalid_argument&) {
												user->sendServerMessage("Please type a proper integer for your desired color.");
											}
										} else if(command == "colors") {
											string output = "";
											for(byte i = 0; i < 255; i++) {
												string num = to_string(i);
												output += "<" + num + ">" + num + " ";
											}
											user->sendServerMessage(output, DEFAULT_COLOR);
										} else if(command == "help" || command == "h" || command == "?" || command == "commands") {
											user->sendServerMessage("/joinroom [room name]", DEFAULT_COLOR);
											user->sendServerMessage("/leaveroom", DEFAULT_COLOR);
											user->sendServerMessage("/addfriend [username]", DEFAULT_COLOR);
											user->sendServerMessage("/removefriend [username]", DEFAULT_COLOR);
											user->sendServerMessage("/friendslist", DEFAULT_COLOR);
											user->sendServerMessage("/pm [username] [message]", DEFAULT_COLOR);
											user->sendServerMessage("/reply [message]", DEFAULT_COLOR);
											user->sendServerMessage("/settextcolor [color number]", DEFAULT_COLOR);
											user->sendServerMessage("/setnamecolor [color number]", DEFAULT_COLOR);
											user->sendServerMessage("/colors", DEFAULT_COLOR);
										} else {
											validCommand = false;
										}
									} else if(user->getRoom() != nullptr) {
										server->log("<" + user->getRoom()->getName() + "> " + user->getUsername() + ": " + message);
										user->getRoom()->sendMessage(user, message);
									} else {
										validCommand = false;
									}
									if(!validCommand) {
										user->sendServerMessage("Invalid command.");
										user->sendServerMessage("Type /help to see a list of proper commands.");
									}
								}
								break;
							}
							default:
								throw PacketException("Invalid packet id " + to_string(packetId));
						}
					}
					totalRead = 0;
					totalSize = 0;
					stream->resetRead();
				} else {
					throw PacketException("Read more than total size.");
				}
				peeker->resetRead();
			}
			if(in == SOCKET_ERROR)
				break;
		}
	} catch(PacketAuthException& e) {
		cerr << e.what() << endl;
	} catch(PacketException& e) {
		cerr << "Read loop error: " << e.what() << endl;
	} catch(exception& e) {
		cerr << "Read loop big error: " << e.what() << endl;
	}
	user->disconnect();
}

/*
	Contionuslly try to write information to the client.

	A mutex is used here to ensure that we don't write while we're still in the process of constructing a packet. (Used in constructPacket()/finalizePacket())
	If any data as accumulated in the buffer it will send that data.
	(If a mutex wasn't used here we would have the possibility of sending a unfinished packet, which would break everything.)

	A 50 millisecond sleep occurs each loop so that packets can be pooled together instead of only sending 1 at a time.
	Additionally, if it were less than 50 milliseconds it would use more CPU.
*/
void PacketHandler::writeLoop() {
	while(connected) {
		mtx.lock();
		if(connected && stream->getWriteIndex().getPosition() > 0)
			flush(true);
		mtx.unlock();
		this_thread::sleep_for(chrono::milliseconds(50)); // sleep snippet from https://stackoverflow.com/questions/4184468/sleep-for-milliseconds
	}
}

/* Makes a new packet and returns it for modification.
	This also locks a mutex to prevent sending data in flush() until it's finished.
*/
Packet* const PacketHandler::constructPacket(unsigned short id) {
	mtx.lock();
	constructingPacket = new Packet(stream, id);
	return constructingPacket;
}

/* Finializes the packet by unlocking the mutex to let flush send the fully constructed packet. */
void PacketHandler::finializePacket(Packet* const packet, bool _flush) {
	if(packet != constructingPacket)
		throw runtime_error("Finialized packet wasn't the original.");
	constructingPacket = nullptr;
	delete packet;
	if(_flush)
		flush(true);
	mtx.unlock();
}

/* Sends the accumulated packet payload to the client.
	First it sends how many bytes will actually be inside the payload inside 2 bytes. It will keep looping to ensure these 2 bytes were successfully sent.
	Next it will send the entire payload and keep looping until all of the payload was sent.
	See the readLoop() description for the reasoning behind this.
*/
void PacketHandler::flush(bool self) {
	if(!connected)
		return;
	if(!self)
		mtx.lock();
	unsigned short totalSent = 0;
	unsigned short desiredSize = stream->getWriteIndex().getPosition();
	*peeker << desiredSize;
	unsigned short totalSize = peeker->getWriteIndex().getPosition();
	while(totalSent < totalSize) {
		int sent = send(socket, peeker->getOutputBuffer() + totalSent, totalSize - totalSent, 0);
		if(sent == SOCKET_ERROR) { //Possibly lost connection.
			user->disconnect();
			if(!self)
				mtx.unlock();
			return;
		}
		totalSent += sent;
	}
	if(totalSent > totalSize) {
		throw exception("Peeker sent more than total size.");
	}
	totalSent = 0;
	totalSize = desiredSize;
	while(totalSent < totalSize) {
		int sent = send(socket, stream->getOutputBuffer() + totalSent, totalSize - totalSent, 0);
		if(sent == SOCKET_ERROR) { //Possibly lost connection.
			user->disconnect();
			if(!self)
				mtx.unlock();
			return;
		}
		totalSent += sent;
	}
	if(totalSent > totalSize) {
		throw exception("Stream sent more than total size.");
	}
	peeker->resetWrite();
	stream->resetWrite();
	if(!self)
		mtx.unlock();
}

/* Sets weither or not the client is connected, if not it will close the socket. */
void PacketHandler::setConnected(bool connected) {
	this->connected = connected;
	if(!connected && socket != INVALID_SOCKET) {
		closesocket(socket);
		socket = INVALID_SOCKET;
	}
}

/* Returns if the client is connected. */
bool PacketHandler::isConnected() const {
	return connected;
}

PacketHandler::~PacketHandler() {
	if(socket != INVALID_SOCKET) {
		closesocket(socket);
		socket = INVALID_SOCKET;
	}
	delete peeker;
	delete stream;
}