// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Set Property->Linker->1. Entry point: mainCRTStartup, 2. SubSystem: Windows.

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>

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

void Dump(BYTE *pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        snprintf(buf, sizeof(buf), "%02X", pData[i]);
    }
}

std::string MakeDriverInfo()
{
    std::string result;
    // 1 == A, 2 == B, 3 == C, 26 == Z
    for (int i = 1; i <= 26; i++)
    {
        if (_chdrive(i) == 0)
        {
            if (result.size() > 0)
                result += ',';
            result += 'A' + i - 1;
        }
    }
    CServerSocket::GetInstance()->Send(CPacket(1, (BYTE *)result.c_str(), result.size()));
}

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
#if 0
            // global variable, only one instance
            CServerSocket* pServer = CServerSocket::GetInstance();
            int count = 0;
            while (pServer)
            {
                if (!pServer->AcceptClient())
                {
                    if (count > 3)
                    {
						MessageBox(NULL, _T("无法正常接入用户，自动重试次数过多，程序退出"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
						break;
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败!"), MB_OK | MB_ICONERROR);
                    count++;
                }

				int ret = pServer->DealCommand();
            }
#endif
            MakeDriverInfo();
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

    return nRetCode;
}
