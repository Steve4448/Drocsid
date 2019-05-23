#include "User.h"
#include "Packet.h"
using namespace std;

PacketAuthenticate::PacketAuthenticate(DataStream * const stream) : Packet(stream, AUTHENTICATION_PACKET_ID) {
}

void PacketAuthenticate::read() {
	*stream >> username;
	*stream >> password;
}

void PacketAuthenticate::write() const {
	*stream << result;
}

void PacketAuthenticate::setData(ResultCode result) {
	this->result = (unsigned short)result;
}

string PacketAuthenticate::getUsername() {
	return username;
}

string PacketAuthenticate::getPassword() {
	return password;
}