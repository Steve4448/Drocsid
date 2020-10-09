#include "DataStream.h"
#include <string>
#include <iostream>
using namespace std;

Cursor::Cursor(unsigned short size) : size(size), position(0) {

}

/* Increment the cursor position.
	Throws an exception if the cursor becomes beyond the size specified.
*/
unsigned short Cursor::operator++(int) {
	if (position + 1 >= size) {
		throw exception("Cursor OOB");
	}
	return position++;
}

/* Increment the cursor position by an amount.
	Throws an exception if the cursor becomes beyond the size specified.
*/
unsigned short Cursor::operator+=(int amount) {
	position += amount;
	if (position >= size) {
		throw exception("Cursor OOB");
	}
	return position;
}


/* Decrement the cursor position.
	Throws an exception if the cursor becomes lower than 0.
*/
unsigned short Cursor::operator--(int) {
	if (position == 0) {
		throw exception("Cursor OOB");
	}
	return position--;
}

/* Decrement the cursor position by an amount.
	Throws an exception if the cursor becomes lower than 0.
*/
unsigned short Cursor::operator-=(int amount) {
	int test = (int)position - amount;
	if (test < 0) {
		throw exception("Cursor OOB");
	}
	position -= amount;
	return position;
}

/* Returns the current position. */
unsigned short Cursor::getPosition() {
	return position;
}

/* Resets the position to 0. */
void Cursor::reset() {
	position = 0;
}

DataStream::DataStream(unsigned short size) : size(size), readIndex(size), writeIndex(size) {
	inBuf = new char[size];
	outBuf = new char[size];
	resetRead();
	resetWrite();
}

/* Returns the input buffer. */
char* DataStream::getInputBuffer() {
	return inBuf;
}

/* Returns the output buffer. */
char* DataStream::getOutputBuffer() {
	return outBuf;
}

/* Returns the size of the stream. */
unsigned short DataStream::getSize() {
	return size;
}

/* Returns the cursor for writing. */
Cursor& DataStream::getWriteIndex() {
	return writeIndex;
}

/* Returns the cursor for reading. */
Cursor& DataStream::getReadIndex() {
	return readIndex;
}

/* Resets the write cursor and output buffer. */
void DataStream::resetWrite() {
	writeIndex.reset();
	for (unsigned short i = 0; i < size; i++) {
		outBuf[i] = '\0';
	}
}

/* Resets the read cursor and input buffer. */
void DataStream::resetRead() {
	readIndex.reset();
	for (unsigned short i = 0; i < size; i++) {
		inBuf[i] = '\0';
	}
}

/* Writes an integer to the output stream. */
DataStream& operator<<(DataStream& dataStream, const int& toWrite) {
	Cursor& idx = dataStream.getWriteIndex();
	char* buf = dataStream.getOutputBuffer();
	buf[idx++] = (toWrite >> 24) & 0xFF;
	buf[idx++] = (toWrite >> 16) & 0xFF;
	buf[idx++] = (toWrite >> 8) & 0xFF;
	buf[idx++] = toWrite & 0xFF;
	return dataStream;
}

/* Reads an integer from the input stream. */
DataStream& operator>>(DataStream& dataStream, int& toRead) {
	Cursor& idx = dataStream.getReadIndex();
	char* buf = dataStream.getInputBuffer();
	toRead = ((buf[idx++] & 0xFF) << 24)
		| ((buf[idx++] & 0xFF) << 16)
		| ((buf[idx++] & 0xFF) << 8)
		| (buf[idx++] & 0xFF);
	return dataStream;
}

/* Writes an unsigned short to the output stream. */
DataStream& operator<<(DataStream& dataStream, const unsigned short& toWrite) {
	Cursor& idx = dataStream.getWriteIndex();
	char* buf = dataStream.getOutputBuffer();
	buf[idx++] = (toWrite >> 8) & 0xFF;
	buf[idx++] = toWrite & 0xFF;
	return dataStream;
}

/* Reads an unsigned short from the input stream. */
DataStream& operator>>(DataStream& dataStream, unsigned short& toRead) {
	Cursor& idx = dataStream.getReadIndex();
	char* buf = dataStream.getInputBuffer();
	toRead = ((buf[idx++] & 0xFF) << 8)
		| (buf[idx++] & 0xFF);
	return dataStream;
}

/* First writes the size of the string to the output stream and then the string itself. */
DataStream& operator<<(DataStream& dataStream, const string& toWrite) {
	dataStream << (unsigned short)toWrite.size();
	Cursor& idx = dataStream.getWriteIndex();
	int curIdx = idx.getPosition();
	idx += (int)toWrite.size();
	memcpy(dataStream.getOutputBuffer() + curIdx, toWrite.c_str(), toWrite.size());
	return dataStream;
}

/* Converts a char array into a string and then writes it via the string implementation. */
DataStream& operator<<(DataStream& dataStream, const char* toWrite) {
	return (dataStream << string(toWrite));
}

/* First reads the size of the string from the input stream and then the string itself. */
DataStream& operator>>(DataStream& dataStream, string& toRead) {
	Cursor& idx = dataStream.getReadIndex();
	unsigned short stringLength = 0;
	dataStream >> stringLength;
	int curIdx = idx.getPosition();
	idx += stringLength;
	char* tempArray = new char[stringLength + 1];
	tempArray[stringLength] = '\0';
	memcpy(tempArray, dataStream.getInputBuffer() + curIdx, stringLength);
	toRead.append(tempArray);
	delete[] tempArray;
	return dataStream;
}

/* Reads a boolean from the input stream. */
DataStream& operator>>(DataStream& dataStream, bool& toRead) {
	Cursor& idx = dataStream.getReadIndex();
	char* buf = dataStream.getInputBuffer();
	toRead = buf[idx++] == 1;
	return dataStream;
}


/* Writes a boolean to the output stream.. */
DataStream& operator<<(DataStream& dataStream, const bool& toWrite) {
	Cursor& idx = dataStream.getWriteIndex();
	char* buf = dataStream.getOutputBuffer();
	buf[idx++] = toWrite ? 1 : 0;
	return dataStream;
}

DataStream::~DataStream() {
	delete[] inBuf;
	delete[] outBuf;
}