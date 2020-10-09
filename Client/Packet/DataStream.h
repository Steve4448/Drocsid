#ifndef DATASTREAM_H_
#define DATASTREAM_H_
#include <string>
#include <mutex>

class Cursor {
public:
	Cursor(unsigned short size);
	void reset();
	unsigned short operator++(int);
	unsigned short operator+=(int);
	unsigned short operator--(int);
	unsigned short operator-=(int);
	unsigned short getPosition();
private:
	const unsigned short size;
	unsigned short position;
};

class DataStream
{
public:
	DataStream(unsigned short size);
	~DataStream();
	void resetWrite();
	void resetRead();
	char* getInputBuffer();
	char* getOutputBuffer();
	unsigned short getSize();
	Cursor& getReadIndex();
	Cursor& getWriteIndex();
private:
	const unsigned short size;
	Cursor readIndex;
	Cursor writeIndex;
	char* inBuf;
	char* outBuf;
	/* Writing Variables*/
	friend DataStream& operator<<(DataStream& dataStream, const char* toWrite);
	friend DataStream& operator<<(DataStream& dataStream, const std::string& toWrite);
	friend DataStream& operator<<(DataStream& dataStream, const int& toWrite);
	friend DataStream& operator<<(DataStream& dataStream, const unsigned short& toWrite);
	friend DataStream& operator<<(DataStream& dataStream, const bool& toWrite);

	/* Reading Variables*/
	friend DataStream& operator>>(DataStream& dataStream, std::string& toRead);
	friend DataStream& operator>>(DataStream& dataStream, int& toRead);
	friend DataStream& operator>>(DataStream& dataStream, unsigned short& toRead);
	friend DataStream& operator>>(DataStream& dataStream, bool& toRead);
};

#endif //DATASTREAM_H_
