#ifndef CONSOLE_RENDERER_H_
#define CONSOLE_RENDERER_H_
#define INPUT_BUFFER_SIZE 128
#define MAX_BODY_MESSAGES 100
#include <thread>
#include <windows.h>
#include <iostream>
#include <condition_variable>
#include "../Constants.h"
class ConsoleHandler
{
public:
	ConsoleHandler();
	~ConsoleHandler();
	std::string getBlockingInput(std::string promptMessage = "");
	void pushBodyMessage(std::string message, unsigned short defaultColor = DEFAULT_COLOR, bool newLine = true);
private:
	bool running;
	INPUT_RECORD inputBuffer[INPUT_BUFFER_SIZE];
	HANDLE inputStdHandle;
	HANDLE outputStdHandle;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	unsigned short rows;
	unsigned short columns;
	COORD curCursor;

	bool inputStringFinished;
	std::string currentInputString;
	std::condition_variable cv;
	std::mutex inputMutex;
	std::mutex outputMutex;
	std::thread inputThreadInstance;

	std::thread resizeThreadInstance;
	uint64_t lastResizeCheck;
	bool resizeNeeded;

	void readInput();
	void handleResize();
	void checkResize();
	void setCursor(unsigned short x, unsigned short y);
	void setColor(unsigned short color);
};

#endif //CONSOLE_RENDERER_H_
