// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "LockDialog.h"
#include "Utils.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if 0
#pragma comment(linker, "/subsystem:windwos /entry:WinMainCRTStartup")
#pragma comment(linker, "/subsystem:windwos /entry:mainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:WinMainCRTStartup")
#pragma comment(linker, "/subsystem:console /entry:mainCRTStartup")
#endif
// The one and only application object

CWinApp theApp;

using namespace std;

// Set Property->Linker->1. Entry point: mainCRTStartup, 2. SubSystem: Windows.
int main()
{
    if (!CUtils::IsAdmin())
    {
        return CUtils::RunAsAdmin() ? 0 : 1;
    }
    if (!CUtils::Init())
    {
        return 1;
    }
    if (!CUtils::ChooseBootStartUp())
    {
        return 1;
    }
    CCommand cmd;
    switch (CServerSocket::GetInstance()->Run(&CCommand::RunCommand, &cmd))
    {
        case -1:
            MessageBox(NULL, _T("Cannot Initiate socket, please check network setting!"), _T("Soccket Initialization Error!"), MB_OK | MB_ICONERROR);
            break;
		case -2:
            MessageBox(NULL, _T("Cannot accept user, failed many time, program exit"), _T("Accept Client Failed!"), MB_OK | MB_ICONERROR);
            break;
    }
    return 0;
}
