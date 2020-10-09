#include "Friend.h"
#include "User.h"
#include <algorithm>
using namespace std;

Friend::Friend(User* const activeUser, string name) :
	activeUser(activeUser),
	name(name),
	lowercaseName(name) {
	transform(lowercaseName.begin(), lowercaseName.end(), lowercaseName.begin(), ::tolower);
}

User* const Friend::getActiveUser() {
	return activeUser;
}

void Friend::setActiveUser(User* const activeUser) {
	this->activeUser = activeUser;
}

string Friend::getName() {
	return name;
}

string Friend::getLowercaseName() {
	return lowercaseName;
}

bool Friend::isOnline() {
	return activeUser != nullptr && activeUser->getPacketHandler()->isConnected();
}