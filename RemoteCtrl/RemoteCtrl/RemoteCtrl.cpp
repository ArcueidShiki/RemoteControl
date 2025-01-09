// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "LockDialog.h"
#include "Utils.h"
#include "Command.h"
#include <conio.h>
#include "Queue.h"
#include <MSWSock.h>
#include "Server.h"

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

#if 0
// IOCP
enum {
    IocpListEmpty,
	IocpListPush,
	IocpListPop,
};

void Func(void* arg)
{
    std::string* pstr = (std::string*)arg;
    if (pstr->empty())
    {
        printf("List is empty. no data\r\n");
    }
    else
    {
        printf("pop from list:%s\r", pstr->c_str());
    }
}

typedef struct IocpParam
{
    int nOpt;
    std::string strData;
    _beginthread_proc_type cbFunc;
	IocpParam(int nOpt, const std::string& strData,
              _beginthread_proc_type cbFunc = NULL)
        : nOpt(nOpt), strData(strData)
    {
        this->cbFunc = cbFunc;
    }
    IocpParam() {}
} IOCP_PARAM;


void ThreadMain(HANDLE hIOCP)
{
    std::list<std::string> lstString;
    DWORD dwTransferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED Overlapped = { 0 };
    LPOVERLAPPED pOverlapped = &Overlapped;
    // wait for iocp to be signaled, otherwise sleep
    while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
    {
        if (dwTransferred == 0 || CompletionKey == 0)
        {
            printf("Thread is prepare to exit\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
        if (pParam->nOpt == IocpListPush)
        {
            lstString.push_back(pParam->strData);
        }
        else if (pParam->nOpt == IocpListPop)
        {
            std::string str = "";
            if (!lstString.empty())
            {
                str = lstString.front();
                lstString.pop_front();
            }
            if (pParam->cbFunc)
            {
                pParam->cbFunc(&str);
            }
        }
        else
        {
            lstString.clear();
        }
        delete pParam;
    }
    lstString.clear();
}

void ThreadQueueEntry(HANDLE hIOCP)
{
    ThreadMain(hIOCP);
	_endthread(); // may cause local variable to unable to destroyed leading to memory leak
}

void test()
{
    CQueue<std::string> lstString;
    ULONGLONG tick = GetTickCount64();
    ULONGLONG tick1 = GetTickCount64();
    ULONGLONG total = GetTickCount64();
    //while (_kbhit() == 0)
    while (GetTickCount64() - total < 1000)
    {
        if (GetTickCount64() - tick > 13)
        {
            // separate request and response
            lstString.PushBack("Item");
            tick = GetTickCount64();
        }
        if (GetTickCount64() - tick1 > 20)
        {
            std::string str;
            lstString.PopFront(str);
            printf("pop from list:%s\r\n", str.c_str());
            tick1 = GetTickCount64();
        }
        Sleep(1);
    }
    printf("Size: %zu\n", lstString.Size());
    lstString.Clear();
    printf("Cleared, Size: %zu\n", lstString.Size());
}

#endif


void iocp()
{
    Server server;
    server.StartService();
	printf("Press any key to exit\n");
    getchar();
    printf("Exit\n");
}

void udp_server();
void udp_client(BOOL isHost = TRUE);

//int wmain(int argc, wchar_t *argv[])
//int _tmain(int argc, TCHAR *argv[])

// Set Property->Linker->1. Entry point: mainCRTStartup, 2. SubSystem: Windows.
int main(int argc, char *argv[])
{
#if 0
    if (!CUtils::IsAdmin())
    {
        return CUtils::RunAsAdmin() ? 0 : 1;
    }
#endif

    if (!CUtils::Init())
    {
        return 1;
    }

    if (argc == 1)
    {   // udp server
        char strDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, strDir);
        STARTUPINFOA si = { 0 };
        PROCESS_INFORMATION pi = {};
        string strCmd = argv[0];
        strCmd += " 1"; // argc = 2
        BOOL ret = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, strDir, &si, &pi);
        if (ret)
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            TRACE("Process pid:%d, tid:%d\n", pi.dwProcessId, pi.dwThreadId);
            strCmd += " 2"; // argc = 3
            ret = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, strDir, &si, &pi);
			if (ret)
			{
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("Process pid:%d, tid:%d\n", pi.dwProcessId, pi.dwThreadId);
                udp_server();
			}
        }
    }
    else if (argc == 2)
    {
        // client host
        udp_client();
    }
    else
    {   // client servant
        udp_client(FALSE);
    }
    //iocp();

#if 0
    HANDLE hIOCP = INVALID_HANDLE_VALUE;
    // HANDLE: socket, file, serial port
	// epoll is single thread, IOCP allows multi-thread
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
    // put IOCP into a thread,
    HANDLE hThread = (HANDLE)_beginthread(ThreadQueueEntry, 0, hIOCP);
	ULONGLONG tick = GetTickCount64();
	ULONGLONG tick1 = GetTickCount64();

    if (hIOCP != NULL)
    {
        // INVOKE 
        while (_kbhit() == 0)
        {
            if (GetTickCount64() - tick > 1300)
            {
				// separate request and response
                PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "Msg pop\r\n", &Func), NULL);
                tick = GetTickCount64();
            }
            if (GetTickCount64() - tick1 > 2000)
            {
                PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello Pushed\r\n"), NULL);
                tick1 = GetTickCount64();
            }
            Sleep(1);
        }
        PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hIOCP);
    }
    printf("exit\n");
#endif

#if 0
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
#endif
    return 0;
}

void udp_server()
{
    printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
    getchar();
}

void udp_client(BOOL isHost)
{
    if (isHost)
    {
        printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
    }
    else
    {
        printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
    }
}