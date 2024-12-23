// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Set Property->Linker->1. Entry point: mainCRTStartup, 2. SubSystem: Windows.

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>

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
        if (i > 0 && (i % 16 == 0)) strOut += '\n';
        snprintf(buf, sizeof(buf), "%02X ", pData[i]);
        strOut += buf;
    }
	strOut += '\n';
    OutputDebugStringA(strOut.c_str());
}

/**
* Lookup files, need driver partitions
*/
int MakeDriverInfo()
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

    // sCmd, data "C,D,E"
    CPacket packet(CMD_DRIVER, (BYTE*)result.c_str(), result.size());
    // FF FE(head) 09 00 00 00(length = datasize + cmd+sum) 01 00(cnd) 43 2C 44 2C 45(C(43),D(44),E(45)) 24 01(01 24 = 43 + 2C + 44 + 2C + 45)
    Dump((BYTE*)packet.Data(), packet.Size());

	//CServerSocket::GetInstance()->Send(packet);
    return 0;
}

typedef struct file_info{
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
} FILEINFO, *PFILEINFO;

/**
* Lookup directories
*/
int MakeDirectoryInfo()
{
    std::string strPath;
    if (!CServerSocket::GetInstance()->GetFilePath(strPath))
    {
		OutputDebugString(_T("GetFilePath failed, Current cmd is not get file list, parse failed!!!\n"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0)
    {
        FILEINFO finfo;
		finfo.IsDirectory = TRUE;
        finfo.IsValid = FALSE;
		finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
		OutputDebugString(_T("No permission to access the directory\n"));
        CPacket packet(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::GetInstance()->Send(packet);
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1 )
    {
		OutputDebugString(_T("No files in the directory\n"));
		return -3;
    }
    do {
        FILEINFO finfo;
        finfo.IsDirectory = fdata.attrib & _A_SUBDIR;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        CPacket packet(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::GetInstance()->Send(packet);
    } while (!_findnext(hfind, &fdata));
    // when tmpfiles or logfiles, are huge, so send one file every time read.
    // tell client, end.
    FILEINFO finfo;
    finfo.HasNext = FALSE;
    CPacket packet(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
    CServerSocket::GetInstance()->Send(packet);
    return 0;
}

int RunFile()
{
    std::string strPath;
    CServerSocket::GetInstance()->GetFilePath(strPath);
    // open / run file.
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CPacket packet(CMD_RUN_FILE, NULL, 0);
    CServerSocket::GetInstance()->Send(packet);
    return 0;
}

int DownloadFile()
{
#define DLD_BUF_SIZE 1024 // local scope macro, is not accessible globally.
    std::string strPath;
    CServerSocket::GetInstance()->GetFilePath(strPath);
    FILE* pFile = NULL;
	// if you open first, then, other process open this in exclusive mode, you have the pointer, but cannot do anything.
    // Or property->C/C++->preprocessor->_CRT_SECURE_NO_WARNINGS
    // Or #pragma warning(disable:4996)
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (!pFile || err != 0)
    {
		CPacket packet(CMD_DLD_FILE, NULL, 0);
		CServerSocket::GetInstance()->Send(packet);
		return -1;
    }
    fseek(pFile, 0, SEEK_END);
    long long data = _ftelli64(pFile);
    CPacket head(CMD_DLD_FILE, (BYTE*)&data, sizeof(data));
    fseek(pFile, 0, SEEK_SET);
    char buf[DLD_BUF_SIZE] = "";
    size_t rlen = 0;
    do {
        rlen = fread(buf, 1, DLD_BUF_SIZE, pFile);
        CPacket packet(CMD_DLD_FILE, (BYTE*)buf, rlen);
        CServerSocket::GetInstance()->Send(packet);
	} while (rlen >= DLD_BUF_SIZE);
    CPacket packet(CMD_DLD_FILE, NULL, 0);
    CServerSocket::GetInstance()->Send(packet);
    fclose(pFile);
	return 0;
}

int MouseEvent()
{
#define MOVE 0x00
#define LEFT 0x01
#define RIGHT 0x02
#define MIDDLE 0x04
#define CLICK 0x08
#define DBCLICK 0x10
#define DOWN 0x20
#define UP 0x40

    MOUSEEV mouse;
    if (CServerSocket::GetInstance()->GetMouseEvent(mouse))
    {
        DWORD operation = mouse.nButton;
        if (operation != MOVE)
			SetCursorPos(mouse.point.x, mouse.point.y);
        operation |= mouse.nAction;
        switch (operation)
        {
		    case LEFT | CLICK:
			    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case LEFT | DBCLICK:
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case LEFT | DOWN:
			    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case LEFT | UP:
			    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case RIGHT | CLICK:
			    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case RIGHT | DBCLICK:
			    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case RIGHT | DOWN:
			    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case RIGHT | UP:
			    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case MIDDLE | CLICK:
			    mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case MIDDLE | DBCLICK:
			    mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			    mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case MIDDLE | DOWN:
			    mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case MIDDLE | UP:
			    mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			    break;
		    case MOVE:
				mouse_event(MOUSEEVENTF_MOVE, mouse.point.x, mouse.point.y, 0, GetMessageExtraInfo());
        }
        CPacket packet(CMD_MOUSE, NULL, 0);
        CServerSocket::GetInstance()->Send(packet);
    }
    else
    {
		OutputDebugString(_T("GetMouseEvent failed, Current cmd is not mouse event, parse failed!!!\n"));
        return -1;
    }
    return 0;
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
            int nCmd = 1;
            switch (nCmd)
            {
                case CMD_DRIVER:
				    MakeDriverInfo();
				    break;
                case CMD_DIR:
                    MakeDirectoryInfo();
                    break;
                case CMD_RUN_FILE:
					RunFile();
					break;
                case CMD_DLD_FILE:
                    DownloadFile();
                    break;
                case CMD_MOUSE:
                    MouseEvent();
                    break;
                default:
                    break;
            }
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
