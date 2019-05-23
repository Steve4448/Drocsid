#ifndef STARTUPEXCEPTION_H_
#define STARTUPEXCEPTION_H_
#include <stdexcept>
#include <string>
class StartupException : public std::exception {
public:
	StartupException(const std::string& errorMessage) : std::exception(errorMessage.c_str()) {}
	StartupException(const std::string& errorMessage, int code) : std::exception((errorMessage + std::to_string(code)).c_str()) {}
	~StartupException() {}
};
#endif