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
	void Init();
	int ExecuteCommand(int nCmd, std::list<CPacket>& lstPackets, CPacket &inPacket);
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
	static unsigned __stdcall ThreadLockDlg(void *obj);
	static void RunCommand(void *obj, int nCmd, std::list<CPacket>& lstPackets, CPacket& inPacket);
protected:
	typedef int (CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); // member function pointer
	std::map<int, CMDFUNC> m_mapCmd;
	CLockDialog dlg;
	unsigned tid;
};

