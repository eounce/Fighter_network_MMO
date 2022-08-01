#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MESSAGE_EXCEPTION 1001

class Message
{
protected:
	int _bufferSize;
	int _front = 0;
	int _rear = 0;
	char* _pBuffer;

public:
	enum MESSAGE { BUFFER_DEFAULT = 1400 };
	Message();
	Message(int bufferSize);
	virtual ~Message();

	void release();
	void clear() { _front = _rear = 0; }
	int getBufferSize() { return _bufferSize; }
	int getDataSize() { return _rear - _front; }
	char* getBufferPtr() { return _pBuffer; }

	int moveWritePos(int size);
	int moveReadPos(int size);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	getData(char* pDest, int size);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int	putData(char* pSrc, int srcSize);

	/* ============================================================================= */
	// 연산자 오버로딩
	/* ============================================================================= */
	//Message& operator = (Message& srcPacket);

	//////////////////////////////////////////////////////////////////////////
	// 넣기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	Message& operator << (const unsigned char value);
	Message& operator << (const char value);
	Message& operator << (const WCHAR value);

	Message& operator << (const short value);
	Message& operator << (const unsigned short value);

	Message& operator << (const int value);
	Message& operator << (const unsigned int value);

	Message& operator << (const long value);
	Message& operator << (const unsigned long value);

	Message& operator << (const __int64 value);
	Message& operator << (const unsigned __int64 value);

	Message& operator << (const float value);
	Message& operator << (const double value);


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	Message& operator >> (unsigned char& value);
	Message& operator >> (char& value);
	Message& operator >> (WCHAR& value);

	Message& operator >> (unsigned short& value);
	Message& operator >> (short& value);

	Message& operator >> (unsigned int& value);
	Message& operator >> (int& value);

	Message& operator >> (unsigned long& value);
	Message& operator >> (long& value);

	Message& operator >> (unsigned __int64& value);
	Message& operator >> (__int64& value);

	Message& operator >> (float& value);
	Message& operator >> (double& value);
};

__forceinline Message& Message::operator << (const unsigned char value)
{
	if (_rear + 1 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(unsigned char*)(_pBuffer + _rear) = value;
	_rear += 1;
	return *this;
}

__forceinline Message& Message::operator << (const char value)
{
	if (_rear + 1 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(char*)(_pBuffer + _rear) = value;
	_rear += 1;
	return *this;
}

__forceinline Message& Message::operator << (const WCHAR value)
{
	if (_rear + 2 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(WCHAR*)(_pBuffer + _rear) = value;
	_rear += sizeof(WCHAR);
	return *this;
}

__forceinline Message& Message::operator << (const short value)
{
	if (_rear + 2 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(short*)(_pBuffer + _rear) = value;
	_rear += sizeof(short);
	return *this;
}

__forceinline Message& Message::operator << (const unsigned short value)
{
	if (_rear + 2 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(unsigned short*)(_pBuffer + _rear) = value;
	_rear += sizeof(unsigned short);
	return *this;
}

__forceinline Message& Message::operator << (const int value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(int*)(_pBuffer + _rear) = value;
	_rear += sizeof(int);
	return *this;
}

__forceinline Message& Message::operator << (const unsigned int value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(unsigned int*)(_pBuffer + _rear) = value;
	_rear += sizeof(unsigned int);
	return *this;
}

__forceinline Message& Message::operator << (const long value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(long*)(_pBuffer + _rear) = value;
	_rear += sizeof(long);
	return *this;
}

__forceinline Message& Message::operator << (const unsigned long value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(unsigned long*)(_pBuffer + _rear) = value;
	_rear += sizeof(unsigned long);
	return *this;
}

__forceinline Message& Message::operator << (const __int64 value)
{
	if (_rear + 8 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(__int64*)(_pBuffer + _rear) = value;
	_rear += sizeof(__int64);
	return *this;
}

__forceinline Message& Message::operator << (const unsigned __int64 value)
{
	if (_rear + 8 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(unsigned __int64*)(_pBuffer + _rear) = value;
	_rear += sizeof(unsigned __int64);
	return *this;
}

__forceinline Message& Message::operator << (const float value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(float*)(_pBuffer + _rear) = value;
	_rear += sizeof(float);
	return *this;
}

__forceinline Message& Message::operator << (const double value)
{
	if (_rear + 8 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	*(double*)(_pBuffer + _rear) = value;
	_rear += sizeof(double);
	return *this;
}


// 빼기
__forceinline Message& Message::operator >> (unsigned char& value)
{
	if (_front + 1 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(unsigned char*)(_pBuffer + _front);
	_front += sizeof(unsigned char);
	return *this;
}

__forceinline Message& Message::operator >> (char& value)
{
	if (_front + 1 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(char*)(_pBuffer + _front);
	_front += sizeof(char);
	return *this;
}

__forceinline Message& Message::operator >> (WCHAR& value)
{
	if (_front + 2 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(WCHAR*)(_pBuffer + _front);
	_front += sizeof(WCHAR);
	return *this;
}

__forceinline Message& Message::operator >> (unsigned short& value)
{
	if (_front + 2 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(unsigned short*)(_pBuffer + _front);
	_front += sizeof(unsigned short);
	return *this;
}

__forceinline Message& Message::operator >> (short& value)
{
	if (_front + 2 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(short*)(_pBuffer + _front);
	_front += sizeof(short);
	return *this;
}

__forceinline Message& Message::operator >> (unsigned int& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(unsigned int*)(_pBuffer + _front);
	_front += sizeof(unsigned int);
	return *this;
}

__forceinline Message& Message::operator >> (int& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(int*)(_pBuffer + _front);
	_front += sizeof(int);
	return *this;
}

__forceinline Message& Message::operator >> (unsigned long& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(unsigned long*)(_pBuffer + _front);
	_front += sizeof(unsigned long);
	return *this;
}

__forceinline Message& Message::operator >> (long& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(long*)(_pBuffer + _front);
	_front += sizeof(long);
	return *this;
}

__forceinline Message& Message::operator >> (unsigned __int64& value)
{
	if (_front + 8 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(unsigned __int64*)(_pBuffer + _front);
	_front += sizeof(unsigned __int64);
	return *this;
}

__forceinline Message& Message::operator >> (__int64& value)
{
	if (_front + 8 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(__int64*)(_pBuffer + _front);
	_front += sizeof(__int64);
	return *this;
}

__forceinline Message& Message::operator >> (float& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(float*)(_pBuffer + _front);
	_front += sizeof(float);
	return *this;
}

__forceinline Message& Message::operator >> (double& value)
{
	if (_front + 8 > _rear)
		throw MESSAGE_EXCEPTION;

	value = *(double*)(_pBuffer + _front);
	_front += sizeof(double);
	return *this;
}

#endif