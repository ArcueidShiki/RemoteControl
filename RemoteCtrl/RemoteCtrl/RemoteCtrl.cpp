// RemoteCtrl.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

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
            // global variable, only one instance
            CServerSocket* pServer = CServerSocket::GetInstance();
            int count = 0;
            while (pServer)
            {
                if (pServer->AcceptClient() == FALSE)
                {
                    if (count > 3)
                    {
						MessageBox(NULL, _T("�޷����������û����Զ����Դ������࣬�����˳�"), _T("�����û�ʧ��!"), MB_OK | MB_ICONERROR);
						break;
                    }
                    MessageBox(NULL, _T("�޷����������û����Զ�����"), _T("�����û�ʧ��!"), MB_OK | MB_ICONERROR);
                    count++;
                }

				if (pServer->DealCommand() == -1)
				{
					MessageBox(NULL, _T("�޷����������û�����"), _T("�����û�����ʧ��!"), MB_OK | MB_ICONERROR);
					break;
				}
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
