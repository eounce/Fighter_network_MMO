#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "RingBuffer.h"

RingBuffer::RingBuffer(int bufferSize)
{
	_buffer = (char*)malloc(bufferSize);
	_bufferSize = bufferSize;
	InitializeSRWLock(&_srw);
}

RingBuffer::~RingBuffer()
{
	free(_buffer);
}

int RingBuffer::GetBufferSize()
{
	return _bufferSize;
}

bool RingBuffer::Enqueue(char* pData, int size)
{
	if (GetFreeSize() < size) return false;

	int rear = (_rear + 1) % _bufferSize;
	if (rear + size > _bufferSize)
	{
		int overIdx = (rear + size) - _bufferSize;
		memcpy_s(&_buffer[rear], size - overIdx, pData, size - overIdx);
		pData += (size - overIdx);
		size -= (size - overIdx);

		rear = 0;
	}

	memcpy_s(&_buffer[rear], size, pData, size);
	_rear = rear + (size - 1);

	return true;
}

bool RingBuffer::Dequeue(char* pDest, int size)
{
	if (GetUseSize() < size) return false;

	int front = (_front + 1) % _bufferSize;
	if (front + size > _bufferSize)
	{
		int overIdx = (front + size) - _bufferSize;
		memcpy_s(pDest, size - overIdx, &_buffer[front], size - overIdx);
		pDest += (size - overIdx);
		size -= (size - overIdx);

		front = 0;
	}

	memcpy_s(pDest, size, &_buffer[front], size);
	_front = front + (size - 1);

	return true;
}

bool RingBuffer::Peek(char* pDest, int size)
{
	if (GetUseSize() < size) return false;

	int front = (_front + 1) % _bufferSize;
	if (front + size > _bufferSize)
	{
		int overIdx = (front + size) - _bufferSize;
		memcpy_s(pDest, size - overIdx, &_buffer[front], size - overIdx);
		pDest += (size - overIdx);
		size -= (size - overIdx);

		front = 0;
	}

	memcpy_s(pDest, size, &_buffer[front], size);
	return true;
}

int RingBuffer::GetUseSize()
{
	int size;
	if (_rear >= _front)
		size = _rear - _front;
	else
		size = _bufferSize + (_rear - _front);

	return size;
}

int RingBuffer::GetFreeSize()
{
	return _bufferSize - GetUseSize() - 1;
}

void RingBuffer::ClearBuffer()
{
	_rear = 0;
	_front = 0;
}

int RingBuffer::DirectEnqueueSize()
{
	int size = (_rear + 1) % _bufferSize;
	if (size > _front)
		return _bufferSize - size;
	else
		return _front - size;
}

int RingBuffer::DirectDequeueSize()
{
	int size = (_front + 1) % _bufferSize;
	if (size > _rear)
		return _bufferSize - size;
	else
		return _rear - size + 1;
}

char* RingBuffer::GetRearBufferPtr()
{
	return &_buffer[(_rear + 1) % _bufferSize];
}

char* RingBuffer::GetFrontBufferPtr()
{
	return &_buffer[(_front + 1) % _bufferSize];
}

void RingBuffer::MoveRear(int size)
{
	_rear = (_rear + size) % _bufferSize;
}

void RingBuffer::MoveFront(int size)
{
	_front = (_front + size) % _bufferSize;
}

void RingBuffer::Lock(LOCK type)
{
	if (type == EXCLUSIVE)
		AcquireSRWLockExclusive(&_srw);
	else
		AcquireSRWLockShared(&_srw);
}

void RingBuffer::Unlock(LOCK type)
{
	if (type == EXCLUSIVE)
		ReleaseSRWLockExclusive(&_srw);
	else
		ReleaseSRWLockShared(&_srw);
}