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
#endif

// Set Property->Linker->1. Entry point: mainCRTStartup, 2. SubSystem: Windows.
int main()
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
    CQueue<std::string> lstString;
    ULONGLONG tick = GetTickCount64();
    ULONGLONG tick1 = GetTickCount64();
    while (_kbhit() == 0)
    {
        if (GetTickCount64() - tick > 1300)
        {
            // separate request and response
			lstString.PushBack("hello push\r\n");
            tick = GetTickCount64();
        }
        if (GetTickCount64() - tick1 > 2000)
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
