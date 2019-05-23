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
	resizeNeeded(false) {
	if (inputStdHandle == INVALID_HANDLE_VALUE || outputStdHandle == INVALID_HANDLE_VALUE)
		throw StartupException("GetStdHandle was invalid.");
	curCursor.X = 0;
	curCursor.Y = 0;
	handleResize();
	inputThreadInstance = thread(&ConsoleHandler::readInput, this);
	resizeThreadInstance = thread(&ConsoleHandler::checkResize, this);
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
	GetConsoleScreenBufferInfo(outputStdHandle, &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void ConsoleHandler::pushBodyMessage(string message, unsigned short defaultColor, bool newLine) {
	outputMutex.lock();
	if (!currentInputString.empty()) {
		setCursor(0, curCursor.Y);
		cout << string(columns, ' ');
		setCursor(0, curCursor.Y);
	}
	setColor(defaultColor);
	bool parsingColor = false;
	string parsedColor = "";
	for (size_t curPos = 0; curPos < message.length(); curPos++) {
		if (!parsingColor) {
			if (message.at(curPos) == '<') {
				parsingColor = true;
			} else if (message.at(curPos) == '\n') {
				setCursor(0, curCursor.Y+1);
			} else {
				cout << message.at(curPos);
				if (curCursor.X+1 >= columns)
					setCursor(0, curCursor.Y+1);
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
		setCursor(0, curCursor.Y+1);
	setColor(DEFAULT_COLOR);

	/*size_t openBracketPos = message.find('<');
	size_t closeBracketPos = message.find('>');
	if (openBracketPos != string::npos && closeBracketPos != string::npos) {
		//cout << "Testing string: " << message << " " << openBracketPos << " " << closeBracketPos << endl;
		//cout << "Color: " << message.substr(openBracketPos + 1, closeBracketPos - openBracketPos - 1) << endl;
		//cout << "Begginning: " << message.substr(0, openBracketPos);
		unsigned short color;
		istringstream iss(message.substr(openBracketPos + 1, closeBracketPos - openBracketPos - 1));
		iss >> color;
		cout << message.substr(0, openBracketPos);
		if (!iss.good()) {
			setColor(color);
		}
		pushBodyMessage(message.substr(closeBracketPos + 1));
	} else {
		//cout << "Rest: " << message << endl;
		cout << message;
	}
	setColor(DEFAULT_COLOR);*/
	if (!currentInputString.empty()) {
		setCursor(0, curCursor.Y);
		string outputString;
		outputString.append(currentInputString);
		outputString.resize(columns);
		cout << outputString;
		setCursor(0 + currentInputString.length(), curCursor.Y);
	}
	outputMutex.unlock();
}

string ConsoleHandler::getBlockingInput(string promptMessage) {
	if(!promptMessage.empty())
		pushBodyMessage(promptMessage);
	unique_lock<mutex> ucv_m(inputMutex);
	while (!inputStringFinished) {
		cv.wait(ucv_m);
	}
	string returnString = currentInputString;
	setCursor(0, curCursor.Y);
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

				//TextRenderer::pushBody("What: " + ker.bKeyDown);
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
				setCursor(0, curCursor.Y);
				string outputString;
				outputString.append(currentInputString);
				outputString.resize(columns);
				cout << outputString;
				setCursor(0 + currentInputString.length(), curCursor.Y);
			}
		}
		outputMutex.unlock();
	}
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
	running = false;
	if (inputThreadInstance.joinable()) {
		inputThreadInstance.join();
	}
	if (resizeThreadInstance.joinable()) {
		resizeThreadInstance.join();
	}
}
