# Drocsid
Drocsid is just a fun little C++ learning project to create a chat server and client while sticking to the windows console, avoiding using external libraries, and using Winsock. Drocsid currently supports multiple users, rooms, friends, simultaneous message sending & receiving, and colored messasges/names.
![Image of Drocsid](application%20preview.png)

## How to use the Server
The server will only prompt for a port to run at, nothing else should be required.

## How to use the Client
The client will first prompt for a IP and then port that points to the running Drocsid server. Afterwards, it will prompt for a username/password to login with. After that, you should be in the lobby in which you can do the commands found in the commands list below. In order to start talking to other connected users you will need to join room under the same name and then non-commands will be sent as messages to each room member.

## Feature/Challenge List
- [x] Winsock
- [x] Simultaneous client connections (threading & mutexes).
- [x] Keep track of registered usernames & don't allow the same username to exist.
- [x] Packet system with streams that support << and >> operators.
- [x] Clients can communicate with eachother.
- [x] Commands support.
- [x] Room support.
- [x] Friends list support.
- [x] User can input text while also receiving messages (mutexes & custom console input method).
- [x] Support for colors.
- [X] Store user information.
- [X] Check user's stored password upon connection to verify credentials.
- [X] A UI with 'widgets' where the user's friends list & the server's room list can be displayed.
- [X] Case insensitive usernames.
- [X] Chat/server logs.
- [ ] Ensure mutexes for packets are actually working properly. (I haven't had any issues yet, but I'm not confident in that code.)

## Command List
- /joinroom \[room name\]
- /leaveroom
- /addfriend \[username\]
- /removefriend \[username\]
- /friendslist
- /pm \[username\] \[message\]
- /settextcolor \[color number\]
- /setnamecolor \[color number\]
- /colors
