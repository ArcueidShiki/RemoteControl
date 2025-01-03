#pragma once
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <list>
#include <map>
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

class CClientSocket
{
public:
	// static function no this pointer, belongs to class. can access non static member.
	static CClientSocket* GetInstance();
	static void ThreadEntry(void *arg);
	void ThreadFunc();
	void MessageLoop();
	BOOL InitSocket();
	int DealCommand();
	BOOL GetFilePath(std::string& strPath);
	BOOL GetMouseEvent(MOUSEEV& mouse);
	void CloseSocket();
	CPacket& GetPacket() { return m_packet; }
	void UpdateAddress(ULONG nIp, USHORT nPort);
	BOOL SendPacket(const CPacket &packet, std::list<CPacket> *lstAcks, BOOL bAutoClose = TRUE);
private:
	// Initialize before main
	// Singleton, private all constructors
	CClientSocket(const CClientSocket& other);
	CClientSocket& operator=(const CClientSocket& other);
	CClientSocket();
	~CClientSocket();
	BOOL InitSocketEnv();
	BOOL Send(const char* pData, size_t nSize);
	BOOL Send(const CPacket& packet);
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
	BOOL m_isAutoClose;
	std::map<HANDLE, std::list<CPacket>*> m_mapAck;
	std::map<HANDLE, BOOL> m_mapAutoClose;
	std::queue<CPacket> m_queueSend; // not thread safe
	std::mutex m_mutex;
	HANDLE m_hThread;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapMsgHandlers;
	void SendPacket(UINT nMsg, WPARAM wParam, LPARAM lParam);
};