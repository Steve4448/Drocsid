#include "PacketHandler.h"
#include <iostream>
#include <algorithm>
#include "../ChatClient.h"
#include "../Exception/PacketException.h"
using namespace std;

PacketHandler::PacketHandler(ChatClient* const clientInstance, SOCKET socket) :
	user(clientInstance),
	socket(socket),
	connected(true),
	constructingPacket(nullptr),
	peeker(new DataStream(4)),
	stream(new DataStream(BUFFER_LENGTH)) {}

/* Continuously reads data from the socket.
	It first will peek to see how many bytes are available to read.
	As soon as at least 2 bytes are available it means that there is enough information to know how much will actually be coming from the server.
	Those two bytes indicate how many bytes will actually be sent as the packet payload.
		The main thought process behind this method is that it is designed to always have 2 bytes to represent how much data will actually need to be read first.
		Otherwise, it would be impossible to know if we have actually gotten enough data to represent anything logical.

		Once we know how long the payload will be, we can make sure we fully read all of it and stop at that point.

	Next, it will keep reading until it has fully read the length that was just received prior.
	Once it has fully read the packet payload, it will begin to process them.

	For processing, it will first read the packet id and then will handle the packet based on the id.

	If any error occurs, or the client loses connection, it will stop reading and disconnect from the server.
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
									throw PacketException("Server had invalid version code: " + versionCode);
								}
								//cout << endl << "Received handshake version: " << versionCode << endl; //TODO: REMOVE
								user->doCredentials();
								break;
							}
							case AUTHENTICATION_PACKET_ID:
							{
								unsigned short returnCode = AUTHENTICATION_FAILURE;
								*stream >> returnCode;
								switch(returnCode) {
									case AUTHENTICATION_INVALID_PASSWORD:
										user->getConsoleRenderer()->pushBodyMessage("You've entered an invalid password.", ERROR_COLOR);
										user->doCredentials(true);
										break;
									case AUTHENTICATION_NAME_IN_USE:
										user->getConsoleRenderer()->pushBodyMessage("That username is already in use.", ERROR_COLOR);
										user->doCredentials();
										break;
									case AUTHENTICATION_INVALID_USERNAME:
										user->getConsoleRenderer()->pushBodyMessage("Please enter a valid username using letters (a-z) and numbers only (0-9).", ERROR_COLOR);
										user->doCredentials();
										break;
									case AUTHENTICATION_SUCCESS:
										user->gotoLobby();
										break;
									default:
									case AUTHENTICATION_FAILURE:
										user->getConsoleRenderer()->pushBodyMessage("The server could not process your authentication.", ERROR_COLOR);
										user->disconnect();
										return;
								}
								break;
							}
							case MESSAGE_PACKET_ID:
							{
								string messageFrom;
								unsigned short usernameColor;
								unsigned short userChatColor;
								bool statusMessage;
								bool personalMessage;
								bool isSender;
								string message;
								*stream >> messageFrom;
								*stream >> usernameColor;
								*stream >> userChatColor;
								*stream >> statusMessage;
								*stream >> personalMessage;
								*stream >> isSender;
								*stream >> message;
								user->getConsoleRenderer()->pushBodyMessage(
									(personalMessage ? "<" + to_string(FRIEND_COLOR) + ">" + (isSender ? "[TO]" : "[FROM]") + " " : "") +
									"<" + to_string((user->isFriend(messageFrom) ? FRIEND_COLOR : usernameColor)) + ">" +
									messageFrom +
									"<" + to_string(DEFAULT_COLOR) + ">" +
									(statusMessage ? " " : ": ") +
									"<" + to_string(userChatColor) + ">" + message);
								break;
							}
							case ATTEMPT_JOIN_ROOM_PACKET_ID:
							{
								unsigned short joinRoomStatusCode = 0;
								*stream >> joinRoomStatusCode;
								if(joinRoomStatusCode == ATTEMPT_JOIN_ROOM_SUCCESS) {
									user->setInRoom(true);
								} else {
									user->getConsoleRenderer()->pushBodyMessage("Could not join room.", ERROR_COLOR);
								}
								break;
							}
							case LEAVE_ROOM_PACKET_ID:
							{
								user->setInRoom(false);
								break;
							}
							case ROOM_STATUS_UPDATE_PACKET_ID:
							{
								unsigned short roomCount = 0;
								*stream >> roomCount;
								string* roomNames = new string[roomCount];
								unsigned short* roomColors = new unsigned short[roomCount];
								//cout << "Room list update (" << roomCount << "):";
								for(unsigned short i = 0; i < roomCount; i++) {
									string roomName = "";
									*stream >> roomName;
									//cout << " " << roomName;
									roomNames[i] = roomName;
									roomColors[i] = DEFAULT_COLOR;
								}
								if(!user->isInRoom())
									user->getConsoleRenderer()->updateTopRight(roomNames, roomColors, roomCount);
								delete[] roomNames;
								delete[] roomColors;
								//cout << endl;
								break;
							}
							case UPDATE_ROOM_LIST_PACKET_ID:
							{

								unsigned short userCount = 0;
								*stream >> userCount;
								string* userRoomList = new string[userCount];
								unsigned short* userRoomColors = new unsigned short[userCount];
								//cout << "User list update (" << userCount << "):";
								for(unsigned short i = 0; i < userCount; i++) {
									unsigned short usernameColor = 0;
									string userName = "";
									*stream >> usernameColor;
									*stream >> userName;
									//cout << " " << userName;
									userRoomList[i] = userName;
									userRoomColors[i] = usernameColor;
								}
								if(user->isInRoom())
									user->getConsoleRenderer()->updateTopRight(userRoomList, userRoomColors, userCount);
								delete[] userRoomList;
								delete[] userRoomColors;
								//cout << endl;
								break;
							}
							case SERVER_MESSAGE_PACKET_ID:
							{
								string message = "";
								*stream >> message;
								user->getConsoleRenderer()->pushBodyMessage(message);
								break;
							}
							case ADD_FRIEND_PACKET_ID:
							{
								string name = "";
								bool isOnline = false;
								*stream >> name;
								*stream >> isOnline;
								for(unsigned short i = 0; i < user->getFriendsListSize(); i++) {
									if(user->getFriendsList()[i] == nullptr) {
										user->getFriendsList()[i] = new Friend(name, isOnline);
										user->getConsoleRenderer()->updateBottomRight(user->getFriendsList(), user->getFriendsListSize());
										break;
									}
								}
								break;
							}
							case REMOVE_FRIEND_PACKET_ID:
							{
								string name = "";
								*stream >> name;
								transform(name.begin(), name.end(), name.begin(), ::tolower);
								for(unsigned short i = 0; i < user->getFriendsListSize(); i++) {
									if(user->getFriendsList()[i] != nullptr && user->getFriendsList()[i]->getLowercaseName() == name) {
										delete user->getFriendsList()[i];
										user->getFriendsList()[i] = nullptr;
										user->getConsoleRenderer()->updateBottomRight(user->getFriendsList(), user->getFriendsListSize());
										break;
									}
								}
								break;
							}
							case FRIEND_STATUS_PACKET_ID:
							{
								string name = "";
								bool isOnline = false;
								*stream >> name;
								*stream >> isOnline;
								transform(name.begin(), name.end(), name.begin(), ::tolower);
								for(unsigned short i = 0; i < user->getFriendsListSize(); i++) {
									if(user->getFriendsList()[i] != nullptr && user->getFriendsList()[i]->getLowercaseName() == name) {
										user->getFriendsList()[i]->setOnline(isOnline);
										user->getConsoleRenderer()->updateBottomRight(user->getFriendsList(), user->getFriendsListSize());
										break;
									}
								}
								break;
							}
							case FRIENDS_LIST_PACKET_ID:
							{
								for(unsigned short i = 0; i < user->getFriendsListSize(); i++) {
									delete user->getFriendsList()[i];
								}
								delete[] user->getFriendsList();

								unsigned short friendCount = 0;
								*stream >> friendCount;
								Friend** friendsList = new Friend * [friendCount];
								for(unsigned short i = 0; i < friendCount; i++) {
									string name = "";
									bool isOnline = false;
									*stream >> name;
									*stream >> isOnline;
									friendsList[i] = (name.empty() ? nullptr : new Friend(name, isOnline));
								}
								user->getConsoleRenderer()->updateBottomRight(friendsList, friendCount);
								user->setFriendsList(friendCount, friendsList);
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
			if(in == SOCKET_ERROR) { //Possibly lost connection.
				break;
			}
		}
	} catch(PacketException& e) {
		//cerr << "Read loop error: " << e.what() << endl;
		user->getConsoleRenderer()->pushBodyMessage("Read loop error: ", ERROR_COLOR);
		user->getConsoleRenderer()->pushBodyMessage(e.what(), ERROR_COLOR);
	} catch(exception& e) {
		//cerr << "Read loop big error: " << e.what() << endl;
		user->getConsoleRenderer()->pushBodyMessage("Read loop big error: ", ERROR_COLOR);
		user->getConsoleRenderer()->pushBodyMessage(e.what(), ERROR_COLOR);
	}
	user->disconnect();
}

/* Makes a new packet and returns it for modification.
	This also locks a mutex to prevent sending data in flush() until it's finished.
*/
Packet* PacketHandler::constructPacket(unsigned short id) {
	mtx.lock();
	constructingPacket = new Packet(stream, id);
	return constructingPacket;
}

/* Finializes the packet by unlocking the mutex to let flush send the fully constructed packet. */
void PacketHandler::finializePacket(Packet* packet, bool _flush) {
	if(packet != constructingPacket)
		throw runtime_error("Finialized packet wasn't the original.");
	constructingPacket = nullptr;
	delete packet;
	if(_flush)
		flush(true);
	mtx.unlock();
}

/* Sends the accumulated packet payload to the server.
	First it sends how many bytes will actually be inside the payload inside 2 bytes. It will keep looping to ensure these 2 bytes were successfully sent.
	Next it will send the entire payload and keep looping until all of the payload was sent.
	See the readLoop() description for the reasoning behind this.
*/
void PacketHandler::flush(bool self) {
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

/* Sets weither or not the client is connected */
void PacketHandler::setConnected(bool connected) {
	this->connected = connected;
}

PacketHandler::~PacketHandler() {
	delete peeker;
	delete stream;
	if(socket != INVALID_SOCKET) {
		closesocket(socket);
		socket = INVALID_SOCKET;
	}
}