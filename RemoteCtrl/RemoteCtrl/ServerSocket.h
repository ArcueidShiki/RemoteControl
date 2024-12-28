#pragma once
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include "Mouse.h"

typedef struct file_info {
	file_info()
	{
		IsValid = TRUE;
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsValid;
	BOOL IsDirectory;
	BOOL HasNext;
	char szFileName[256];
} FILEINFO, * PFILEINFO;

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
	BOOL GetMouseEvent(MOUSEEV &mouse);
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
};