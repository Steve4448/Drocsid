#include "ChatClient.h"
#include "Packet.h"
using namespace std;

PacketAuthenticate::PacketAuthenticate(DataStream * const stream) : Packet(stream, AUTHENTICATION_PACKET_ID) {
}

void PacketAuthenticate::read() {
	*stream >> result;
}

void PacketAuthenticate::write() const {
	*stream << username;
	*stream << password;
}

void PacketAuthenticate::setData(string username, string password) {
	this->username = username;
	this->password = password;
}

PacketAuthenticate::ResultCode PacketAuthenticate::getResultCode() {
	switch (result) {
	case PacketAuthenticate::INVALID_PASSWORD:
		return PacketAuthenticate::INVALID_PASSWORD;
	case PacketAuthenticate::SUCCESS:
		return PacketAuthenticate::SUCCESS;
	case PacketAuthenticate::FAILURE:
	default:
		return PacketAuthenticate::FAILURE;
	}
}