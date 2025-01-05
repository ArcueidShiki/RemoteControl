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
// Linker->Manifest File->UAC Execution Level->requireAdministrator
// Computer\\HKEY_LOCAL_MACHINE\\SOFTWARE\Microsoft\\Windows\\CurrentVersion\\Run For boot startup
// The one and only application object

CWinApp theApp;

using namespace std;

void ChooseBootStartUp()
{
    CString warning = _T("This program is only allow legal usage!\n"
                         "This machine will be under survelience if continue using this program!\n"
                         "Click Cancel Button to exit, if you don't want to\n"
                         "Click Yes, this program will copy to your mahine, and startup when booting\n"
                         "Click No, this program will only run once, it won't left anything on your system");
    int ret = MessageBox(NULL, warning, _T("Warnning"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
        char sPath[MAX_PATH] = "";
		char sSysPath[MAX_PATH] = "";
		GetCurrentDirectoryA(MAX_PATH, sPath);
        GetSystemDirectoryA(sSysPath, MAX_PATH);
		strcat_s(sPath, "\\RemoteCtrl.exe");
        strcat_s(sSysPath, "\\RemoteControlServer.exe");
        char sLinkCmd[MAX_PATH * 2] = "MKLINK ";
        strcat_s(sLinkCmd, sSysPath);
		strcat_s(sLinkCmd, " ");
        strcat_s(sLinkCmd, sPath);
		TRACE("Link Command: [%s]\n", sLinkCmd);
        system(sLinkCmd);
        CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
        HKEY hKey = NULL;
		// KEY_ALL_ACCESS | KEY_WOW64_64KEY
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("Set startup when booting failed. Don't have enough permission?\r\n Program start failed!"),
                _T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
            exit(0);
        }
        // using REG_EXPAND_SZ auto expand %path%
		char sExePath[MAX_PATH] = "%SystemRoot%\\system32\\RemoteControlServer.exe";
        TRACE("sExePath Path: [%s]\n", sExePath);
        // RegSetValueExW(hKey, "RemoteCtrl", 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)sExePath, sExePath.GetLength() * sizeof(THAR))
		if (RegSetValueExA(hKey, "RemoteCtrl", 0, REG_EXPAND_SZ, (BYTE*)sExePath, (DWORD)strlen(sExePath)) != ERROR_SUCCESS)
		{
			MessageBox(NULL, _T("Set startup when booting failed. Don't have enough permission?\r\n Program start failed!"),
				_T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
		}
        RegCloseKey(hKey);
    }
    else if (ret == IDCANCEL)
    {
        exit(0);
    }
    return;
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
            ChooseBootStartUp();
            CCommand cmd;
            switch (CServerSocket::GetInstance()->Run(&CCommand::RunCommand, &cmd))
            {
                case -1:
                    MessageBox(NULL, _T("Cannot Initiate socket, please check network setting!"), _T("Soccket Initialization Error!"), MB_OK | MB_ICONERROR);
                    exit(0);
                    break;
			    case -2:
                    MessageBox(NULL, _T("Cannot accept user, failed many time, program exit"), _T("Accept Client Failed!"), MB_OK | MB_ICONERROR);
                    break;
            }
        }
    }
    else
    {
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

    return nRetCode;
}
