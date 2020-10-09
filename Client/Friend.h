#ifndef FRIEND_H_
#define FRIEND_H_
#include <string>
class Friend {
public:
	Friend(std::string name, bool online);
	std::string getName();
	std::string getLowercaseName();
	bool isOnline();
	void setOnline(bool online);
private:
	std::string name;
	std::string lowercaseName;
	bool online;
};

#endif //FRIEND_H_