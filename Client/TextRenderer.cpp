#include "TextRenderer.h"
#include <iostream>
using namespace std;

unsigned short TextRenderer::curBodyY = 0;
unsigned short TextRenderer::rows = 0;
unsigned short TextRenderer::columns = 0;
unsigned short TextRenderer::bodyMessageSize = 0;
string TextRenderer::bodyMessages[MAX_BODY_MESSAGE_COUNT] = { "" };
unsigned short TextRenderer::bodyColors[MAX_BODY_MESSAGE_COUNT] = { DEFAULT_COLOR };
mutex TextRenderer::lckMutex;

void TextRenderer::renderFrame(bool displayColumns) {
	getConsoleSize(rows, columns);

	string lengthX = string(columns, '\xDB') + "\n";
	string lengthY = "\xDB" + string(columns - (displayColumns ? COLUMN_WIDTH + 1 : 2), ' ') + "\xDB" + (displayColumns ? (string(COLUMN_WIDTH - 2, ' ') + "\xDB") : "");
	string lengthSepY = "\xDB" + string(columns - (displayColumns ? COLUMN_WIDTH + 1 : 2), ' ') + "\xDB" + (displayColumns ? (string(COLUMN_WIDTH - 1, '\xDB') + "") : "");
	setCursor(0, 0);
	cout << lengthX;
	for (int i = 0; i < (displayColumns ? FRAME_HEIGHT - 3 : FRAME_HEIGHT - 2); i++)
	{
		if (i == (displayColumns ? FRAME_HEIGHT / 2 - 3 : FRAME_HEIGHT / 2 - 2))
			cout << lengthSepY;
		else
			cout << lengthY;
		setCursor(0, i+1);
	}
	setCursor(0, (displayColumns ? FRAME_HEIGHT - 3 : FRAME_HEIGHT - 2));
	cout << lengthX;

	/*COORD coord;
	coord.X = 0;
	coord.Y = FRAME_HEIGHT - 1;
	SetConsoleCursorPosition(stdOut, coord);*/
	if (displayColumns) {
		for (int i = 2; i > 1; i--) {
			setCursor(0, FRAME_HEIGHT - i);
			cout << lengthY;
		}
		setCursor(0, FRAME_HEIGHT - 1);
		cout << string(columns, '\xDB');

		/*string lengthX2 = string(columns, '\xDB');
		string lengthY2 = "\xDB" + string(columns - 2, ' ') + "\xDB";

		cout << lengthX2;
		for (int i = 0; i < 4 - 2; i++)
		{
			cout << lengthY2;
		}
		cout << lengthX2;*/
	}
	bodyMessageSize = (displayColumns ? columns - (BODY_OFFSET_X * 2) - COLUMN_WIDTH : columns - (BODY_OFFSET_X * 2));
}

void TextRenderer::setCursor(unsigned short x, unsigned short y, bool self) {
	if (!self)
		lckMutex.lock();
	static HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	static COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(stdOut, coord);
	if (!self)
		lckMutex.unlock();
}

void TextRenderer::pushBody(string message, unsigned short color, bool increment, bool resize, bool self) {
	if (!self)
		lckMutex.lock();
	if(resize)
		message.resize(bodyMessageSize);
	if (curBodyY < MAX_BODY_MESSAGE_COUNT) {
		setCursor(BODY_OFFSET_X, increment ? curBodyY + 1 : curBodyY, true);
		bodyMessages[(increment ? curBodyY++ : curBodyY)] = message;
		bodyColors[(increment ? curBodyY : curBodyY)] = color;
		setColor(color);
		cout << message;
	} else {
		if(increment)
			for (unsigned short i = 0; i < MAX_BODY_MESSAGE_COUNT - 1; i++) {
				setCursor(BODY_OFFSET_X, i + 1, true);
				bodyMessages[i] = bodyMessages[i + 1];
				bodyColors[i] = bodyColors[i + 1];
				setColor(bodyColors[i]);
				cout << bodyMessages[i];
			}
		setCursor(BODY_OFFSET_X, curBodyY, true);
		bodyColors[MAX_BODY_MESSAGE_COUNT - 1] = color;
		bodyMessages[MAX_BODY_MESSAGE_COUNT - 1] = message;
		setColor(color);
		cout << message;
	}
	if(resize)
		setCursor(BODY_OFFSET_X, rows - BODY_OFFSET_X, true);
	if (!self)
		lckMutex.unlock();
}

string TextRenderer::promptInternalInput(string message, bool self) {
	if(!self)
		lckMutex.lock();
	string inputMessage = "";
	//setCursor(BODY_OFFSET_X, curBodyY + 1);
	TextRenderer::pushBody(message, DEFAULT_COLOR, true, false, true);
	//setCursor(BODY_OFFSET_X + (unsigned short)message.length(), curBodyY + 1);
	getline(cin, inputMessage); //TODO: Limit cin to whatever the max can be.
	//cin.ignore();
	TextRenderer::pushBody(message.append(inputMessage), DEFAULT_COLOR, false, true, true);
	if (!self)
		lckMutex.unlock();
	return inputMessage;
}

string TextRenderer::promptBottomInput() {
	string inputMessage = "";
	setCursor(BODY_OFFSET_X, rows - BODY_OFFSET_X, true);
	cout << string(bodyMessageSize, ' ');
	setCursor(BODY_OFFSET_X, rows - BODY_OFFSET_X, true);
	getline(cin, inputMessage);

	return inputMessage;
}

void TextRenderer::updateTopRight(string * values, unsigned short * valueColors, unsigned short size) {
	for (unsigned short i = 0; i < MAX_COLUMN_MESSAGE_COUNT && i < size; i++) {
		values[i].resize(MAX_COLUMN_MESSAGE_SIZE);
		setCursor(FRAME_WIDTH - COLUMN_WIDTH + BODY_OFFSET_X, i + 1);
		setColor(valueColors[i]);
		cout << values[i];
	}
}

void TextRenderer::updateTopBottom(string * values, unsigned short * valueColors, unsigned short size) {
	for (unsigned short i = 0; i < MAX_COLUMN_MESSAGE_COUNT && i < size; i++) {
		values[i].resize(MAX_COLUMN_MESSAGE_SIZE);
		setCursor(FRAME_WIDTH - COLUMN_WIDTH + BODY_OFFSET_X, FRAME_HEIGHT - BODY_OFFSET_X + 1);
		setColor(valueColors[i]);
		cout << values[i];
	}
}

void TextRenderer::getConsoleSize(unsigned short & rows, unsigned short & columns, bool self) { //https://stackoverflow.com/questions/23369503/get-size-of-terminal-window-rows-columns
	static HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	static CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(stdOut, &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void TextRenderer::setColor(unsigned short color) {
	static HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(stdOut, color);
}