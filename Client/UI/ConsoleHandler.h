#ifndef CONSOLE_RENDERER_H_
#define CONSOLE_RENDERER_H_
#define INPUT_BUFFER_SIZE 128
#define MAX_BODY_MESSAGES 100
#define MAX_WIDGET_MESSAGES 10
#define WIDGET_SIZE 20
#include <thread>
#include <windows.h>
#include <iostream>
#include <condition_variable>
#include "../Constants.h"
#include "../Friend.h"
class MessageEntry {
public:
	MessageEntry(std::string message, unsigned short defaultColor, bool newLine) :
		message(message),
		defaultColor(defaultColor),
		newLine(newLine) {};
	std::string getMessage() { return message; }
	unsigned short getDefaultColor() { return defaultColor; }
	bool doesNewLine() { return newLine; }
private:
	std::string message;
	unsigned short defaultColor;
	bool newLine;
};

class WidgetEntry {
public:
	WidgetEntry(std::string message, unsigned short color) :
		message(message),
		color(color) {};
	std::string getMessage() { return message; };
	unsigned short getColor() { return color; };
private:
	std::string message;
	unsigned short color;
};

class ConsoleHandler
{
public:
	ConsoleHandler();
	~ConsoleHandler();
	std::string getBlockingInput(std::string promptMessage = "");
	void pushBodyMessage(std::string message, unsigned short defaultColor = DEFAULT_COLOR, bool newLine = true, bool push = true);
	void updateTopRight(std::string * names, unsigned short * colors, unsigned short count, bool push = true);
	void updateBottomRight(Friend ** friends, unsigned short count, bool push = true);
	void updateBottomRight(std::string* names, unsigned short* colors, unsigned short count, bool push = true);
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

	MessageEntry * messageHistory[MAX_BODY_MESSAGES];
	unsigned short curBodyMessageIdx;
	COORD topWidgetStart;
	COORD bottomWidgetStart;
	WidgetEntry * topWidgetHistory[MAX_WIDGET_MESSAGES];
	WidgetEntry * bottomWidgetHistory[MAX_WIDGET_MESSAGES];

	void readInput();
	void handleResize();
	void checkResize();
	void setCursor(unsigned short x, unsigned short y);
	void setColor(unsigned short color);
	void scrollScreenUp();
};

#endif //CONSOLE_RENDERER_H_
