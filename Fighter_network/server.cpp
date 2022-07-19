#define PROFILE

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <time.h>
#include <unordered_map>
#include <vector>
#include <conio.h>
#include "RingBuffer.h"
#include "Message.h"
#include "MemoryPool.h"
#include "Profile.h"
#include "ProfileInit.h"
#include "server.h"
#include "CrashDump.h"

using namespace std;

bool g_shutdown = FALSE;
int g_logLevel = LOG_LEVEL_SYSTEM;
WCHAR g_logBuff[1024];
WCHAR g_fileName[128];

// 네트워크
SOCKET g_listenSock;
unordered_map<SOCKET, Session*> g_sessionMap;
MemoryPool<Session> g_sessionPool(0, true);

// 컨텐츠
unordered_map<DWORD, Character*> g_characterMap;
list<Character*> g_sector[SECTOR_MAX_Y][SECTOR_MAX_X];
MemoryPool<Character> g_characterPool(0, true);

// 모니터용 변수
int g_mainLoop = 0;		// 초당 전체 루프 횟수
int g_updateCnt = 0;	// 초당 업데이트 횟수
int g_syncCnt = 0;		// 초당 Sync 발생 횟수
int g_sendCnt = 0;		// 초당 Send Message 개수

int g_networkTime = 0;
int g_selectTime = 0;
int g_sendTime = 0;
int g_recvTime = 0;
int g_acceptTime = 0;
int g_logicTime = 0;

int g_maxFrameTime = 0;
int g_minFrameTime = 100000;
int g_totalFrameTime = 0;
int g_avgFrameTime = 0;

int wmain()
{
	// 로그용 파일
	tm TM;
	time_t timer;

	time(&timer);
	localtime_s(&TM, &timer);
	swprintf_s(g_fileName, 128, L"Server_Log_%04d%02d%02d_%02d%02d%02d.txt",
		TM.tm_year + 1900,
		TM.tm_mon + 1,
		TM.tm_mday,
		TM.tm_hour, TM.tm_min, TM.tm_sec);

	netStartUp();

	while (!g_shutdown)
	{
		g_mainLoop++;

		netProcess();
		update();

		serverControl();

		monitor();
	}

	netCleanUp();
	return 0;
}

void netStartUp()
{
	timeBeginPeriod(1);

	int wsaRet;
	int bindRet;
	int listenRet;
	int ioctlRet;

	// 윈속 초기화
	WSADATA wsa;
	wsaRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (wsaRet)
	{
		_LOG(LOG_LEVEL_ERROR, L"WSAStartup Error : %d", wsaRet);
		throw NET_START_UP_ERROR;
	}
	_LOG(LOG_LEVEL_SYSTEM, L"WSAStartup #");

	// socket()
	g_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_listenSock == INVALID_SOCKET)
	{
		_LOG(LOG_LEVEL_ERROR, L"socket Error : %d", WSAGetLastError());
		throw NET_START_UP_ERROR;
	}

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(g_listenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	// 넌블로킹 소켓 전환
	u_long on = 1;
	ioctlRet = ioctlsocket(g_listenSock, FIONBIO, &on);
	if (ioctlRet == SOCKET_ERROR)
	{
		_LOG(LOG_LEVEL_ERROR, L"ioctlsocket error : %d", WSAGetLastError());
		throw NET_START_UP_ERROR;
	}

	// bind()
	SOCKADDR_IN serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVER_PORT);
	serveraddr.sin_addr.s_addr = htonl(ADDR_ANY);
	bindRet = bind(g_listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		_LOG(LOG_LEVEL_ERROR, L"bind Error : %d", WSAGetLastError());
		throw NET_START_UP_ERROR;
	}
	_LOG(LOG_LEVEL_SYSTEM, L"Bind OK # Port:%d", SERVER_PORT);

	// listen()
	listenRet = listen(g_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		_LOG(LOG_LEVEL_ERROR, L"listen Error : %d", WSAGetLastError());
		throw NET_START_UP_ERROR;
	}
	_LOG(LOG_LEVEL_SYSTEM, L"Listen OK #");
}

void netCleanUp()
{
	closesocket(g_listenSock);
	WSACleanup();
}

void serverControl()
{
	static bool controlMode = false;

	if (_kbhit())
	{
		WCHAR controlKey = _getwch();

		if (controlKey == L'u' || controlKey == L'U')
		{
			controlMode = true;

			wprintf(L"Control Mode : Press Q - Quit \n");
			wprintf(L"Control Mode : Press L - Key Lock \n");
			wprintf(L"Control Mode : Press C - Change Log Level \n");
		}

		if ((controlKey == L'l' || controlKey == L'L') && controlMode)
		{
			controlMode = false;
			
			wprintf(L"Control Lock...! Press U - Control Unlock\n");
		}

		if ((controlKey == L'q' || controlKey == L'Q') && controlMode)
		{
			g_shutdown = true;
		}

		if ((controlKey == L'c' || controlKey == L'C') && controlMode)
		{
			wprintf(L"Change Log Level\n");
			wprintf(L"\tPress D - DEBUG \n");
			wprintf(L"\tPress S - SYSTEM \n");
			wprintf(L"\tPress E - ERROR \n");

			WCHAR logLevel = _getwch();
			if (logLevel == L'd' || logLevel == L'D')
			{
				g_logLevel = LOG_LEVEL_DEBUG;
				wprintf(L"\tLog Level : DEBUG... \n");
			}
			else if (logLevel == L's' || logLevel == L'S')
			{
				g_logLevel = LOG_LEVEL_SYSTEM;
				wprintf(L"\tLog Level : SYSTEM... \n");
			}
			else if (logLevel == L'e' || logLevel == L'E')
			{
				g_logLevel = LOG_LEVEL_ERROR;
				wprintf(L"\tLog Level : ERROR... \n");
			}
		}

		// 기타 필요한 처리 추가
	}

}

void monitor()
{
	static int oldTime = timeGetTime();
	int curTime = timeGetTime();
	if (curTime - oldTime < 1000) return;
	oldTime = curTime;

	time_t timer;
	tm t;
	time(&timer);
	localtime_s(&t, &timer);

	// 프레임이 떨어지면 로그 남기기
	if (g_updateCnt < 25)
	{
		_LOG(LOG_LEVEL_SYSTEM, L"FPS Drop : %d", g_updateCnt);
	}

	g_avgFrameTime = g_totalFrameTime / g_updateCnt;
	
	// [06/03/22 16:11:49]
	wprintf(L"[%d/%d/%d %d:%d:%d]\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	wprintf(L"------------------------------------------------------------\n");
	wprintf(L"Main Loop/sec : %d\n", g_mainLoop);
	wprintf(L"------------------------------------------------------------\n");
	wprintf(L"FPS : %d\n", g_updateCnt);
	wprintf(L"Average Frame Time : %d ms\n", g_avgFrameTime);
	wprintf(L"Max Frame Time : %d ms\n", g_maxFrameTime);
	wprintf(L"Min Frame Time : %d ms\n", g_minFrameTime);
	wprintf(L"------------------------------------------------------------\n");
	wprintf(L"Session Count : %zd\n", g_sessionMap.size());
	wprintf(L"Character Count : %zd\n", g_characterMap.size());
	wprintf(L"------------------------------------------------------------\n");
	wprintf(L"Message Count/sec : %d\n", g_sendCnt);
	wprintf(L"Sync Message Count/sec : %d\n", g_syncCnt);
	wprintf(L"------------------------------------------------------------\n");
	wprintf(L"Network Time : %d ms\n", g_networkTime);
	wprintf(L"Select Time : %d ms\n", g_selectTime);
	wprintf(L"Accept Time : %d ms\n", g_acceptTime);
	wprintf(L"Send Time : %d ms\n", g_sendTime);
	wprintf(L"Recv Time : %d ms\n", g_recvTime);
	wprintf(L"Logic Time : %d ms\n", g_logicTime);
	wprintf(L"------------------------------------------------------------\n\n\n\n\n\n");

	g_networkTime = 0;
	g_selectTime = 0;
	g_sendTime = 0;
	g_recvTime = 0;
	g_acceptTime = 0;
	g_logicTime = 0;

	g_sendCnt = 0;
	g_mainLoop = 0;
	g_updateCnt = 0;
	g_syncCnt = 0;

	g_totalFrameTime = 0;
	g_maxFrameTime = 0;
	g_minFrameTime = 100000;
}

void log(WCHAR* str, int logLevel)
{
	//wprintf(L"%s\n", str);
	FILE* pFile;
	time_t timer;
	tm t;

	time(&timer);
	localtime_s(&t, &timer);

	_wfopen_s(&pFile, g_fileName, L"at");
	fwprintf_s(pFile, L"[%d/%02d/%02d %02d:%02d:%02d] %s\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, str);
	fclose(pFile);
}

// ---------------------------------------------
// 섹터 관련 함수
// ---------------------------------------------
void sectorAddCharacter(Character* pCharacter)
{
	int x;
	int y;

	x = pCharacter->x / SECTOR_WIDTH;
	y = pCharacter->y / SECTOR_HEIGHT;
	pCharacter->curSector.pos.x = x;
	pCharacter->curSector.pos.y = y;
	g_sector[y][x].push_back(pCharacter);
}

void sectorRemoveCharacter(Character* pCharacter)
{
	int x;
	int y;

	x = pCharacter->curSector.pos.x;
	y = pCharacter->curSector.pos.y;
	list<Character*>::iterator iter;
	for (iter = g_sector[y][x].begin(); iter != g_sector[y][x].end(); ++iter)
	{
		if ((*iter)->sessionID == pCharacter->sessionID)
		{
			g_sector[y][x].erase(iter);
			break;
		}
	}
}

bool sectorUpdateCharacter(Character* pCharacter)
{
	int x;
	int y;

	x = pCharacter->x / SECTOR_WIDTH;
	y = pCharacter->y / SECTOR_HEIGHT;
	if (pCharacter->curSector.pos.x == x && pCharacter->curSector.pos.y == y)
		return false;

	pCharacter->oldSector = pCharacter->curSector;
	sectorRemoveCharacter(pCharacter);
	getSectorAround(x, y, &pCharacter->curSector);
	sectorAddCharacter(pCharacter);
	return true;
}

void getSectorAround(int sectorX, int sectorY, SECTOR_AROUND* pSectorAround)
{
	pSectorAround->pos.x = sectorX;
	pSectorAround->pos.y = sectorY;
	sectorX--;
	sectorY--;

	pSectorAround->count = 0;
	for (int cntY = 0; cntY < 3; cntY++)
	{
		if (sectorY + cntY < 0 || sectorY + cntY >= SECTOR_MAX_Y)
			continue;
		for (int cntX = 0; cntX < 3; cntX++)
		{
			if (sectorX + cntX < 0 || sectorX + cntX >= SECTOR_MAX_X)
				continue;

			pSectorAround->around[pSectorAround->count].x = sectorX + cntX;
			pSectorAround->around[pSectorAround->count].y = sectorY + cntY;
			pSectorAround->count++;
		}
	}
}

void getUpdateSectorAround(Character* pCharacter, SECTOR_AROUND* pRemoveSector, SECTOR_AROUND* pAddSector)
{
	SECTOR_AROUND* curSectorAround = &pCharacter->curSector;
	SECTOR_AROUND* oldSectorAround = &pCharacter->oldSector;
	bool bFind;

	pRemoveSector->count = 0;
	pAddSector->count = 0;

	for (int cntOld = 0; cntOld < oldSectorAround->count; cntOld++)
	{
		bFind = false;
		for (int cntCur = 0; cntCur < curSectorAround->count; cntCur++)
		{
			if (oldSectorAround->around[cntOld].x == curSectorAround->around[cntCur].x &&
				oldSectorAround->around[cntOld].y == curSectorAround->around[cntCur].y)
			{
				bFind = true;
				break;
			}
		}

		if (!bFind)
		{
			pRemoveSector->around[pRemoveSector->count++] = oldSectorAround->around[cntOld];
		}
	}

	for (int cntCur = 0; cntCur < curSectorAround->count; cntCur++)
	{
		bFind = false;
		for (int cntOld = 0; cntOld < oldSectorAround->count; cntOld++)
		{
			if (curSectorAround->around[cntCur].x == oldSectorAround->around[cntOld].x &&
				curSectorAround->around[cntCur].y == oldSectorAround->around[cntOld].y)
			{
				bFind = true;
				break;
			}
		}

		if (!bFind)
		{
			pAddSector->around[pAddSector->count++] = curSectorAround->around[cntCur];
		}
	}
}

void characterSectorUpdatePacket(Character* pCharacter)
{
	SECTOR_AROUND removeSector, addSector;
	Character* pExistCharacter;

	list<Character*>* pSectorList;
	list<Character*>::iterator iter;
	Message message;
	
	getUpdateSectorAround(pCharacter, &removeSector, &addSector);

	// RemoveSector에 캐릭터 삭제 메시지 보내기
	mpDelete(&message, pCharacter->sessionID);
	for (int cnt = 0; cnt < removeSector.count; cnt++)
	{
		sendMessageSectorOne(removeSector.around[cnt].x, removeSector.around[cnt].y, &message, nullptr);
	}

	// 해당 플레이어에게 RemoveSector에 있는 캐릭터들 삭제 메시지 보내기
	for (int cnt = 0; cnt < removeSector.count; cnt++)
	{
		pSectorList = &g_sector[removeSector.around[cnt].y][removeSector.around[cnt].x];
		for (iter = pSectorList->begin(); iter != pSectorList->end(); ++iter)
		{
			mpDelete(&message, (*iter)->sessionID);
			sendMessageUnicast(pCharacter->pSession, &message);
		}
	}

	// AddSector에 캐릭터 생성 메시지 보내기
	mpOtherCharcter(&message, pCharacter->sessionID, pCharacter->direction, pCharacter->hp, pCharacter->x, pCharacter->y);
	for (int cnt = 0; cnt < addSector.count; cnt++)
	{
		sendMessageSectorOne(addSector.around[cnt].x, addSector.around[cnt].y, &message, nullptr);
	}

	// AddSector에 캐릭터 움직임 메시지 보내기
	mpMoveStart(&message, pCharacter->sessionID, pCharacter->action, pCharacter->x, pCharacter->y);
	for (int cnt = 0; cnt < addSector.count; cnt++)
	{
		sendMessageSectorOne(addSector.around[cnt].x, addSector.around[cnt].y, &message, nullptr);
	}

	// 해당 플레이어에게 AddSector에 있는 캐릭터들 생성 메시지 보내기
	for (int cnt = 0; cnt < addSector.count; cnt++)
	{
		pSectorList = &g_sector[addSector.around[cnt].y][addSector.around[cnt].x];
		for (iter = pSectorList->begin(); iter != pSectorList->end(); ++iter)
		{
			pExistCharacter = *iter;
			mpOtherCharcter(&message, pExistCharacter->sessionID, pExistCharacter->direction, pExistCharacter->hp, pExistCharacter->x, pExistCharacter->y);
			sendMessageUnicast(pCharacter->pSession, &message);

			if (pExistCharacter->action != STOP)
			{
				mpMoveStart(&message, pExistCharacter->sessionID, pExistCharacter->action, pExistCharacter->x, pExistCharacter->y);
				sendMessageUnicast(pCharacter->pSession, &message);
			}
		}
	}

}

// ---------------------------------------------
// 컨텐츠 관련 함수
// ---------------------------------------------
void update()
{
	static DWORD prePrameTime = timeGetTime();
    DWORD curPrameTime = timeGetTime();
	int frameTime = curPrameTime - prePrameTime;
	if (frameTime < SLEEP_TIME) return;
	prePrameTime = curPrameTime;

	// 모니터 링 데이터
	if (g_maxFrameTime < frameTime)
		g_maxFrameTime = frameTime;
	else if (g_minFrameTime > frameTime)
		g_minFrameTime = frameTime;
	g_totalFrameTime += frameTime;
	g_updateCnt++;

	int curLogicTime = timeGetTime();

	int nx;
	int ny;

	unordered_map<DWORD, Character*>::iterator iter;
	for (iter = g_characterMap.begin(); iter != g_characterMap.end(); iter++)
	{
		Character* pCharacter = iter->second;

		if (pCharacter->hp <= 0)
		{
			pCharacter->flag = true;
			pCharacter->pSession->flag = true;
		}
		else
		{
			if (curPrameTime - pCharacter->pSession->lastRecvTime > NETWORK_PACKET_RECV_TIMEOUT)
			{
				_LOG(LOG_LEVEL_SYSTEM, L"TimeOut SessionID:%d", pCharacter->sessionID);
				pCharacter->flag = true;
				pCharacter->pSession->flag = true;
				continue;
			}

			nx = pCharacter->x;
			ny = pCharacter->y;
			switch (pCharacter->action)
			{
			case PACKET_MOVE_DIR_LL:
				nx -= FRAME_MOVE_X;
				break;
			case PACKET_MOVE_DIR_LU:
				nx -= FRAME_MOVE_X;
				ny -= FRAME_MOVE_Y;
				break;
			case PACKET_MOVE_DIR_UU:
				ny -= FRAME_MOVE_Y;
				break;
			case PACKET_MOVE_DIR_RU:
				nx += FRAME_MOVE_X;
				ny -= FRAME_MOVE_Y;
				break;
			case PACKET_MOVE_DIR_RR:
				nx += FRAME_MOVE_X;
				break;
			case PACKET_MOVE_DIR_RD:
				nx += FRAME_MOVE_X;
				ny += FRAME_MOVE_Y;
				break;
			case PACKET_MOVE_DIR_DD:
				ny += FRAME_MOVE_Y;
				break;
			case PACKET_MOVE_DIR_LD:
				nx -= FRAME_MOVE_X;
				ny += FRAME_MOVE_Y;
				break;
			}

			if (nx < RANGE_MOVE_LEFT || nx >= RANGE_MOVE_RIGHT || ny < RANGE_MOVE_TOP || ny >= RANGE_MOVE_BOTTOM) continue;
			pCharacter->x = nx;
			pCharacter->y = ny;

			// 캐릭터가 이동한 경우 섹터를 다시 구한다.
			if (pCharacter->action != STOP)
			{
				if (sectorUpdateCharacter(pCharacter))
				{
					characterSectorUpdatePacket(pCharacter);
				}
			}
		}
	}
	
	disconnect();

	g_logicTime += timeGetTime() - curLogicTime;
}

void disconnect()
{
	Message message;
	bool flag;

	unordered_map<DWORD, Character*>::iterator charIter;
	unordered_map<SOCKET, Session*>::iterator sessionIter;
	for (charIter = g_characterMap.begin(); charIter != g_characterMap.end();)
	{
		Character* pCharacter = charIter->second;
		++charIter;

		if (pCharacter->flag)
		{
			mpDelete(&message, pCharacter->pSession->sessionID);
			sendMeesageAround(pCharacter->pSession, &message, false);

			deleteCharacter(pCharacter->sessionID);
			sectorRemoveCharacter(pCharacter);
			flag = g_characterPool.Free(pCharacter);
			if (!flag)
			{
				_LOG(LOG_LEVEL_ERROR, L"Object Pool Not Free Error sessionID:%d", pCharacter->sessionID);
			}
		}
	}

	for (sessionIter = g_sessionMap.begin(); sessionIter != g_sessionMap.end();)
	{
		Session* pSession = sessionIter->second;
		++sessionIter;

		if (pSession->flag)
		{
			disconnectSession(pSession->sock);
			closesocket(pSession->sock);
			flag = g_sessionPool.Free(pSession);
			if (!flag)
			{
				_LOG(LOG_LEVEL_ERROR, L"Object Pool Not Free Error sessionID:%d", pSession->sessionID);
			}
		}
	}
}

Character* createCharacter(Session* pSession, CHAR direction, CHAR action, CHAR hp, short x, short y)
{
	//Character* pCharacter = new Character;
	Character* pCharacter = g_characterPool.Alloc();
	pCharacter->pSession = pSession;
	pCharacter->sessionID = pSession->sessionID;
	pCharacter->direction = direction;
	pCharacter->action = action;
	pCharacter->hp = hp;
	pCharacter->x = x;
	pCharacter->y = y;
	pCharacter->flag = FALSE;
	pCharacter->attack1Time = 0;
	pCharacter->attack2Time = 0;
	pCharacter->attack3Time = 0;

	g_characterMap.insert({ pSession->sessionID, pCharacter });
	return pCharacter;
}

Character* findCharacter(DWORD sessionID)
{
	return g_characterMap.find(sessionID)->second;
}

void deleteCharacter(DWORD sessionID)
{
	g_characterMap.erase(sessionID);
}

// ---------------------------------------------
// 세션 관련 함수
// ---------------------------------------------
Session* findSession(SOCKET socket)
{
	return g_sessionMap.find(socket)->second;
}

Session* createSession(SOCKET socket, SOCKADDR_IN addr)
{
	static DWORD sessionID = 1;

	//Session* session = new Session;
	Session* session = g_sessionPool.Alloc();
	session->sessionID = sessionID++;
	session->sock = socket;
	session->port = ntohs(addr.sin_port);
	session->flag = FALSE;
	session->recvQ.ClearBuffer();
	session->sendQ.ClearBuffer();
	InetNtop(AF_INET, &addr.sin_addr, session->ip, sizeof(session->ip));
	session->lastRecvTime = timeGetTime();

	g_sessionMap.insert({ socket, session });
	return session;
}

void disconnectSession(SOCKET socket)
{
	g_sessionMap.erase(socket);
}

void netProcess()
{
	int curNetworkTime = timeGetTime();

	Session* pSession;
	SOCKET userSocketTable[FD_SETSIZE] = { INVALID_SOCKET, };
	int socketCnt = 0;

	FD_SET readSet;
	FD_SET writeSet;
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	// 리스닝 소켓 등록
	FD_SET(g_listenSock, &readSet);
	userSocketTable[socketCnt++] = g_listenSock;

	unordered_map<SOCKET, Session*>::iterator sessionIter;
	for (sessionIter = g_sessionMap.begin(); sessionIter != g_sessionMap.end(); ++sessionIter)
	{
		pSession = sessionIter->second;

		// 유저 소켓 등록
		userSocketTable[socketCnt++] = pSession->sock;
		FD_SET(pSession->sock, &readSet);
		if (pSession->sendQ.GetUseSize())
			FD_SET(pSession->sock, &writeSet);

		// FD_SETSIZE에 도달하면 select 실행
		if (socketCnt >= FD_SETSIZE)
		{
			// select 실행
			netSelectSocket(userSocketTable, &readSet, &writeSet);

			// FD_SET 초기화
			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);

			// SOCKET 배열 초기화
			memset(userSocketTable, INVALID_SOCKET, sizeof(userSocketTable));

			// 리스닝 소켓 등록
			socketCnt = 0;
			FD_SET(g_listenSock, &readSet);
			userSocketTable[socketCnt++] = g_listenSock;
		}
	}

	if (socketCnt > 0)
	{
		netSelectSocket(userSocketTable, &readSet, &writeSet);
	}

	g_networkTime += timeGetTime() - curNetworkTime;
}

void netSelectSocket(SOCKET* userSocketTable, FD_SET* readSet, FD_SET* writeSet)
{
	Profile profile(L"select_socket");
	int curSelectTime = timeGetTime();

	int count;
	int error;

	TIMEVAL time;
	time.tv_sec = 0;
	time.tv_usec = 0;

	count = select(0, readSet, writeSet, NULL, &time);
	if (count == SOCKET_ERROR)
	{
		error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			_LOG(LOG_LEVEL_ERROR, L"select error : %d", WSAGetLastError());
			shutdown();
		}
	}

	if (count > 0)
	{
		for (int i = 0; i < FD_SETSIZE; i++)
		{
			if (userSocketTable[i] == INVALID_SOCKET) break;

			if (FD_ISSET(userSocketTable[i], writeSet))
			{
				Profile p1(L"send_proc");
				netSendProc(userSocketTable[i]);
				count--;
			}

			if (FD_ISSET(userSocketTable[i], readSet))
			{
				count--;
				if (userSocketTable[i] == g_listenSock)
				{
					Profile p2(L"accept_proc");
					netAcceptProc();
				}
				else
				{
					Profile p3(L"recv_proc");
					netRecvProc(userSocketTable[i]);
				}
			}

			if (count <= 0) break;
		}
	}

	g_selectTime += timeGetTime() - curSelectTime;
}

void netAcceptProc()
{
	int curAcceptTime = timeGetTime();

	SOCKET sock;
	SOCKADDR_IN clientaddr;
	Message message;
	list<Character*>* pSectorList;
	list<Character*>::iterator iter;
	int size = sizeof(clientaddr);

	sock = accept(g_listenSock, (SOCKADDR*)&clientaddr, &size);
	if (sock == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			_LOG(LOG_LEVEL_ERROR, L"accept error : %d", error);
		}
		return;
	}

	// Session 생성
	Session* pSession = createSession(sock, clientaddr);

	// Charecter 생성
	short x = rand() % (RANGE_MOVE_RIGHT - RANGE_MOVE_LEFT) + RANGE_MOVE_LEFT;
	short y = rand() % (RANGE_MOVE_BOTTOM - RANGE_MOVE_TOP) + RANGE_MOVE_TOP;
	Character* pCharacter = createCharacter(pSession, PACKET_MOVE_DIR_LL, STOP, 100, x, y);
	sectorAddCharacter(pCharacter);
	getSectorAround(pCharacter->curSector.pos.x, pCharacter->curSector.pos.y, &pCharacter->curSector);
	_LOG(LOG_LEVEL_DEBUG, L"Connect # IP:%s:%d / SessionID:%d", pSession->ip, pSession->port, pSession->sessionID);
	_LOG(LOG_LEVEL_DEBUG, L"# PACKET_CONNECT # SessionID:%d # SectorX:%d SectorY:%d", pCharacter->sessionID, pCharacter->curSector.pos.x, pCharacter->curSector.pos.y);

	// 신규 유저에게 캐릭터 생성 메시지 전달
	mpCharcter(&message, pCharacter->sessionID, pCharacter->direction, pCharacter->hp, pCharacter->x, pCharacter->y);
	sendMessageUnicast(pSession, &message);

	// 색터내에 유저에게 신규 캐릭터 생성 메시지 전달
	mpOtherCharcter(&message, pCharacter->sessionID, pCharacter->direction, pCharacter->hp, pCharacter->x, pCharacter->y);
	sendMeesageAround(pSession, &message, false);

	// 색터내에 기존 캐릭터 생성 메시지를 신규 유저에게 전달
	for (int cnt = 0; cnt < pCharacter->curSector.count; cnt++)
	{
		pSectorList = &g_sector[pCharacter->curSector.around[cnt].y][pCharacter->curSector.around[cnt].x];
		for (iter = pSectorList->begin(); iter != pSectorList->end(); ++iter)
		{
			Character* pExistCharacter = *iter;
			if (pExistCharacter->sessionID == pSession->sessionID) continue;

			mpOtherCharcter(&message, pExistCharacter->sessionID, pExistCharacter->direction, pExistCharacter->hp, pExistCharacter->x, pExistCharacter->y);
			sendMessageUnicast(pSession, &message);

			if (pExistCharacter->action != STOP)
			{
				mpMoveStart(&message, pExistCharacter->sessionID, pExistCharacter->action, pExistCharacter->x, pExistCharacter->y);
				sendMessageUnicast(pCharacter->pSession, &message);
			}
		}
	}

	g_acceptTime += timeGetTime() - curAcceptTime;
}

void netSendProc(SOCKET socket)
{
	int curSendTime = timeGetTime();

	int error;
	int sendRet;
	Session* pSession = findSession(socket);

	{
		Profile p(L"send_send");
		sendRet = send(socket, pSession->sendQ.GetFrontBufferPtr(), pSession->sendQ.DirectDequeueSize(), 0);
	}
	if (sendRet == SOCKET_ERROR)
	{
		error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			if(error != 10054)
				_LOG(LOG_LEVEL_ERROR, L"send error : %d", error);

			Character* pCharacter = findCharacter(pSession->sessionID);
			pSession->flag = TRUE;
			pCharacter->flag = TRUE;
		}
		return;
	}
	pSession->sendQ.MoveFront(sendRet);

	g_sendTime += timeGetTime() - curSendTime;
}

void netRecvProc(SOCKET socket)
{ 
	int curRecvTime = timeGetTime();

	Session* pSession;
	int error;
	int recvRet;
	int completeRet;

	pSession = findSession(socket);
	pSession->lastRecvTime = timeGetTime();

	recvRet = recv(socket, pSession->recvQ.GetRearBufferPtr(), pSession->recvQ.DirectEnqueueSize(), 0);
	if (recvRet == SOCKET_ERROR || recvRet == 0)
	{
		error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			if (error != 0 && error != 10054)
				_LOG(LOG_LEVEL_ERROR, L"recv error : %d", error);

			Character* pCharacter = findCharacter(pSession->sessionID);
			pSession->flag = TRUE;
			pCharacter->flag = TRUE;
		}
		return;
	}

	if (recvRet > 0)
	{
		pSession->recvQ.MoveRear(recvRet);

		while (1)
		{
			completeRet = completeRecvMessage(pSession);

			if (completeRet == 1)
				break;

			if (completeRet == -1)
			{
				_LOG(LOG_LEVEL_ERROR, L"recv Error SessionID:%d", pSession->sessionID);

				Character* pCharacter = findCharacter(pSession->sessionID);
				pSession->flag = TRUE;
				pCharacter->flag = TRUE;
				return;
			}
		}
	}

	g_recvTime += timeGetTime() - curRecvTime;
}

int completeRecvMessage(Session* pSession)
{
	Message message;
	HEADER header;
	int peekRet;
	int deqRet;

	if (pSession->recvQ.GetUseSize() < sizeof(HEADER))
		return 1;

	peekRet = pSession->recvQ.Peek((char*)&header, sizeof(header));
	if (peekRet == FALSE)
		return -1;

	if (header.code != PACKET_CODE)
		return -1;

	if (pSession->recvQ.GetUseSize() < sizeof(header) + header.size)
		return 1;

	pSession->recvQ.MoveFront(sizeof(header));
	deqRet = pSession->recvQ.Dequeue(message.getBufferPtr(), header.size);
	if (deqRet == FALSE)
		return -1;

	message.moveWritePos(header.size);

	if (!messageProc(pSession, header.type, &message))
		return -1;

	return 0;
}

void shutdown()
{
	int* p = nullptr;
	*p = 0;
}

// ---------------------------------------------
// 메시지 프로시저
// ---------------------------------------------
bool messageProc(Session* pSession, BYTE type, Message* message)
{
	switch (type)
	{
	case PACKET_CS_MOVE_START:
		moveStart(pSession, message);
		break;
	case PACKET_CS_MOVE_STOP:
		moveStop(pSession, message);
		break;
	case PACKET_CS_ATTACK1:
		attack1(pSession, message);
		break;
	case PACKET_CS_ATTACK2:
		attack2(pSession, message);
		break;
	case PACKET_CS_ATTACK3:
		attack3(pSession, message);
		break;
	case PACKET_CS_ECHO:
		echo(pSession, message);
		break;
	default:
		_LOG(LOG_LEVEL_ERROR, L"Message Type Error - Type:%d, SessionID:%d", type, pSession->sessionID);
		return FALSE;
	}

	return TRUE;
}

void moveStart(Session* pSession, Message* message)
{
	char direction;
	short x;
	short y;
	Character* pCharacter;

	pCharacter = findCharacter(pSession->sessionID);
	*message >> direction;
	*message >> x;
	*message >> y;

	if (abs(pCharacter->x - x) > ERROR_RANGE || abs(pCharacter->y - y) > ERROR_RANGE)
	{
		g_syncCnt++;
		mpSync(message, pCharacter->sessionID, pCharacter->x, pCharacter->y);
		sendMessageUnicast(pSession, message);

		x = pCharacter->x;
		y = pCharacter->y;
		_LOG(LOG_LEVEL_ERROR, L"Sync Error SessionID:%d", pSession->sessionID);
	}

	switch (direction)
	{
	case PACKET_MOVE_DIR_LL:
	case PACKET_MOVE_DIR_LU:
	case PACKET_MOVE_DIR_LD:
		pCharacter->direction = PACKET_MOVE_DIR_LL;
		break;
	case PACKET_MOVE_DIR_RR:
	case PACKET_MOVE_DIR_RU:
	case PACKET_MOVE_DIR_RD:
		pCharacter->direction = PACKET_MOVE_DIR_RR;
		break;
	}

	pCharacter->action = direction;
	pCharacter->x = x;
	pCharacter->y = y;

	if (sectorUpdateCharacter(pCharacter))
	{
		characterSectorUpdatePacket(pCharacter);
	}

	mpMoveStart(message, pCharacter->sessionID, direction, x, y);
	sendMeesageAround(pSession, message);
}

void moveStop(Session* pSession, Message* message)
{
	char direction;
	short x;
	short y;
	Character* pCharacter;

	pCharacter = findCharacter(pSession->sessionID);
	*message >> direction;
	*message >> x;
	*message >> y;

	if (abs(pCharacter->x - x) > ERROR_RANGE || abs(pCharacter->y - y) > ERROR_RANGE)
	{
		g_syncCnt++;
		mpSync(message, pCharacter->sessionID, pCharacter->x, pCharacter->y);
		sendMessageUnicast(pSession, message);

		x = pCharacter->x;
		y = pCharacter->y;
		_LOG(LOG_LEVEL_ERROR, L"Sync Error SessionID:%d", pSession->sessionID);
	}

	pCharacter->action = STOP;
	pCharacter->direction = direction;
	pCharacter->x = x;
	pCharacter->y = y;

	if (sectorUpdateCharacter(pCharacter))
	{
		characterSectorUpdatePacket(pCharacter);
	}

	mpMoveStop(message, pCharacter->sessionID, direction, x, y);
	sendMeesageAround(pSession, message);
}

void attack1(Session* pSession, Message* message)
{
	DWORD curTime = timeGetTime();
	Character* pCharacter;
	char direction;
	short x;
	short y;

	pCharacter = findCharacter(pSession->sessionID);
	if (pCharacter->attack1Time - curTime < ATTACK1_TIME) return;
	*message >> direction;
	*message >> x;
	*message >> y;

	pCharacter->direction = direction;
	pCharacter->x = x;
	pCharacter->y = y;
	pCharacter->attack1Time = curTime;

	mpAttack1(message, pCharacter->sessionID, direction, x, y);
	sendMeesageAround(pSession, message);

	// 충돌처리는 활성 섹터내에서만 처리하기
	for (int idx = 0; idx < pCharacter->curSector.count; ++idx)
	{
		SECTOR_POS pos = pCharacter->curSector.around[idx];
		list<Character*>::iterator iter;
		list<Character*>* list = &g_sector[pos.y][pos.x];

		for (iter = list->begin(); iter != list->end(); ++iter)
		{
			bool flag = FALSE;
			Character* otherCharacter = *iter;
			if (otherCharacter->sessionID == pCharacter->sessionID) continue;

			if (pCharacter->direction == PACKET_MOVE_DIR_LL)
			{
				if (otherCharacter->x <= pCharacter->x && otherCharacter->x >= pCharacter->x - ATTACK1_RANGE_X &&
					otherCharacter->y >= pCharacter->y - ATTACK1_RANGE_Y && otherCharacter->y <= pCharacter->y + ATTACK1_RANGE_Y)
					flag = TRUE;
			}
			else
			{
				if (otherCharacter->x >= pCharacter->x && otherCharacter->x <= pCharacter->x + ATTACK1_RANGE_X &&
					otherCharacter->y >= pCharacter->y - ATTACK1_RANGE_Y && otherCharacter->y <= pCharacter->y + ATTACK1_RANGE_Y)
					flag = TRUE;
			}

			if (flag)
			{
				otherCharacter->hp -= ATTACK1_DAMAGE;

				mpDamage(message, pCharacter->sessionID, otherCharacter->sessionID, otherCharacter->hp);
				sendMeesageAround(otherCharacter->pSession, message, true);
				break;
			}
		}
	}
}

void attack2(Session* pSession, Message* message)
{
	DWORD curTime = timeGetTime();
	Character* pCharacter;
	char direction;
	short x;
	short y;

	pCharacter = findCharacter(pSession->sessionID);
	if (pCharacter->attack2Time - curTime < ATTACK2_TIME) return;
	*message >> direction;
	*message >> x;
	*message >> y;

	pCharacter->direction = direction;
	pCharacter->x = x;
	pCharacter->y = y;
	pCharacter->attack2Time = curTime;

	mpAttack2(message, pCharacter->sessionID, direction, x, y);
	sendMeesageAround(pSession, message);

	// 충돌처리는 활성 섹터내에서만 처리하기
	for (int idx = 0; idx < pCharacter->curSector.count; ++idx)
	{
		SECTOR_POS pos = pCharacter->curSector.around[idx];
		list<Character*>::iterator iter;
		list<Character*>* list = &g_sector[pos.y][pos.x];

		for (iter = list->begin(); iter != list->end(); ++iter)
		{
			bool flag = FALSE;
			Character* otherCharacter = *iter;
			if (otherCharacter->sessionID == pCharacter->sessionID) continue;

			if (pCharacter->direction == PACKET_MOVE_DIR_LL)
			{
				if (otherCharacter->x <= pCharacter->x && otherCharacter->x >= pCharacter->x - ATTACK2_RANGE_X &&
					otherCharacter->y >= pCharacter->y - ATTACK2_RANGE_Y && otherCharacter->y <= pCharacter->y + ATTACK2_RANGE_Y)
					flag = TRUE;
			}
			else
			{
				if (otherCharacter->x >= pCharacter->x && otherCharacter->x <= pCharacter->x + ATTACK2_RANGE_X &&
					otherCharacter->y >= pCharacter->y - ATTACK2_RANGE_Y && otherCharacter->y <= pCharacter->y + ATTACK2_RANGE_Y)
					flag = TRUE;
			}

			if (flag)
			{
				otherCharacter->hp -= ATTACK2_DAMAGE;

				mpDamage(message, pCharacter->sessionID, otherCharacter->sessionID, otherCharacter->hp);
				sendMeesageAround(otherCharacter->pSession, message, true);
				break;
			}
		}
	}
}

void attack3(Session* pSession, Message* message)
{
	DWORD curTime = timeGetTime();
	Character* pCharacter;
	char direction;
	short x;
	short y;

	pCharacter = findCharacter(pSession->sessionID);
	if (pCharacter->attack3Time - curTime < ATTACK3_TIME) return;
	*message >> direction;
	*message >> x;
	*message >> y;

	pCharacter->direction = direction;
	pCharacter->x = x;
	pCharacter->y = y;
	pCharacter->attack3Time = curTime;

	mpAttack3(message, pCharacter->sessionID, direction, x, y);
	sendMeesageAround(pSession, message);

	// 충돌처리는 활성 섹터내에서만 처리하기
	for (int idx = 0; idx < pCharacter->curSector.count; ++idx)
	{
		SECTOR_POS pos = pCharacter->curSector.around[idx];
		list<Character*>::iterator iter;
		list<Character*>* list = &g_sector[pos.y][pos.x];

		for (iter = list->begin(); iter != list->end(); ++iter)
		{
			bool flag = FALSE;
			Character* otherCharacter = *iter;
			if (otherCharacter->sessionID == pCharacter->sessionID) continue;

			if (pCharacter->direction == PACKET_MOVE_DIR_LL)
			{
				if (otherCharacter->x <= pCharacter->x && otherCharacter->x >= pCharacter->x - ATTACK3_RANGE_X &&
					otherCharacter->y >= pCharacter->y - ATTACK3_RANGE_Y && otherCharacter->y <= pCharacter->y + ATTACK3_RANGE_Y)
					flag = TRUE;
			}
			else
			{
				if (otherCharacter->x >= pCharacter->x && otherCharacter->x <= pCharacter->x + ATTACK3_RANGE_X &&
					otherCharacter->y >= pCharacter->y - ATTACK3_RANGE_Y && otherCharacter->y <= pCharacter->y + ATTACK3_RANGE_Y)
					flag = TRUE;
			}

			if (flag)
			{
				otherCharacter->hp -= ATTACK3_DAMAGE;

				mpDamage(message, pCharacter->sessionID, otherCharacter->sessionID, otherCharacter->hp);
				sendMeesageAround(otherCharacter->pSession, message, true);
				break;
			}
		}
	}
}

void echo(Session* pSession, Message* message)
{
	mpEcho(message, pSession->lastRecvTime);
	sendMessageUnicast(pSession, message);
}


// ---------------------------------------------
// 메시지 생성 함수
// ---------------------------------------------
void mpCharcter(Message* message, int id, char direction, char hp, short x, short y)
{

	HEADER header;
	header.code = PACKET_CODE;
	header.size = CREATE_MY_CHARACTER_SIZE;
	header.type = PACKET_SC_CREATE_MY_CHARACTER;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << direction;
	*message << x;
	*message << y;
	*message << hp;

	_LOG(LOG_LEVEL_DEBUG, L"Create Character # SessionID:%d\tX:%d\tY:%d", id, x, y);
}

void mpOtherCharcter(Message* message, int id, char direction, char hp, short x, short y)
{

	HEADER header;
	header.code = PACKET_CODE;
	header.size = CREATE_OTHER_CHARACTER_SIZE;
	header.type = PACKET_SC_CREATE_OTHER_CHARACTER;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << direction;
	*message << x;
	*message << y;
	*message << hp;
}

void mpMoveStart(Message* message, int id, char direction, short x, short y)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = MOVE_START_SIZE;
	header.type = PACKET_SC_MOVE_START;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << direction;
	*message << x;
	*message << y;

	_LOG(LOG_LEVEL_DEBUG, L"# PACKET_MOVESTART # SessionId:%d / Direction:%d / X:%d / Y:%d", id, direction, x, y);
}

void mpMoveStop(Message* message, int id, char direction, short x, short y)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = MOVE_STOP_SIZE;
	header.type = PACKET_SC_MOVE_STOP;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << direction;
	*message << x;
	*message << y;

	_LOG(LOG_LEVEL_DEBUG, L"# PACKET_MOVESTOP # SessionId:%d / Direction:%d / X:%d / Y:%d", id, direction, x, y);
}

void mpAttack1(Message* message, int id, char direction, short x, short y)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = ATTACK1_SIZE;
	header.type = PACKET_SC_ATTACK1;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << direction;
	*message << x;
	*message << y;

	_LOG(LOG_LEVEL_DEBUG, L"# PACKET_ATTACK1 # SessionId:%d / Direction:%d / X:%d / Y:%d", id, direction, x, y);
}

void mpAttack2(Message* message, int id, char direction, short x, short y)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = ATTACK2_SIZE;
	header.type = PACKET_SC_ATTACK2;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << direction;
	*message << x;
	*message << y;

	_LOG(LOG_LEVEL_DEBUG, L"# PACKET_ATTACK2 # SessionId:%d / Direction:%d / X:%d / Y:%d", id, direction, x, y);
}

void mpAttack3(Message* message, int id, char direction, short x, short y)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = ATTACK3_SIZE;
	header.type = PACKET_SC_ATTACK3;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << direction;
	*message << x;
	*message << y;

	_LOG(LOG_LEVEL_DEBUG, L"# PACKET_ATTACK3 # SessionId:%d / Direction:%d / X:%d / Y:%d", id, direction, x, y);
}

void mpDamage(Message* message, int attackId, int damageId, char damageHp)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = DAMAGE_SIZE;
	header.type = PACKET_SC_DAMAGE;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << attackId;
	*message << damageId;
	*message << damageHp;
}

void mpDelete(Message* message, int id)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = DELETE_CHARACTER_SIZE;
	header.type = PACKET_SC_DELETE_CHARACTER;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;

	_LOG(LOG_LEVEL_DEBUG, L"Disconnect # SessionId:%d", id);
}

void mpSync(Message* message, int id, short x, short y)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = SYNC_SIZE;
	header.type = PACKET_SC_SYNC;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << id;
	*message << x;
	*message << y;

	_LOG(LOG_LEVEL_DEBUG, L"Sync # SessionId:%d", id);
}

void mpEcho(Message* message, int time)
{
	HEADER header;
	header.code = PACKET_CODE;
	header.size = ECHO_SIZE;
	header.type = PACKET_SC_ECHO;

	message->clear();
	message->putData((char*)&header, HEADER_SIZE);
	*message << time;
}

// ---------------------------------------------
// 메시지 저장 함수
// ---------------------------------------------

void sendMessageUnicast(Session* pSession, Message* message)
{
	bool retval;

	retval = pSession->sendQ.Enqueue(message->getBufferPtr(), message->getDataSize());
	if (retval == FALSE)
	{
		_LOG(LOG_LEVEL_SYSTEM, L"SendQueue Full SessionID:%d", pSession->sessionID);
		Character* pCharacter = findCharacter(pSession->sessionID);
		pSession->flag = true;
		pCharacter->flag = true;
	}
	message->moveReadPos(message->getDataSize());
	g_sendCnt++;
}

void sendMessageBroadcast(Session* pExceptSession, Message* message)
{
	bool retval;
	Session* pSession;

	unordered_map<SOCKET, Session*>::iterator sessionIter;
	for (sessionIter = g_sessionMap.begin(); sessionIter != g_sessionMap.end(); ++sessionIter)
	{
		pSession = sessionIter->second;
		if (pSession->sessionID == pExceptSession->sessionID) continue;

		retval = pSession->sendQ.Enqueue(message->getBufferPtr(), message->getDataSize());
		if (retval == FALSE)
		{
			_LOG(LOG_LEVEL_SYSTEM, L"SendQueue Full SessionID:%d", pSession->sessionID);
			Character* pCharacter = findCharacter(pSession->sessionID);
			pSession->flag = true;
			pCharacter->flag = true;
		}
	}
	message->moveReadPos(message->getDataSize());
	g_sendCnt += g_sessionMap.size();
}

void sendMessageSectorOne(int sectorX, int sectorY, Message* message, Session* pExceptSession)
{
	bool retval;
	list<Character*>::iterator iter;
	list<Character*>* curSector = &g_sector[sectorY][sectorX];

	for (iter = curSector->begin(); iter != curSector->end(); ++iter)
	{
		Character* pCharacter = *iter;
		if (pExceptSession != nullptr && pCharacter->sessionID == pExceptSession->sessionID) 
			continue;

		g_sendCnt++;
		retval = pCharacter->pSession->sendQ.Enqueue(message->getBufferPtr(), message->getDataSize());
		if (retval == FALSE)
		{
			_LOG(LOG_LEVEL_SYSTEM, L"SendQueue Full SessionID:%d", pCharacter->sessionID);
			pCharacter->flag = true;
			pCharacter->pSession->flag = true;
		}
	}
	message->moveReadPos(message->getDataSize());
}

void sendMeesageAround(Session* pSession, Message* message, bool sendMe)
{
	bool retval;
	list<Character*>::iterator iter;
	list<Character*>* curSector;
	Character* pCharacter = findCharacter(pSession->sessionID);

	for (int sectorCnt = 0; sectorCnt < pCharacter->curSector.count; sectorCnt++)
	{
		SECTOR_POS sector = pCharacter->curSector.around[sectorCnt];
		curSector = &g_sector[sector.y][sector.x];
		for (iter = curSector->begin(); iter != curSector->end(); ++iter)
		{
			Character* pCharacter = *iter;
			if (!sendMe && pCharacter->sessionID == pSession->sessionID) continue;

			g_sendCnt++;
			retval = pCharacter->pSession->sendQ.Enqueue(message->getBufferPtr(), message->getDataSize());
			if (retval == FALSE)
			{
				_LOG(LOG_LEVEL_SYSTEM, L"SendQueue Full SessionID:%d", pCharacter->sessionID);
				pCharacter->flag = true;
				pCharacter->pSession->flag = true;
			}
		}
	}
	message->moveReadPos(message->getDataSize());
}