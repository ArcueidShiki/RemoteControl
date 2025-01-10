#pragma once
#include <map>
#include <atlimage.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <list>
#include "ServerSocket.h"
#include "Utils.h"
#include "LockDialog.h"
#include "Resource.h"
#include "SendQueue.h"

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

class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExecuteCommand(int nCmd, std::list<CPacket>& lstPackets, CPacket &inPacket);
	static void RunCommand(void *obj, int nCmd, std::list<CPacket>& lstPackets, CPacket& inPacket);
protected:
	int MakeDriverInfo(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int MakeDirectoryInfo(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int RunFile(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int DownloadFile(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int DelFile(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int MouseEvent(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int SendScreen(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int LockMachine(std::list<CPacket>& lstPackets, CPacket& inPacket);
	int UnlockMachine(std::list<CPacket>& lstPackets, CPacket& inPacket);
	void ThreadLockDlgMain();
	static unsigned __stdcall ThreadLockDlg(void* obj);
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); // member function pointer
	std::map<int, CMDFUNC> m_mapCmd;
	CLockDialog dlg;
	unsigned tid;
};

class Command
{
public:
	Command();
	~Command();
	int ExecuteCommand(int nCmd, SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	static void RunCommand(void* obj, int nCmd, SendQueue<CPacket>& sendQueue, CPacket& inPacket);
protected:
	int MakeDriverInfo(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int MakeDirectoryInfo(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int RunFile(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int DownloadFile(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int DelFile(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int MouseEvent(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int SendScreen(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int LockMachine(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	int UnlockMachine(SendQueue<CPacket>& sendQueue, CPacket& inPacket);
	void ThreadLockDlgMain();
	static unsigned __stdcall ThreadLockDlg(void* obj);
	typedef int (Command::* CMD_CALLBACK)(SendQueue<CPacket>& sendQueue, CPacket& inPacket); // member function pointer
	std::map<int, CMD_CALLBACK> m_mapCmd;
	CLockDialog dlg;
	unsigned tid;
private:
	void MouseLClick();
	void MouseLDBClick();
	void MouseLDown();
	void MouseLUp();
	void MouseRClick();
	void MouseRDBClick();
	void MouseRDown();
	void MouseRUp();
	void MouseMClick();
	void MouseMDBClick();
	void MouseMDown();
	void MouseMUp();
	void MouseWheelUp();
	void MouseWheelDown();
	void MouseMove(MOUSEEV &mouse);
};

using CMD_CB = void(*)(void*, int, SendQueue<CPacket>& sendQueue, CPacket& inPacket);
using CMD_SPTR = std::shared_ptr<Command>;
