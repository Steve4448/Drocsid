#ifndef USER_H_
#define USER_H_
#include <thread>
#include "PacketHandler.h"
#include <winsock2.h>
class Server;
class Room;

class User
{
public:
	User(Server * server, unsigned short userId, SOCKET socket);
	~User();
	Room * getRoom() const;
	void setRoom(Room * room);
	std::string getUsername() const;
	void setUsername(std::string username);
	std::string getPassword() const;
	void setPassword(std::string password);
	bool isVerified() const;
	void setVerified(bool verified);
	void setAuthenticated(bool authenticated);
	bool isAuthenticated() const;
	unsigned short getUserId() const;
	void setUserNameColor(unsigned short userNameColor);
	void setUserChatColor(unsigned short userChatColor);
	unsigned short getUserNameColor() const;
	unsigned short getUserChatColor() const;
	unsigned short getFriendsListCount() const;
	void disconnect();
	void sendMessage(User * const from, std::string message, bool personalMessage);
	bool isFriend(std::string name);
	bool addFriend(std::string name);
	bool removeFriend(std::string name);
	void updateFriendsList();
	std::string * getFriends();
	PacketHandler * getPacketHandler();
	void sendServerMessage(std::string message, unsigned short messageColor = ERROR_COLOR);
private:
	Server * server;
	Room * room;
	unsigned short userId;
	unsigned short friendsListCount;
	unsigned short userNameColor;
	unsigned short userChatColor;
	PacketHandler * packetHandler;
	std::thread readThreadInstance;
	std::thread writeThreadInstance;
	std::string username;
	std::string password;
	std::string friendsList[MAX_FRIENDS];
	bool verified;
	bool authenticated;
};
#endif //USER_H_
