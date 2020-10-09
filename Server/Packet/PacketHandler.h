#ifndef PACKET_HANDLER_H_
#define PACKET_HANDLER_H_
#include "../Constants.h"
#include "Packet.h"
#include <winsock2.h>
#include <thread>
#include <mutex>
#include <condition_variable>
class Server;
class User;

class PacketHandler {
public:
	PacketHandler(Server* const server, User* const user, SOCKET socket);
	~PacketHandler();
	void readLoop();
	void writeLoop();
	void setConnected(bool connected);
	bool isConnected() const;
	Packet* const constructPacket(unsigned short);
	void finializePacket(Packet* const packet, bool _flush = false);
	void flush(bool self = false);
private:
	Server* const server;
	SOCKET socket;
	User* const user;
	bool connected;
	DataStream* const stream;
	DataStream* const peeker;
	std::mutex mtx;
	Packet* constructingPacket;
};
#endif //PACKET_HANDLER_H_