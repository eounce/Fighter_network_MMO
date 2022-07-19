#include <stdio.h>
#include <string>
#include <Windows.h>
#include "Message.h"

Message::Message()
{
	_bufferSize = BUFFER_DEFAULT;
	_pBuffer = (char*)malloc(BUFFER_DEFAULT);
}

Message::Message(int bufferSize)
{
	_bufferSize = bufferSize;
	_pBuffer = (char*)malloc(bufferSize);
}

Message::~Message()
{
	free(_pBuffer);
}

void Message::release()
{

}

int Message::moveWritePos(int size)
{
	if (size <= 0) return 0;

	int movePos = 0;
	if (_rear + size >= _bufferSize)
		movePos = _bufferSize - (_rear + 1);
	else
		movePos = size;

	_rear += movePos;
	return movePos;
}

int Message::moveReadPos(int size)
{
	if (size <= 0) return 0;

	int movePos;
	if (_front + size > _rear)
		movePos = _rear - _front;
	else
		movePos = size;

	_front += movePos;
	return movePos;
}

int	Message::getData(char* pDest, int size)
{
	if (_front + size > _rear)
		return -1;

	memcpy_s(pDest, size, &_pBuffer[_front], size);
	_front += size;
	_dataSize -= size;

	return size;
}

int	Message::putData(char* pSrc, int srcSize)
{
	if (_rear + srcSize >= _bufferSize)
		return -1;

	memcpy_s(&_pBuffer[_rear], srcSize, pSrc, srcSize);
	_rear += srcSize;
	_dataSize += srcSize;

	return srcSize;
}