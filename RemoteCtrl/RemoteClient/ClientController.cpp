#include "pch.h"
#include "ClientController.h"
#include "Utils.h"
#include "Resource.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::GetInstance()
{
	if (!m_instance)
	{
		m_instance = new CClientController();
	}
	return m_instance;
}

CClientController::CClientController()
	: m_statusDlg(m_clientDlg)
	, m_watchDlg(m_clientDlg)
{
	m_localPath = new char[MAX_PATH];
	m_remotePath = new char[MAX_PATH];
	m_tid = UINT(-1);
	m_hThread = INVALID_HANDLE_VALUE;
	m_isWatchDlgClosed = TRUE;
	m_aRunning.store(FALSE);
	m_hThreadWatch = INVALID_HANDLE_VALUE;
}

int CClientController::InitController()
{
	// Create Handle Message Loop Thread with m_tid!
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::ThreadEntryMessageLoop, this, 0, &m_tid);
	m_statusDlg.Create(IDD_DLG_STATUS, m_clientDlg);
	struct msgfunc {
		UINT nMsg;
		MSGFUNC func;
	} MsgFuncs[] = {
		//{WM_SEND_STATUS, &CClientController::OnShowtatus},
		//{WM_SEND_WATCH, &CClientController::OnShowWatch},
		{WM_SEND_MESSAGE, NULL/*TODO*/},
		{UINT(-1), NULL}
	};
	for (int i = 0; i < sizeof(MsgFuncs) / sizeof(struct msgfunc); i++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
	}
	return 0;
}

int CClientController::Invoke(CWnd** pMainWnd)
{
	m_clientDlg = new CRemoteClientDlg();
	*pMainWnd = m_clientDlg;
	return (int)m_clientDlg->DoModal();
}

void CClientController::ReleaseInstance()
{
	if (m_instance)
	{
		// clear static resources
		m_mapFunc.clear();
		delete m_instance;
		m_instance = NULL;
	}
}

CClientController::~CClientController()
{
	m_aRunning.store(FALSE);
	TerminateThread(m_hThread, 0);
	//if (WaitForSingleObject(m_hThread, 500) != WAIT_OBJECT_0)
	//{
	//	// wait for m_hThread been signaled failed. force terminate thread.
	//	TerminateThread(m_hThread, 0);
	//}
	m_hThread = INVALID_HANDLE_VALUE;
	if (m_clientDlg)
	{
		delete m_clientDlg;
		m_clientDlg = NULL;
	}
	if (m_localPath)
	{
		delete[] m_localPath;
		m_localPath = NULL;
	}
	if (m_remotePath)
	{
		delete[] m_remotePath;
		m_remotePath = NULL;
	}
}

void CClientController::UpdateAddress(ULONG nIp, USHORT nPort)
{
	CClientSocket::GetInstance()->UpdateAddress(nIp, nPort);
}

int CClientController::DealCommand()
{
	return CClientSocket::GetInstance()->DealCommand();
}

void CClientController::CloseSocket()
{
	CClientSocket::GetInstance()->CloseSocket();
}

BOOL CClientController::SendCommandPacket(HWND hWnd, int nCmd, BYTE* pData, size_t nLength, BOOL bAutoClose, WPARAM wParam)
{
	return CClientSocket::GetInstance()->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
}

/**
* Memory to Image
*/
int CClientController::GetImage(CImage& img)
{
	return CUtils::Bytes2Image(img, CClientSocket::GetInstance()->GetPacket().strData);
}

int CClientController::DownloadFile(CString strPath)
{
	// False for Save sa, "extension", "filename", flags, filter, parent
	CFileDialog fileDlg(FALSE, L"*", strPath,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					NULL, m_clientDlg);
	INT_PTR action = fileDlg.DoModal();
	if (action == IDOK)
	{
		memset(m_localPath, 0, MAX_PATH);
		memset(m_remotePath, 0, MAX_PATH);

		CT2A LPath(fileDlg.GetPathName());
		char* ascciiLocal = LPath;
		memcpy(m_localPath, ascciiLocal, strlen(ascciiLocal) + 1);

		CW2A RPath(strPath);
		char* asciiRemote = RPath;
		memcpy(m_remotePath, asciiRemote, strlen(asciiRemote) + 1);
		size_t len = strlen(m_remotePath);

		TRACE("Remote Path:[%s], Local Path:[%s] len:[%d]\n", m_remotePath, m_localPath, strlen(m_remotePath));

		FILE* pFile;
		int err = fopen_s(&pFile, m_localPath, "wb+");
		if (err != 0 || pFile == NULL)
		{
			AfxMessageBox(L"Open Dlg File Failed\n");
			return -1;
		}
		int ret = SendCommandPacket(m_clientDlg->GetSafeHwnd(),
									CMD_DLD_FILE, (BYTE*)m_remotePath,
									strlen(m_remotePath), FALSE, (WPARAM)pFile);
		m_clientDlg->BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(L"Downloading...");
		m_statusDlg.ShowWindow(SW_SHOW); // active window accept keyboard input
		m_statusDlg.CenterWindow(m_clientDlg);
		m_statusDlg.SetActiveWindow();
	}
	else if (action == IDCANCEL)
	{
		m_clientDlg->MessageBox(L"Cancel download", L"Cancel");
		return -1;
	}

	return 0;
}

void CClientController::DownloadEnd()
{
	CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_clientDlg->EndWaitCursor();
	m_clientDlg->MessageBox(_T("Download End"), _T("Download End"));
}

void CClientController::StartWatchScreen()
{
	m_isWatchDlgClosed = FALSE;
	m_watchDlg = CWatchDlg(m_clientDlg);
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::ThreadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal();
	m_isWatchDlgClosed = TRUE;
	TerminateThread(m_hThreadWatch, 0);
	//if (WaitForSingleObject(m_hThreadWatch, 500) != WAIT_OBJECT_0)
	//{
	//	// wait for m_hThreadWatch been signaled failed. force terminate thread.
	//	TerminateThread(m_hThreadWatch, 0);
	//}
	m_hThreadWatch = INVALID_HANDLE_VALUE;
	// Don't double close, close NULL, or INVALID_HANDLE_VALUE
}

void CClientController::MessageLoop()
{
	MSG msg; // Message Loop
	m_aRunning.store(TRUE);
	while (m_aRunning.load() && ::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE)
		{
			HANDLE hEvent = (HANDLE)msg.lParam;
			MSGINFO* pMsg = (MSGINFO*)msg.wParam;

			// This has problems!!!
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pMsg->msg.message);
			if (it != m_mapFunc.end())
			{
				pMsg->result = (this->*(it->second))(
					pMsg->msg.message,
					pMsg->msg.wParam,
					pMsg->msg.lParam);
			}
			else
			{
				pMsg->result = -1;
			}
			// After Finishing handling message, set event flag as complete.
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end())
			{
				(this->*(it->second))(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

UINT __stdcall CClientController::ThreadEntryMessageLoop(void* arg)
{
	CClientController* self = (CClientController*)arg;
	self->MessageLoop();
	_endthreadex(0);
	return 0;
}

void CClientController::ThreadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isWatchDlgClosed)
	{
		if (!m_watchDlg.isImageBufFull())
		{
			if (GetTickCount64() - nTick < 200)
			{
				Sleep((DWORD)(nTick + 200 - GetTickCount64()));
			}
			// watchDlg Hwnd receive WM_SEND_PACKET_ACK msg
			// TODO: optimize this to udp and long connection
			if (!SendCommandPacket(m_watchDlg.GetSafeHwnd(), CMD_SEND_SCREEN, NULL, 0, FALSE))
			{
				TRACE("Get Image Failed\n");
			}
		}
		Sleep(1);
		nTick = GetTickCount64();
	}
}

void __stdcall CClientController::ThreadWatchScreenEntry(void* arg)
{
	CClientController* self = (CClientController*)arg;
	self->ThreadWatchScreen();
	_endthread();
}

#if 0
LRESULT CClientController::OnShowtatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
#endif

CClientController::CHelper::CHelper()
{
	CClientController::GetInstance();
}

CClientController::CHelper::~CHelper()
{
	CClientController::ReleaseInstance();
}

CClientController::MsgInfo::MsgInfo(MSG m)
{
	msg = m;
	result = 0;
}

CClientController::MsgInfo::MsgInfo(const MsgInfo& m)
{
	result = m.result;
	msg = m.msg;
}

CClientController::MsgInfo& CClientController::MsgInfo::operator=(const MsgInfo& m)
{
	if (this != &m)
	{
		result = m.result;
		msg = m.msg;
	}
	return *this;
}
