#ifndef PACKETEXCEPTION_H_
#define PACKETEXCEPTION_H_
#include <stdexcept>
#include <string>
class PacketException : public std::exception {
public:
	PacketException(const std::string& errorMessage) : std::exception(errorMessage.c_str()) {}
	~PacketException() {}
};
#endif