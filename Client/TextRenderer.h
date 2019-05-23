#ifndef TEXTRENDERER_H_
#define TEXTRENDERER_H_
#define FRAME_WIDTH 75
#define FRAME_HEIGHT 30
#define COLUMN_WIDTH 20
#define MAX_BODY_MESSAGE_COUNT 24
#define MAX_BODY_MESSAGE_SIZE 80
#define MAX_COLUMN_MESSAGE_COUNT 10
#define MAX_COLUMN_MESSAGE_SIZE 10
#define BODY_OFFSET_X 2
#include <string>
#include <Windows.h>
#include <mutex>
#include "Constants.h"
class TextRenderer
{
public:
	static void setCursor(unsigned short x, unsigned short y, bool self=false);
	static void renderFrame(bool displayColumns);
	static void pushBody(std::string message, unsigned short color = DEFAULT_COLOR, bool increment=true, bool resize=true, bool self = false);
	static std::string promptInternalInput(std::string message, bool self = false);
	static std::string promptBottomInput();
	static void updateTopRight(std::string * values, unsigned short * valueColors, unsigned short size);
	static void updateTopBottom(std::string * values, unsigned short * valueColors, unsigned short size);
	static void getConsoleSize(unsigned short & rows, unsigned short & columns, bool self = false);
	static void setColor(unsigned short color);
private:
	static unsigned short curBodyY;
	static unsigned short rows;
	static unsigned short columns;
	static unsigned short bodyMessageSize;
	static std::string bodyMessages[];
	static unsigned short bodyColors[];
	static std::mutex lckMutex;
};
#endif