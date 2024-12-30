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
	int ExecuteCommand(int nCmd);
	int MakeDriverInfo();
	int MakeDirectoryInfo();
	int RunFile();
	int DownloadFile();
	int DelFile();
	int MouseEvent();
	int SendScreen();
	int LockMachine();
	int UnlockMachine();
	static unsigned __stdcall ThreadLockDlg(void *arg);
	void ThreadLockDlgMain();
protected:
	typedef int (CCommand::* CMDFUNC)(); // member function pointer
	std::map<int, CMDFUNC> m_mapCmd;
	CLockDialog dlg;
	unsigned tid;
	CServerSocket* pServer;
};

