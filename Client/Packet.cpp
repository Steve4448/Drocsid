#include "Packet.h"
using namespace std;

Packet::Packet(DataStream * stream, unsigned short packetId) : stream(stream), packetId(packetId), currentIndex(0) {
	*this << (int)getId();
}

Packet::~Packet() {
}

unsigned short Packet::getId() const{
	return packetId;
}

/* Writes an integer to the output stream. */
Packet & operator<<(Packet & packet, const int & toWrite) {
	*packet.stream << toWrite;
	return packet;
}

/* Reads an integer from the input stream. */
Packet & operator>>(Packet & packet, int & toRead) {
	*packet.stream >> toRead;
	return packet;
}

/* Writes an unsigned short to the output stream. */
Packet & operator<<(Packet & packet, const unsigned short & toWrite) {
	*packet.stream << toWrite;
	return packet;
}

/* Reads an unsigned short from the input stream. */
Packet & operator>>(Packet & packet, unsigned short & toRead) {
	*packet.stream >> toRead;
	return packet;
}

/* Writes string to the output stream. */
Packet & operator<<(Packet & packet, const string & toWrite) {
	*packet.stream << toWrite;
	return packet;
}

/* Reads a string from the input stream. */
Packet & operator>>(Packet & packet, string & toRead) {
	*packet.stream >> toRead;
	return packet;
}
