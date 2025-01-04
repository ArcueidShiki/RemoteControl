#pragma once
#pragma pack(push)
#pragma pack(1)

#define PACKET_HEAD 0xFEFF
#define CMD_DRIVER 1
#define CMD_DIR 2
#define CMD_RUN_FILE 3
#define CMD_DLD_FILE 4
#define CMD_DEL_FILE 5
#define CMD_MOUSE 6
#define CMD_SEND_SCREEN 7
#define CMD_LOCK_MACHINE 8
#define CMD_UNLOCK_MACHINE 9
#define CMD_ACK 0xFF

class CPacket
{
public:
	CPacket();
	CPacket(const CPacket& other);
	// parse packet
	CPacket(const BYTE* pData, size_t& nSize);
	// construct packet
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);
	CPacket& operator=(const CPacket& other);
	~CPacket() {}
	size_t Size() const;
	const char* GetData();
public:
	WORD sHead;	// FE FF
	DWORD nLength; // packet length
	WORD sCmd; // conctrol command
	std::string strData;
	WORD sSum; // check sum / crc
	std::string strOut;
};
#pragma pack(pop)

typedef struct file_info {
	file_info()
	{
		IsValid = TRUE;
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	file_info(BOOL valid, BOOL dir, BOOL next, const char* name = "")
	{
		IsValid = valid;
		IsDirectory = dir;
		HasNext = next;
		memset(szFileName, 0, sizeof(szFileName));
		memcpy(szFileName, name, strlen(name));
	}
	BOOL IsValid;
	BOOL IsDirectory;
	BOOL HasNext;
	char szFileName[256];
} FILEINFO, * PFILEINFO;