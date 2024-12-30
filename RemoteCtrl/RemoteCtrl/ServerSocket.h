#pragma once
#include <list>
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include "Mouse.h"
#define DEFAULT_PROT 20000

typedef void(*SOCKET_CALLBACK)(void*, int, std::list<CPacket>&, CPacket& inPacket);

class CServerSocket
{
public:
	// static function no this pointer, belongs to class. can access non static member.
	static CServerSocket* GetInstance();
	int Run(SOCKET_CALLBACK callback, void* arg, USHORT port = DEFAULT_PROT);
protected:
	BOOL InitSocket(USHORT port = DEFAULT_PROT);
	BOOL AcceptClient();
	BOOL Send(const char* pData, size_t nSize);
	BOOL Send(CPacket& packet);
	int DealCommand();
	void CloseClient();
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
	SOCKET_CALLBACK m_callback;
	void* m_arg;
};