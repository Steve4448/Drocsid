#include "ConsoleHandler.h"
#include "../Exception/StartupException.h"
#include <chrono>
#include <sstream>
using namespace std;

ConsoleHandler::ConsoleHandler() :
	running(true),
	inputStringFinished(false),
	inputStdHandle(GetStdHandle(STD_INPUT_HANDLE)),
	outputStdHandle(GetStdHandle(STD_OUTPUT_HANDLE)),
	lastResizeCheck(0),
	resizeNeeded(false),
	messageHistory{ nullptr },
	topWidgetHistory{ nullptr },
	bottomWidgetHistory{ nullptr },
	curBodyMessageIdx(0) {
	if (inputStdHandle == INVALID_HANDLE_VALUE || outputStdHandle == INVALID_HANDLE_VALUE)
		throw StartupException("GetStdHandle was invalid.");
	curCursor.X = 0;
	curCursor.Y = 0;
	handleResize();
	inputThreadInstance = thread(&ConsoleHandler::readInput, this);
	resizeThreadInstance = thread(&ConsoleHandler::checkResize, this);
	SetConsoleTitle(wstring(L"Drocsid").c_str());
}

void ConsoleHandler::checkResize() { //Hacky way of delaying the resize to only happen a maximum of once every 100ms.
	while (running) {
		if (chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - lastResizeCheck >= 100) {
			if (resizeNeeded) {
				handleResize();
				resizeNeeded = false;
			}
			lastResizeCheck = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
		}
		this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void ConsoleHandler::handleResize() {
	outputMutex.lock();
	GetConsoleScreenBufferInfo(outputStdHandle, &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	for (unsigned short y = 0; y < rows; y++) {
		setCursor(0, y);
		cout << string(columns, ' ');
	}

	setColor(BORDER_COLOR);

	for (unsigned short x = 1; x < columns-1; x++) {
		setCursor(x, 0);
		cout << "\xCD";
		setCursor(x, rows-1);
		cout << "\xCD";
	}
	for (unsigned short y = 1; y < rows - 1; y++) {
		setCursor(0, y);
		cout << "\xBA";
		setCursor(columns-1, y);
		cout << "\xBA";
	}

	setCursor(0, 0);
	cout << "\xC9";
	setCursor(columns-1, 0);
	cout << "\xBB";
	setCursor(0, rows-1);
	cout << "\xC8";
	setCursor(columns - 1, rows-1);
	cout << "\xBC";

	for (unsigned short y = 1; y < rows - 1; y++) {
		setCursor(columns - 1 - WIDGET_SIZE, y);
		cout << "\xBA";
	}
	for (unsigned short x = columns-WIDGET_SIZE; x < columns-1; x++) {
		setCursor(x, rows/2);
		cout << "\xCD";
	}
	setCursor(columns - 1 - WIDGET_SIZE, rows/2);
	cout << "\xCC";
	setCursor(columns - 1, rows / 2);
	cout << "\xB9";
	setCursor(columns - 1 - WIDGET_SIZE, 0);
	cout << "\xCB";
	setCursor(columns - 1 - WIDGET_SIZE, rows-1);
	cout << "\xCA";

	topWidgetStart.X = columns - WIDGET_SIZE;
	topWidgetStart.Y = 1;
	bottomWidgetStart.X = columns - WIDGET_SIZE;
	bottomWidgetStart.Y = rows / 2 + 1;
	string topNames[MAX_WIDGET_MESSAGES]{ "" };
	unsigned short topColors[MAX_WIDGET_MESSAGES]{ 0 };
	for (unsigned short i = 0; i < MAX_WIDGET_MESSAGES; i++) {
		if (topWidgetHistory[i] == nullptr)
			continue;
		topNames[i] = topWidgetHistory[i]->getMessage();
		topColors[i] = topWidgetHistory[i]->getColor();
	}
	updateTopRight(topNames, topColors, MAX_WIDGET_MESSAGES, false);

	string bottomNames[MAX_WIDGET_MESSAGES]{ "" };
	unsigned short bottomColors[MAX_WIDGET_MESSAGES]{ 0 };
	for (unsigned short i = 0; i < MAX_WIDGET_MESSAGES; i++) {
		if (bottomWidgetHistory[i] == nullptr)
			continue;
		bottomNames[i] = bottomWidgetHistory[i]->getMessage();
		bottomColors[i] = bottomWidgetHistory[i]->getColor();
	}
	updateBottomRight(bottomNames, bottomColors, MAX_WIDGET_MESSAGES, false);

	setColor(DEFAULT_COLOR);
	setCursor(1, 1);
	for (unsigned short i = 0; i < curBodyMessageIdx; i++) {
		MessageEntry * entry = messageHistory[i];
		if (entry != nullptr)
			pushBodyMessage(entry->getMessage(), entry->getDefaultColor(), entry->doesNewLine(), false);
	}

	setCursor(1, curCursor.Y);
	cout << currentInputString;

	outputMutex.unlock();
}

void ConsoleHandler::scrollScreenUp() {
	if (curCursor.Y + 1 > rows - 2) {

		SMALL_RECT srctScrollRect, srctClipRect;
		CHAR_INFO chiFill;
		COORD coordDest;
		srctScrollRect.Top = 1;
		srctScrollRect.Bottom = rows-2;
		srctScrollRect.Left = 1;
		srctScrollRect.Right = columns-2-WIDGET_SIZE;

		coordDest.X = 1;
		coordDest.Y = 0;

		srctClipRect = srctScrollRect;
		chiFill.Attributes = 0;
		chiFill.Char.AsciiChar = (char)' ';
		ScrollConsoleScreenBuffer(outputStdHandle, &srctScrollRect, &srctClipRect, coordDest, &chiFill);
		setCursor(1, curCursor.Y);
	} else {
		setCursor(1, curCursor.Y + 1);
	}
}

void ConsoleHandler::pushBodyMessage(string message, unsigned short defaultColor, bool newLine, bool push) {
	if (push) {
		outputMutex.lock();
		if(curBodyMessageIdx < MAX_BODY_MESSAGES)
			messageHistory[curBodyMessageIdx++] = new MessageEntry(message, defaultColor, newLine);
		else {
			delete messageHistory[0];
			for (unsigned short i = 0; i < MAX_BODY_MESSAGES-1; i++) {
				messageHistory[i] = messageHistory[i + 1];
			}
			messageHistory[curBodyMessageIdx - 1] = new MessageEntry(message, defaultColor, newLine);
		}
		if (!currentInputString.empty()) {
			setCursor(1, curCursor.Y);
			cout << string(columns - 2 - WIDGET_SIZE, ' ');
			setCursor(1, curCursor.Y);
		}
	}
	setColor(defaultColor);
	bool parsingColor = false;
	string parsedColor = "";
	for (size_t curPos = 0; curPos < message.length(); curPos++) {
		if (!parsingColor) {
			if (message.at(curPos) == '<') {
				parsingColor = true;
			} else if (message.at(curPos) == '\n') {
				scrollScreenUp();
			} else {
				cout << message.at(curPos);
				if (curCursor.X + 1 >= columns - 2 - WIDGET_SIZE)
					scrollScreenUp();
				else
					setCursor(curCursor.X + 1, curCursor.Y);
			}
		} else {
			if (message.at(curPos) == '>') {
				unsigned short color;
				istringstream iss(parsedColor);
				iss >> color;
				if (!iss.good()) {
					setColor(color);
				}
				parsedColor = "";
				parsingColor = false;
			} else {
				parsedColor.append(1, message.at(curPos));
			}
		}
	}
	if(newLine)
		scrollScreenUp();
	setColor(DEFAULT_COLOR);

	if (push) {
		if (!currentInputString.empty()) {
			setCursor(1, curCursor.Y);
			string outputString;
			outputString.append(currentInputString);
			outputString.resize(columns);
			cout << outputString;
			setCursor(1 + currentInputString.length(), curCursor.Y);
		}
		outputMutex.unlock();
	}
}

string ConsoleHandler::getBlockingInput(string promptMessage) {
	if(!promptMessage.empty())
		pushBodyMessage(promptMessage);
	unique_lock<mutex> ucv_m(inputMutex);
	while (!inputStringFinished) {
		cv.wait(ucv_m);
	}

	string returnString = currentInputString;
	setCursor(1, curCursor.Y);
	currentInputString = "";
	pushBodyMessage(returnString);
	inputStringFinished = false;

	return returnString;
}

void ConsoleHandler::readInput() {
	while (running) {
		bool updateTextEntry = false;
		DWORD recordsRead;
		ReadConsoleInput(inputStdHandle, inputBuffer, INPUT_BUFFER_SIZE, &recordsRead);
		outputMutex.lock();
		for (unsigned int i = 0; i < recordsRead; i++) {
			switch (inputBuffer[i].EventType) {
			case KEY_EVENT: // keyboard input 
				updateTextEntry = true;
				KEY_EVENT_RECORD ker = inputBuffer[i].Event.KeyEvent;
				switch (ker.wVirtualKeyCode) { //Keys that aren't effected by modifiers
				case VK_BACK:
					if (ker.bKeyDown && !currentInputString.empty()) {
						currentInputString = currentInputString.substr(0, currentInputString.length() - 1);
					}
					break;
				case VK_ESCAPE:
					if (ker.bKeyDown && !currentInputString.empty()) {
						currentInputString = "";
					}
					break;
				case VK_TAB:
					if(ker.bKeyDown)
						currentInputString.append(ker.wRepeatCount, '\t');
					break;
				case VK_RETURN:
					if (ker.bKeyDown) {
						inputStringFinished = true;
						cv.notify_all();
					}
					break;
				case VK_SPACE:
					if (ker.bKeyDown)
						currentInputString.append(ker.wRepeatCount, ' ');
					break;
				default: //Keys that are effected by modifiers
					if ((ker.bKeyDown && ker.dwControlKeyState & CAPSLOCK_ON) || (ker.dwControlKeyState & SHIFT_PRESSED)) {
						bool shiftSnowflake = true;
						if (ker.dwControlKeyState & SHIFT_PRESSED) { //keys that act differently from having caps lock on versus shift.
							switch (ker.wVirtualKeyCode) {
							case 0x30: //0
								currentInputString.append(ker.wRepeatCount, ')');
								break;
							case 0x31: //1
								currentInputString.append(ker.wRepeatCount, '!');
								break;
							case 0x32: //2
								currentInputString.append(ker.wRepeatCount, '@');
								break;
							case 0x33: //3
								currentInputString.append(ker.wRepeatCount, '#');
								break;
							case 0x34: //4
								currentInputString.append(ker.wRepeatCount, '$');
								break;
							case 0x35: //5
								currentInputString.append(ker.wRepeatCount, '%');
								break;
							case 0x36: //6
								currentInputString.append(ker.wRepeatCount, '^');
								break;
							case 0x37: //7
								currentInputString.append(ker.wRepeatCount, '&');
								break;
							case 0x38: //8
								currentInputString.append(ker.wRepeatCount, '*');
								break;
							case 0x39: //9
								currentInputString.append(ker.wRepeatCount, '(');
								break;
							case VK_OEM_1: //;
								currentInputString.append(ker.wRepeatCount, ':');
								break;
							case VK_OEM_PLUS: //=
								currentInputString.append(ker.wRepeatCount, '+');
								break;
							case VK_OEM_COMMA: //,
								currentInputString.append(ker.wRepeatCount, '<');
								break;
							case VK_OEM_MINUS: //-
								currentInputString.append(ker.wRepeatCount, '_');
								break;
							case VK_OEM_PERIOD: //.
								currentInputString.append(ker.wRepeatCount, '>');
								break;
							case VK_OEM_2: ///
								currentInputString.append(ker.wRepeatCount, '?');
								break;
							case VK_OEM_3: //`
								currentInputString.append(ker.wRepeatCount, '~');
								break;
							case VK_OEM_4: //[
								currentInputString.append(ker.wRepeatCount, '{');
								break;
							case VK_OEM_5: //\|
								currentInputString.append(ker.wRepeatCount, '|');
								break;
							case VK_OEM_6: //]
								currentInputString.append(ker.wRepeatCount, '}');
								break;
							case VK_OEM_7: //'
								currentInputString.append(ker.wRepeatCount, '"');
								break;
							default:
								shiftSnowflake = false;
								break;
							}
						}
						if (shiftSnowflake)
							break;
						switch (ker.wVirtualKeyCode) {
						case 0x30: //0
							currentInputString.append(ker.wRepeatCount, '0');
							break;
						case 0x31: //1
							currentInputString.append(ker.wRepeatCount, '1');
							break;
						case 0x32: //2
							currentInputString.append(ker.wRepeatCount, '2');
							break;
						case 0x33: //3
							currentInputString.append(ker.wRepeatCount, '3');
							break;
						case 0x34: //4
							currentInputString.append(ker.wRepeatCount, '4');
							break;
						case 0x35: //5
							currentInputString.append(ker.wRepeatCount, '5');
							break;
						case 0x36: //6
							currentInputString.append(ker.wRepeatCount, '6');
							break;
						case 0x37: //7
							currentInputString.append(ker.wRepeatCount, '7');
							break;
						case 0x38: //8
							currentInputString.append(ker.wRepeatCount, '8');
							break;
						case 0x39: //9
							currentInputString.append(ker.wRepeatCount, '9');
							break;
						case VK_OEM_1: //;
							currentInputString.append(ker.wRepeatCount, ';');
							break;
						case VK_OEM_PLUS: //=
							currentInputString.append(ker.wRepeatCount, '=');
							break;
						case VK_OEM_COMMA: //,
							currentInputString.append(ker.wRepeatCount, ',');
							break;
						case VK_OEM_MINUS: //-
							currentInputString.append(ker.wRepeatCount, '-');
							break;
						case VK_OEM_PERIOD: //.
							currentInputString.append(ker.wRepeatCount, '.');
							break;
						case VK_OEM_2: ///
							currentInputString.append(ker.wRepeatCount, '/');
							break;
						case VK_OEM_3: //`
							currentInputString.append(ker.wRepeatCount, '`');
							break;
						case VK_OEM_4: //[
							currentInputString.append(ker.wRepeatCount, '[');
							break;
						case VK_OEM_5: //\|
							currentInputString.append(ker.wRepeatCount, '\\');
							break;
						case VK_OEM_6: //]
							currentInputString.append(ker.wRepeatCount, ']');
							break;
						case VK_OEM_7: //'
							currentInputString.append(ker.wRepeatCount, '\'');
							break;
						case 0x41: //A
							currentInputString.append(ker.wRepeatCount, 'A');
							break;
						case 0x42: //B
							currentInputString.append(ker.wRepeatCount, 'B');
							break;
						case 0x43: //C
							currentInputString.append(ker.wRepeatCount, 'C');
							break;
						case 0x44: //D
							currentInputString.append(ker.wRepeatCount, 'D');
							break;
						case 0x45: //E
							currentInputString.append(ker.wRepeatCount, 'E');
							break;
						case 0x46: //F
							currentInputString.append(ker.wRepeatCount, 'F');
							break;
						case 0x47: //G
							currentInputString.append(ker.wRepeatCount, 'G');
							break;
						case 0x48: //H
							currentInputString.append(ker.wRepeatCount, 'H');
							break;
						case 0x49: //I
							currentInputString.append(ker.wRepeatCount, 'I');
							break;
						case 0x4A: //J
							currentInputString.append(ker.wRepeatCount, 'J');
							break;
						case 0x4B: //K
							currentInputString.append(ker.wRepeatCount, 'K');
							break;
						case 0x4C: //L
							currentInputString.append(ker.wRepeatCount, 'L');
							break;
						case 0x4D: //M
							currentInputString.append(ker.wRepeatCount, 'M');
							break;
						case 0x4E: //N
							currentInputString.append(ker.wRepeatCount, 'N');
							break;
						case 0x4F: //O
							currentInputString.append(ker.wRepeatCount, 'O');
							break;
						case 0x50: //P
							currentInputString.append(ker.wRepeatCount, 'P');
							break;
						case 0x51: //Q
							currentInputString.append(ker.wRepeatCount, 'Q');
							break;
						case 0x52: //R
							currentInputString.append(ker.wRepeatCount, 'R');
							break;
						case 0x53: //S
							currentInputString.append(ker.wRepeatCount, 'S');
							break;
						case 0x54: //T
							currentInputString.append(ker.wRepeatCount, 'T');
							break;
						case 0x55: //U
							currentInputString.append(ker.wRepeatCount, 'U');
							break;
						case 0x56: //V
							currentInputString.append(ker.wRepeatCount, 'V');
							break;
						case 0x57: //W
							currentInputString.append(ker.wRepeatCount, 'W');
							break;
						case 0x58: //X
							currentInputString.append(ker.wRepeatCount, 'X');
							break;
						case 0x59: //Y
							currentInputString.append(ker.wRepeatCount, 'Y');
							break;
						case 0x5A: //Z
							currentInputString.append(ker.wRepeatCount, 'Z');
							break;
						} //Now, I know my ABC's. Next time won't you sing with me?
					} else if (ker.bKeyDown) { //No meaninful modifiers active.
						switch (ker.wVirtualKeyCode) {
						case 0x30: //0
							currentInputString.append(ker.wRepeatCount, '0');
							break;
						case 0x31: //1
							currentInputString.append(ker.wRepeatCount, '1');
							break;
						case 0x32: //2
							currentInputString.append(ker.wRepeatCount, '2');
							break;
						case 0x33: //3
							currentInputString.append(ker.wRepeatCount, '3');
							break;
						case 0x34: //4
							currentInputString.append(ker.wRepeatCount, '4');
							break;
						case 0x35: //5
							currentInputString.append(ker.wRepeatCount, '5');
							break;
						case 0x36: //6
							currentInputString.append(ker.wRepeatCount, '6');
							break;
						case 0x37: //7
							currentInputString.append(ker.wRepeatCount, '7');
							break;
						case 0x38: //8
							currentInputString.append(ker.wRepeatCount, '8');
							break;
						case 0x39: //9
							currentInputString.append(ker.wRepeatCount, '9');
							break;
						case VK_OEM_1: //;
							currentInputString.append(ker.wRepeatCount, ';');
							break;
						case VK_OEM_PLUS: //=
							currentInputString.append(ker.wRepeatCount, '=');
							break;
						case VK_OEM_COMMA: //,
							currentInputString.append(ker.wRepeatCount, ',');
							break;
						case VK_OEM_MINUS: //-
							currentInputString.append(ker.wRepeatCount, '-');
							break;
						case VK_OEM_PERIOD: //.
							currentInputString.append(ker.wRepeatCount, '.');
							break;
						case VK_OEM_2: ///
							currentInputString.append(ker.wRepeatCount, '/');
							break;
						case VK_OEM_3: //`
							currentInputString.append(ker.wRepeatCount, '`');
							break;
						case VK_OEM_4: //[
							currentInputString.append(ker.wRepeatCount, '[');
							break;
						case VK_OEM_5: //\|
							currentInputString.append(ker.wRepeatCount, '\\');
							break;
						case VK_OEM_6: //]
							currentInputString.append(ker.wRepeatCount, ']');
							break;
						case VK_OEM_7: //'
							currentInputString.append(ker.wRepeatCount, '\'');
							break;
						case 0x41: //A
							currentInputString.append(ker.wRepeatCount, 'a');
							break;
						case 0x42: //B
							currentInputString.append(ker.wRepeatCount, 'b');
							break;
						case 0x43: //C
							currentInputString.append(ker.wRepeatCount, 'c');
							break;
						case 0x44: //D
							currentInputString.append(ker.wRepeatCount, 'd');
							break;
						case 0x45: //E
							currentInputString.append(ker.wRepeatCount, 'e');
							break;
						case 0x46: //F
							currentInputString.append(ker.wRepeatCount, 'f');
							break;
						case 0x47: //G
							currentInputString.append(ker.wRepeatCount, 'g');
							break;
						case 0x48: //H
							currentInputString.append(ker.wRepeatCount, 'h');
							break;
						case 0x49: //I
							currentInputString.append(ker.wRepeatCount, 'i');
							break;
						case 0x4A: //J
							currentInputString.append(ker.wRepeatCount, 'j');
							break;
						case 0x4B: //K
							currentInputString.append(ker.wRepeatCount, 'k');
							break;
						case 0x4C: //L
							currentInputString.append(ker.wRepeatCount, 'l');
							break;
						case 0x4D: //M
							currentInputString.append(ker.wRepeatCount, 'm');
							break;
						case 0x4E: //N
							currentInputString.append(ker.wRepeatCount, 'n');
							break;
						case 0x4F: //O
							currentInputString.append(ker.wRepeatCount, 'o');
							break;
						case 0x50: //P
							currentInputString.append(ker.wRepeatCount, 'p');
							break;
						case 0x51: //Q
							currentInputString.append(ker.wRepeatCount, 'q');
							break;
						case 0x52: //R
							currentInputString.append(ker.wRepeatCount, 'r');
							break;
						case 0x53: //S
							currentInputString.append(ker.wRepeatCount, 's');
							break;
						case 0x54: //T
							currentInputString.append(ker.wRepeatCount, 't');
							break;
						case 0x55: //U
							currentInputString.append(ker.wRepeatCount, 'u');
							break;
						case 0x56: //V
							currentInputString.append(ker.wRepeatCount, 'v');
							break;
						case 0x57: //W
							currentInputString.append(ker.wRepeatCount, 'w');
							break;
						case 0x58: //X
							currentInputString.append(ker.wRepeatCount, 'x');
							break;
						case 0x59: //Y
							currentInputString.append(ker.wRepeatCount, 'y');
							break;
						case 0x5A: //Z
							currentInputString.append(ker.wRepeatCount, 'z');
							break;
						} //Now, I know my ABC's. Next time won't you sing with me?
					}
					break;
				}

				break;
			case WINDOW_BUFFER_SIZE_EVENT:
				resizeNeeded = true;
				lastResizeCheck = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
				break;
			case MOUSE_EVENT:
			case FOCUS_EVENT:
			case MENU_EVENT:
				break;
			default:
				throw exception("Unknown event type");
			}
			if (updateTextEntry && !inputStringFinished) {
				setCursor(1, curCursor.Y);
				string outputString;
				outputString.append(currentInputString);
				outputString.resize(columns - 2 - WIDGET_SIZE);
				cout << outputString;
				setCursor(1 + currentInputString.length(), curCursor.Y);
			}
		}
		outputMutex.unlock();
	}
}

void ConsoleHandler::updateTopRight(string * names, unsigned short * colors, unsigned short count, bool push) {
	if(push)
		outputMutex.lock();
	COORD lastCursor = curCursor;
	setCursor(topWidgetStart.X + 1, topWidgetStart.Y - 1);
	setColor(BORDER_COLOR);
	cout << "Room List";
	for (unsigned short i = 0; i < MAX_WIDGET_MESSAGES; i++) {
		setCursor(topWidgetStart.X, topWidgetStart.Y + i);
		cout << string(WIDGET_SIZE - 1, ' ');
	}
	for (unsigned short i = 0; i < min(count, MAX_WIDGET_MESSAGES); i++) {
		setCursor(topWidgetStart.X, topWidgetStart.Y + i);
		setColor(colors[i]);
		cout << names[i];
		if (push) {
			delete topWidgetHistory[i];
			topWidgetHistory[i] = new WidgetEntry(names[i], colors[i]);
		}
	}
	setColor(DEFAULT_COLOR);
	setCursor(lastCursor.X, lastCursor.Y);
	if (push)
		outputMutex.unlock();
}

void ConsoleHandler::updateBottomRight(string * names, unsigned short * colors, unsigned short count, bool push) {
	if (push)
		outputMutex.lock();
	COORD lastCursor = curCursor;
	setCursor(bottomWidgetStart.X + 1, bottomWidgetStart.Y - 1);
	setColor(BORDER_COLOR);
	cout << "Friend List";
	for (unsigned short i = 0; i < MAX_WIDGET_MESSAGES; i++) {
		setCursor(bottomWidgetStart.X, bottomWidgetStart.Y + i);
		cout << string(WIDGET_SIZE - 1, ' ');
	}
	for (unsigned short i = 0; i < min(count, MAX_WIDGET_MESSAGES); i++) {
		setCursor(bottomWidgetStart.X, bottomWidgetStart.Y + i);
		setColor(colors[i]);
		cout << names[i];
		if (push) {
			delete bottomWidgetHistory[i];
			bottomWidgetHistory[i] = new WidgetEntry(names[i], colors[i]);
		}
	}
	setColor(DEFAULT_COLOR);
	setCursor(lastCursor.X, lastCursor.Y);
	if (push)
		outputMutex.unlock();
}

void ConsoleHandler::setCursor(unsigned short x, unsigned short y) {
	curCursor.X = x;
	curCursor.Y = y;
	SetConsoleCursorPosition(outputStdHandle, curCursor);
}

void ConsoleHandler::setColor(unsigned short color) {
	SetConsoleTextAttribute(outputStdHandle, color);
}

ConsoleHandler::~ConsoleHandler() {
	for (unsigned short i = 0; i < MAX_BODY_MESSAGES; i++) {
		delete messageHistory[i];
	}
	for (unsigned short i = 0; i < MAX_WIDGET_MESSAGES; i++) {
		delete topWidgetHistory[i];
		delete bottomWidgetHistory[i];
	}
	running = false;
	if (inputThreadInstance.joinable()) {
		inputThreadInstance.join();
	}
	if (resizeThreadInstance.joinable()) {
		resizeThreadInstance.join();
	}
}
