#pragma once
#include "pch.h"
#include "framework.h"
#include "Packet.h"

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
	BOOL GetFilePath(std::string& strPath);
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