// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Set Property->Linker->1. Entry point: mainCRTStartup, 2. SubSystem: Windows.

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

CServerSocket *pServer;

using namespace std;

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: code your application's behavior here.
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
        else
        {
#if 1
            CCommand cmd;
            // global variable, only one instance
            pServer = CServerSocket::GetInstance();
            int count = 0;
            while (pServer)
            {
                if (!pServer->AcceptClient())
                {
                    if (count > 3)
                    {
						MessageBox(NULL, _T("Cannot accept user, failed many time, program exit"), _T("Accept Client Failed!"), MB_OK | MB_ICONERROR);
						break;
                    }
                    MessageBox(NULL, _T("Cannot accept user, auto retry"), _T("Accept Client Failed!"), MB_OK | MB_ICONERROR);
                    count++;
                }
                //TRACE("New Client connection\n");
				int rCmd = pServer->DealCommand();
				if (rCmd == -1)
				{
					TRACE("Parse Command failed\n");
					break;
				}
                //TRACE("Parse Command : %d\n", cmd);
                int ret = cmd.ExecuteCommand(rCmd);
                if (ret == -1)
                {
					TRACE("Execute Command failed: cmd = %d, ret = %d\n", rCmd, ret);
                }
				//TRACE("Execute Command : %d, Success\n", cmd);
                //CPacket reply(ret, NULL, 0);
				//pServer->Send(reply); Really Need?
                // short connection. FTP usually use long connection
                pServer->CloseClient();
            }
#endif
        }
    }
    else
    {
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

    return nRetCode;
}
