#ifndef USER_H_
#define USER_H_
#include <thread>
#include "Packet/PacketHandler.h"
#include <winsock2.h>
#include "Friend.h"
class Server;
class Room;

class User {
public:
	User(Server* server, unsigned short userId, SOCKET socket);
	~User();
	Room* getRoom() const;
	void setRoom(Room* room);
	std::string getUsername() const;
	void setUsername(std::string username);
	std::string getUsernameLowercase() const;
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
	unsigned short load(std::string username);
	void save();
	void disconnect();
	bool isFriend(std::string name);
	bool addFriend(std::string name);
	bool removeFriend(std::string name);
	Friend* getFriend(std::string name);
	void updateFriendStatus(Friend* friendEntry);
	void sendFriendsList();
	void setReplyUsername(std::string name);
	std::string getReplyUsername();
	std::string getIp();
	Friend** getFriends();
	PacketHandler* getPacketHandler();
	void sendMessage(User* const from, std::string message, bool personalMessage);
	void sendServerMessage(std::string message, unsigned short messageColor = ERROR_COLOR);
private:
	Server* server;
	Room* room;
	unsigned short userId;
	unsigned short userNameColor;
	unsigned short userChatColor;
	PacketHandler* packetHandler;
	std::thread readThreadInstance;
	std::thread writeThreadInstance;
	std::string username;
	std::string usernameLowercase;
	std::string password;
	std::string replyUsername;
	Friend* friendsList[MAX_FRIENDS];
	std::string ip;
	bool verified;
	bool authenticated;
};
#endif //USER_H_
