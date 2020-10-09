#ifndef PACKET_HANDLER_H_
#define PACKET_HANDLER_H_
#include "../Constants.h"
#include "Packet.h"
#include <winsock2.h>
#include <mutex>
class ChatClient;

class PacketHandler {
public:
	PacketHandler(ChatClient* const clientInstance, SOCKET socket);
	~PacketHandler();
	void readLoop();
	void setConnected(bool connected);
	Packet* constructPacket(unsigned short);
	void finializePacket(Packet* packet, bool _flush = false);
	void flush(bool self = false);
private:
	SOCKET socket;
	ChatClient* const user;
	bool connected;
	DataStream* stream;
	DataStream* peeker;
	std::mutex mtx;
	Packet* constructingPacket;
};
#endif //PACKET_HANDLER_H_