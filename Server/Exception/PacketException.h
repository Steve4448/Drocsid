#ifndef PACKETEXCEPTION_H_
#define PACKETEXCEPTION_H_
#include <stdexcept>
#include <string>
class PacketException : public std::exception {
public:
	PacketException(const std::string& errorMessage) : std::exception(errorMessage.c_str()) {}
	~PacketException() {}
};

class PacketAuthException : public PacketException {
public:
	PacketAuthException(const std::string& errorMessage) : PacketException(errorMessage) {}
	~PacketAuthException() {}
};
#endif