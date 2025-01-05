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

void MakeLink()
{
    // %Windows/System32%
    char sSysPath[MAX_PATH] = "";
    GetSystemDirectoryA(sSysPath, MAX_PATH);
    strcat_s(sSysPath, "\\RemoteControlServer.exe");
    if (PathFileExistsA(sSysPath))
    {
        TRACE("RemoteControlServer.exe already exists in system directory.\n");
        return;
    }

    char sPath[MAX_PATH] = "";
    GetCurrentDirectoryA(MAX_PATH, sPath);
    strcat_s(sPath, "\\RemoteCtrl.exe");

    char sLinkCmd[MAX_PATH * 2] = "MKLINK ";
    strcat_s(sLinkCmd, sSysPath);
    strcat_s(sLinkCmd, " ");
    strcat_s(sLinkCmd, sPath);
    TRACE("Link Command: [%s]\n", sLinkCmd);
    system(sLinkCmd);
}

BOOL WriteRegistryTable()
{
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    HKEY hKey = NULL;
    // KEY_ALL_ACCESS | KEY_WOW64_64KEY
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("Open Registry key failed. Don't have enough permission?\r\n Program start failed!"),
            _T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
        return FALSE;
    }
    // using REG_EXPAND_SZ auto expand %path%
    char sExePath[MAX_PATH] = "%SystemRoot%\\system32\\RemoteControlServer.exe";
    TRACE("sExePath Path: [%s]\n", sExePath);
    // RegSetValueExW(hKey, "RemoteCtrl", 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)sExePath, sExePath.GetLength() * sizeof(THAR))
    if (RegSetValueExA(hKey, "RemoteCtrl", 0, REG_EXPAND_SZ, (BYTE*)sExePath, (DWORD)strlen(sExePath)) != ERROR_SUCCESS)
    {
        MessageBox(NULL, _T("Set Registry key failed. Don't have enough permission?\r\n Program start failed!"),
            _T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
    }
    RegCloseKey(hKey);
    return TRUE;
}

BOOL WriteStartupDir()
{
	DWORD nSize = MAX_PATH;
    char sUser[MAX_PATH] = "";
	GetUserNameA(sUser, &nSize);
	TRACE("User Name: [%s]\n", sUser);
    // startup dir path
    char sStarupDir[MAX_PATH] = "C:\\Users\\";
	strcat_s(sStarupDir, sUser);
    // this path need run as administrator
    strcat_s(sStarupDir, "\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteControlServer.exe");
	if (PathFileExistsA(sStarupDir))
	{
        TRACE("RemoteControlServer.exe already exists in startup directory, Override it\n");
	}
    // get program startup cmdline
     LPSTR sCmd =  GetCommandLineA(); //it will contain extra "" which unncessary
    //char sPath[MAX_PATH] = "";
    //GetCurrentDirectoryA(MAX_PATH, sPath);
    //strcat_s(sPath, "\\RemoteCtrl.exe");

	TRACE("Program name: [%s], dst name:[%s]\n", sCmd, sStarupDir);
    // other choices: fopen, CFile, system(copy), OpenFile, mklin, create short cut.
    if (!CopyFileA(sCmd, sStarupDir, FALSE))
    {
		MessageBox(NULL, _T("Copy file to startup menu failed. Don't have enough permission?\r\n Program start failed!"),
			_T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
        return FALSE;
    }
    return TRUE;
}

/**
* Two ways:
* 1. Modify registry to set startup when booting : during logging
* 2. Add boot statrup option: win + r -> shell:startup -> create shortcut or copy file. : after logged in
* Configuration of Properties -> Advanced -> Use of MFC - > Use MFC in Static Library
* Linker->Manifest File->UAC Execution Level->requireAdministrator / asInvoker
* Computer\\HKEY_LOCAL_MACHINE\\SOFTWARE\Microsoft\\Windows\\CurrentVersion\\Run For boot startup
* your previldge is following invoker, if permission are not the same, it may cause some errors
* boot startup will affects env variables, dependent dll(system32(64bit program)) sysmWow64(32bit program)
* using static library
*/
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
        MakeLink();
        if (!WriteStartupDir() && !WriteRegistryTable())
        {
            ::exit(0);
        }
    }
    else if (ret == IDCANCEL)
    {
        ::exit(0);
    }
    return;
}

// Set Property->Linker->1. Entry point: mainCRTStartup, 2. SubSystem: Windows.
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
