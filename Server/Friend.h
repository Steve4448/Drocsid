#ifndef FRIEND_H_
#define FRIEND_H_
#include <string>
class User;
class Friend {
public:
	Friend(User* activeUser, std::string name);
	User* getActiveUser();
	void setActiveUser(User* activeUser);
	std::string getName();
	std::string getLowercaseName();
	bool isOnline();
private:
	User* activeUser;
	std::string name;
	std::string lowercaseName;
};

#endif //FRIEND_H_