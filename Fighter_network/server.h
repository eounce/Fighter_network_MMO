#ifndef _SERVER_H_
#define _SERVER_H_

// ��Ʈ��ũ ����
#define SERVER_PORT 20000

// ȭ�� ũ��
#define MAP_WIDTH 6400
#define MAP_HEIGHT 6400

// ���� ũ��
/*
#define SECTOR_MAX_Y 100	// 200
#define SECTOR_MAX_X 100	// 256
#define SECTOR_HEIGHT 64
#define SECTOR_WIDTH 64
*/

#define SECTOR_MAX_Y 50	// 200
#define SECTOR_MAX_X 50	// 256
#define SECTOR_HEIGHT 128
#define SECTOR_WIDTH 128

/*
#define SECTOR_MAX_Y 25	// 200
#define SECTOR_MAX_X 20	// 256
#define SECTOR_HEIGHT 256
#define SECTOR_WIDTH 320
*/

// �̵� ���� ����
#define ERROR_RANGE		50

// Ÿ�� �ƿ� �ð�
#define NETWORK_PACKET_RECV_TIMEOUT	30000

// ȭ�� �̵�����
#define RANGE_MOVE_TOP	0
#define RANGE_MOVE_LEFT	0
#define RANGE_MOVE_RIGHT	6400
#define RANGE_MOVE_BOTTOM	6400

// �ּ� ���� ��� �ð�(ms)
#define ATTACK1_TIME 250
#define ATTACK2_TIME 330
#define ATTACK3_TIME 490

// ���� ����
#define ATTACK1_RANGE_X		80
#define ATTACK2_RANGE_X		90
#define ATTACK3_RANGE_X		100
#define ATTACK1_RANGE_Y		10
#define ATTACK2_RANGE_Y		10
#define ATTACK3_RANGE_Y		20

// �����Ӵ� �̵� ����
#define SLEEP_TIME 40  // 50 frame (20) 40
#define FRAME_MOVE_X 6  // 3 6
#define FRAME_MOVE_Y 4  // 2 4

// �������� ����
#define	PACKET_SC_CREATE_MY_CHARACTER			0
#define	PACKET_SC_CREATE_OTHER_CHARACTER		1
#define	PACKET_SC_DELETE_CHARACTER				2
#define	PACKET_SC_DAMAGE						30

#define	PACKET_CS_MOVE_START					10
#define	PACKET_SC_MOVE_START					11
#define	PACKET_CS_MOVE_STOP						12
#define	PACKET_SC_MOVE_STOP						13

#define PACKET_SC_SYNC							251
#define PACKET_CS_ECHO							252
#define PACKET_SC_ECHO							253

// �̵� ����
#define PACKET_MOVE_DIR_LL					0
#define PACKET_MOVE_DIR_LU					1
#define PACKET_MOVE_DIR_UU					2
#define PACKET_MOVE_DIR_RU					3
#define PACKET_MOVE_DIR_RR					4
#define PACKET_MOVE_DIR_RD					5
#define PACKET_MOVE_DIR_DD					6
#define PACKET_MOVE_DIR_LD					7

#define	PACKET_CS_ATTACK1						20
#define	PACKET_SC_ATTACK1						21
#define	PACKET_CS_ATTACK2						22
#define	PACKET_SC_ATTACK2						23
#define	PACKET_CS_ATTACK3						24
#define	PACKET_SC_ATTACK3						25

// �ٶ󺸴� ����
#define LEFT 0
#define RIGHT 1

// �׼�
#define STOP 8

// ������
#define ATTACK1_DAMAGE 1
#define ATTACK2_DAMAGE 2
#define ATTACK3_DAMAGE 3

#define PACKET_CODE 0x89

// �޽��� ũ��
#define HEADER_SIZE 3
#define CREATE_MY_CHARACTER_SIZE		10
#define	CREATE_OTHER_CHARACTER_SIZE		10
#define	DELETE_CHARACTER_SIZE			4
#define	DAMAGE_SIZE						9

#define	MOVE_START_SIZE					9
#define	MOVE_STOP_SIZE					9

#define	ATTACK1_SIZE					9
#define	ATTACK2_SIZE					9
#define	ATTACK3_SIZE					9

#define SYNC_SIZE						8
#define ECHO_SIZE						4

// ����
#define NET_START_UP_ERROR 10;

// �α� �ܰ�
#define LOG_LEVEL_DEBUG		0
#define LOG_LEVEL_SYSTEM	1
#define LOG_LEVEL_ERROR		2

#define _LOG(logLevel, fmt, ...)				\
do {											\
	if (g_logLevel <= logLevel)					\
	{											\
		wsprintf(g_logBuff, fmt, ##__VA_ARGS__);\
		log(g_logBuff, logLevel);				\
	}											\
} while(0)										\

#pragma pack(push, 1)
struct HEADER
{
	BYTE code; // 0x89 ����
	BYTE size;
	BYTE type;
};
#pragma pack(pop)

struct SECTOR_POS
{
	int x;
	int y;
};

struct SECTOR_AROUND
{
	int count;
	SECTOR_POS pos;
	SECTOR_POS around[9];
};

struct Session
{
	DWORD sessionID;		// ���� ����ID
	RingBuffer recvQ;		// ���� ť
	RingBuffer sendQ;		// �۽� ť
	SOCKET sock;			// TCP ����
	WCHAR ip[20];			// IP
	USHORT port;			// Port
	DWORD lastRecvTime;		// Ÿ�Ӿƿ� �� ���� üũ
	BOOL flag;
};

struct Character
{
	Session* pSession;
	DWORD sessionID;

	CHAR direction;
	CHAR action;

	SECTOR_AROUND curSector;
	SECTOR_AROUND oldSector;

	CHAR hp;
	short x;
	short y;

	DWORD attack1Time;
	DWORD attack2Time;
	DWORD attack3Time;
	BOOL flag;
};

// �α׿� �Լ�
void log(WCHAR* str, int logLevel);

void netStartUp();
void netCleanUp();
void serverControl();
void monitor();

// ���� ���� �Լ�
void netProcess();
void netSelectSocket(SOCKET* userSocketTable, FD_SET* readSet, FD_SET* writeSet);
void netAcceptProc();
void netSendProc(SOCKET socket);
void netRecvProc(SOCKET socket);
int completeRecvMessage(Session* pSession);

void disconnect();

Session* findSession(SOCKET socket);
Session* createSession(SOCKET socket, SOCKADDR_IN addr);
void disconnectSession(SOCKET socket);

// ������ ���� �Լ�
void update();

Character* createCharacter(Session* pSession, CHAR direction, CHAR action, CHAR hp, short x, short y);
Character* findCharacter(DWORD sessionID);
void deleteCharacter(DWORD sessionID);

// ���� ���� �Լ�
void sectorAddCharacter(Character* pCharacter);
void sectorRemoveCharacter(Character* pCharacter);
bool sectorUpdateCharacter(Character* pCharacter);
void getSectorAround(int sectorX, int sectorY, SECTOR_AROUND* pSectorAround);
void getUpdateSectorAround(Character* pCharacter, SECTOR_AROUND *pRemoveSector, SECTOR_AROUND* pAddSector);
void characterSectorUpdatePacket(Character* pCharater);

// �޽��� ���ν���
bool messageProc(Session* pSession, BYTE type, Message* message);
void moveStart(Session* pSession, Message* message);
void moveStop(Session* pSession, Message* message);
void attack1(Session* pSession, Message* message);
void attack2(Session* pSession, Message* message);
void attack3(Session* pSession, Message* message);
void echo(Session* pSession, Message* message);

// �޽��� ���� �Լ�
void mpCharcter(Message* message, int id, char direction, char hp, short x, short y);
void mpOtherCharcter(Message* message, int id, char direction, char hp, short x, short y);
void mpMoveStart(Message* message, int id, char direction, short x, short y);
void mpMoveStop(Message* message, int id, char direction, short x, short y);
void mpAttack1(Message* message, int id, char direction, short x, short y);
void mpAttack2(Message* message, int id, char direction, short x, short y);
void mpAttack3(Message* message, int id, char direction, short x, short y);
void mpDamage(Message* message, int attackId, int damageId, char damageHp);
void mpDelete(Message* message, int id);
void mpSync(Message* message, int id, short x, short y);
void mpEcho(Message* message, int time);

// �޽��� ���� �Լ�
void sendMessageUnicast(Session* pSession, Message* message);
void sendMessageBroadcast(Session* pExceptSession, Message* message);
void sendMessageSectorOne(int sectorX, int sectorY, Message* message, Session* pExceptSession);
void sendMeesageAround(Session* pSession, Message* message, bool sendMe = false);

// ���� ���� �Լ�
void shutdown();
#endif