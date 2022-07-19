#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MESSAGE_EXCEPTION 1001

class Message
{
protected:
	int _bufferSize;
	int _dataSize = 0;
	int _front = 0;
	int _rear = 0;
	char* _pBuffer;

public:
	enum MESSAGE { BUFFER_DEFAULT = 1400 };
	Message();
	Message(int bufferSize);
	virtual ~Message();

	void release();
	void clear() { _front = _rear = _dataSize = 0; }
	int getBufferSize() { return _bufferSize; }
	int getDataSize() { return _dataSize; }
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
	
	_pBuffer[_rear++] = value;
	_dataSize += 1;
	return *this;
}

__forceinline Message& Message::operator << (const char value)
{
	if (_rear + 1 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	_pBuffer[_rear++] = value;
	_dataSize += 1;
	return *this;
}

__forceinline Message& Message::operator << (const WCHAR value)
{
	if (_rear + 2 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(WCHAR), &value, sizeof(WCHAR));
	_rear += sizeof(WCHAR);
	_dataSize += 2;
	return *this;
}

__forceinline Message& Message::operator << (const short value)
{
	if (_rear + 2 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(short), &value, sizeof(short));
	_rear += sizeof(short);
	_dataSize += 2;
	return *this;
}

__forceinline Message& Message::operator << (const unsigned short value)
{
	if (_rear + 2 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(unsigned short), &value, sizeof(unsigned short));
	_rear += sizeof(unsigned short);
	_dataSize += 2;
	return *this;
}

__forceinline Message& Message::operator << (const int value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&_pBuffer[_rear], sizeof(int), &value, sizeof(int));
	_rear += sizeof(int);
	_dataSize += 4;
	return *this;
}

__forceinline Message& Message::operator << (const unsigned int value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&_pBuffer[_rear], sizeof(unsigned int), &value, sizeof(unsigned int));
	_rear += sizeof(unsigned int);
	_dataSize += 4;
	return *this;
}

__forceinline Message& Message::operator << (const long value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(long), &value, sizeof(long));
	_rear += sizeof(long);
	_dataSize += 4;
	return *this;
}

__forceinline Message& Message::operator << (const unsigned long value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&_pBuffer[_rear], sizeof(unsigned long), &value, sizeof(unsigned long));
	_rear += sizeof(unsigned long);
	_dataSize += 4;
	return *this;
}

__forceinline Message& Message::operator << (const __int64 value)
{
	if (_rear + 8 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(__int64), &value, sizeof(__int64));
	_rear += sizeof(__int64);
	_dataSize += 8;
	return *this;
}

__forceinline Message& Message::operator << (const unsigned __int64 value)
{
	if (_rear + 8 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(unsigned __int64), &value, sizeof(unsigned __int64));
	_rear += sizeof(unsigned __int64);
	_dataSize += 8;
	return *this;
}

__forceinline Message& Message::operator << (const float value)
{
	if (_rear + 4 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(float), &value, sizeof(float));
	_rear += sizeof(float);
	_dataSize += 4;
	return *this;
}

__forceinline Message& Message::operator << (const double value)
{
	if (_rear + 8 >= _bufferSize)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&_pBuffer[_rear], sizeof(double), &value, sizeof(double));
	_rear += sizeof(double);
	_dataSize += 8;
	return *this;
}


// 빼기
__forceinline Message& Message::operator >> (unsigned char& value)
{
	if (_front + 1 > _rear)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&value, sizeof(unsigned char), &_pBuffer[_front], sizeof(unsigned char));
	_front += sizeof(unsigned char);
	_dataSize -= 1;
	return *this;
}

__forceinline Message& Message::operator >> (char& value)
{
	if (_front + 1 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(char), &_pBuffer[_front], sizeof(char));
	_front += sizeof(char);
	_dataSize -= 1;
	return *this;
}

__forceinline Message& Message::operator >> (WCHAR& value)
{
	if (_front + 2 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(WCHAR), &_pBuffer[_front], sizeof(WCHAR));
	_front += sizeof(WCHAR);
	_dataSize -= 2;
	return *this;
}

__forceinline Message& Message::operator >> (unsigned short& value)
{
	if (_front + 2 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(unsigned short), &_pBuffer[_front], sizeof(unsigned short));
	_front += sizeof(unsigned short);
	_dataSize -= 2;
	return *this;
}

__forceinline Message& Message::operator >> (short& value)
{
	if (_front + 2 > _rear)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&value, sizeof(short), &_pBuffer[_front], sizeof(short));
	_front += sizeof(short);
	_dataSize -= 2;
	return *this;
}

__forceinline Message& Message::operator >> (unsigned int& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(unsigned int), &_pBuffer[_front], sizeof(unsigned int));
	_front += sizeof(unsigned int);
	_dataSize -= 4;
	return *this;
}

__forceinline Message& Message::operator >> (int& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(int), &_pBuffer[_front], sizeof(int));
	_front += sizeof(int);
	_dataSize -= 4;
	return *this;
}

__forceinline Message& Message::operator >> (unsigned long& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&value, sizeof(unsigned long), &_pBuffer[_front], sizeof(unsigned long));
	_front += sizeof(unsigned long);
	_dataSize -= 4;
	return *this;
}

__forceinline Message& Message::operator >> (long& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&value, sizeof(long), &_pBuffer[_front], sizeof(long));
	_front += sizeof(long);
	_dataSize -= 4;
	return *this;
}

__forceinline Message& Message::operator >> (unsigned __int64& value)
{
	if (_front + 8 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(unsigned __int64), &_pBuffer[_front], sizeof(unsigned __int64));
	_front += sizeof(unsigned __int64);
	_dataSize -= 8;
	return *this;
}

__forceinline Message& Message::operator >> (__int64& value)
{
	if (_front + 8 > _rear)
		throw MESSAGE_EXCEPTION;

	memcpy_s(&value, sizeof(__int64), &_pBuffer[_front], sizeof(__int64));
	_front += sizeof(__int64);
	_dataSize -= 8;
	return *this;
}

__forceinline Message& Message::operator >> (float& value)
{
	if (_front + 4 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(float), &_pBuffer[_front], sizeof(float));
	_front += sizeof(float);
	_dataSize -= 4;
	return *this;
}

__forceinline Message& Message::operator >> (double& value)
{
	if (_front + 8 > _rear)
		throw MESSAGE_EXCEPTION;
	
	memcpy_s(&value, sizeof(double), &_pBuffer[_front], sizeof(double));
	_front += sizeof(double);
	_dataSize -= 8;
	return *this;
}

#endif