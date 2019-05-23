#include "Packet.h"
#include <iostream>
using namespace std;

PacketHandshake::PacketHandshake(DataStream * const stream) : Packet(stream, HANDSHAKE_PACKET_ID) {}

void PacketHandshake::read() {
	*stream >> versionCode;
}

void PacketHandshake::write() const {
	*stream << string(VERSION_CODE);
}

string PacketHandshake::getVersionCode() {
	return versionCode;
}