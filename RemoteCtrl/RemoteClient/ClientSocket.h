#pragma once
#include <string>
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include <vector>

typedef struct MouseEvent {
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		point.x = 0;
		point.y = 0;
	}
	WORD nAction; // move, click, double click
	WORD nButton; // left, middle, right
	POINT point;
} MOUSEEV, * PMOUSEEV;

class CClientSocket
{
public:
	// static function no this pointer, belongs to class. can access non static member.
	static CClientSocket* GetInstance();
	BOOL InitSocket(ULONG ip, USHORT port);
	int DealCommand();
	BOOL Send(const char* pData, size_t nSize);
	BOOL Send(CPacket& packet);
	BOOL GetFilePath(std::string& strPath);
	BOOL GetMouseEvent(MOUSEEV& mouse);
	void CloseSocket();
	CPacket& GetPacket() { return m_packet; }
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
};