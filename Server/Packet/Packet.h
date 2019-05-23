#ifndef PACKET_H_
#define PACKET_H_
#include "../Constants.h"
#include "DataStream.h"
#include <string>

class Packet
{
	friend class PacketHandler;
public:
	~Packet();
	unsigned short getId() const;

private:
	Packet(DataStream * const stream, unsigned short packetId);
	DataStream * stream;
	unsigned short currentIndex;
	unsigned short const packetId;

	friend Packet & operator<<(Packet & dataStream, const std::string & toWrite);
	friend Packet & operator<<(Packet & dataStream, const int & toWrite);
	friend Packet & operator<<(Packet & dataStream, const unsigned short & toWrite);

	friend Packet & operator>>(Packet & dataStream, std::string & toRead);
	friend Packet & operator>>(Packet & dataStream, int & toRead);
	friend Packet & operator>>(Packet & dataStream, unsigned short & toRead);
};
#endif //PACKET_H_
