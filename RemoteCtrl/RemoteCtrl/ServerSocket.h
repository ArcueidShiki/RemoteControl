#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)

#define PACKET_HEAD 0xFEFF

class CPacket
{
public:
	CPacket();
	CPacket(const CPacket& other);
	// parse packet
	CPacket(const BYTE* pData, size_t& nSize);
	// construct packet
	CPacket(DWORD nCmd, const BYTE* pData, size_t nSize);
	CPacket& operator=(const CPacket& other);
	~CPacket() {}
	size_t Size() const;
	const char* Data();
public:
	WORD sHead;	// FE FF
	DWORD nLength; // packet length
	WORD sCmd; // conctrol command
	std::string strData;
	WORD sSum; // check sum / crc
	std::string strOut;
};
#pragma pack(pop)

class CServerSocket
{
public:
	// static function no this pointer, belongs to class. can access non static member.
	static CServerSocket* GetInstance();
	BOOL InitSocket();
	BOOL AcceptClient();
	int DealCommand();
	BOOL Send(const char* pData, size_t nSize);
	BOOL Send(CPacket &packet);
private:
	// Initialize before main
	// Singleton, private all constructors
	CServerSocket(const CServerSocket& other);
	CServerSocket& operator=(const CServerSocket& other);
	CServerSocket();
	~CServerSocket();
	BOOL InitSocketEnv();
	static void ReleaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CServerSocket *m_instance;
	static CHelper m_helper;
	SOCKET m_socket;
	SOCKET m_client;
	CPacket m_packet;
};