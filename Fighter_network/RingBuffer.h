#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

class RingBuffer
{
private:
	int _front = 0;
	int _rear = 0;
	int _bufferSize;
	char* _buffer;
public:
	RingBuffer(int bufferSize = 10000);
	~RingBuffer();

	//void Resize(int size);
	int GetBufferSize(void);
	int GetUseSize(void);
	int GetFreeSize(void);

	int DirectEnqueueSize(void);
	int DirectDequeueSize(void);

	bool Enqueue(char* pData, int size);
	bool Dequeue(char* pDest, int size);
	bool Peek(char* pDest, int size);
	void ClearBuffer(void);

	void MoveRear(int size);
	void MoveFront(int size);

	char* GetFrontBufferPtr(void);
	char* GetRearBufferPtr(void);
};

#endif