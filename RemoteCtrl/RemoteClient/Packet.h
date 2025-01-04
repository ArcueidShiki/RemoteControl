#pragma once
#include <string>
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

typedef struct PacketData{
	std::string strData;
 	UINT nMode;
	WPARAM wParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM wParam = 0)
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		this->wParam = wParam;
	}
	PacketData(const PacketData& other)
	{
		strData = other.strData;
		nMode = other.nMode;
		wParam = other.wParam;
	}
	PacketData& operator=(const PacketData& other)
	{
		if (this != &other)
		{ 
			strData = other.strData;
			nMode = other.nMode;
			wParam = other.wParam;
		}
		return *this;
	}
} PACKET_DATA;

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
	const char* GetData(std::string &strOut) const;
public:
	WORD sHead;	// FE FF
	DWORD nLength; // packet length
	WORD sCmd; // conctrol command
	std::string strData;
	WORD sSum; // check sum / crc
};
#pragma pack(pop)