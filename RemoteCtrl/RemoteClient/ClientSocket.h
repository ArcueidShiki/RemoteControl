#pragma once
#include <string>
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include <vector>
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

class CClientSocket
{
public:
	// static function no this pointer, belongs to class. can access non static member.
	static CClientSocket* GetInstance();
	BOOL InitSocket();
	int DealCommand();
	BOOL Send(const char* pData, size_t nSize);
	BOOL Send(const CPacket& packet);
	BOOL GetFilePath(std::string& strPath);
	BOOL GetMouseEvent(MOUSEEV& mouse);
	void CloseSocket();
	CPacket& GetPacket() { return m_packet; }
	void UpdateAddress(ULONG nIp, USHORT nPort);
private:
	// Initialize before main
	// Singleton, private all constructors
	CClientSocket(const CClientSocket& other);
	CClientSocket& operator=(const CClientSocket& other);
	CClientSocket();
	~CClientSocket();
	BOOL InitSocketEnv();
	static void ReleaseInstance();
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	static CClientSocket* m_instance;
	static CHelper m_helper;
	SOCKET m_socket;
	CPacket m_packet;
	std::vector<char> m_buf;
	ULONG m_nIp;
	USHORT m_nPort;
};