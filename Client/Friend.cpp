#include "Friend.h"
#include <algorithm>
using namespace std;

Friend::Friend(string name, bool online) : 
	name(name),
	lowercaseName(name),
	online(online) {
	transform(lowercaseName.begin(), lowercaseName.end(), lowercaseName.begin(), ::tolower);
}

string Friend::getName() {
	return name;
}

string Friend::getLowercaseName() {
	return lowercaseName;
}

bool Friend::isOnline() {
	return online;
}

void Friend::setOnline(bool online) {
	this->online = online;
}