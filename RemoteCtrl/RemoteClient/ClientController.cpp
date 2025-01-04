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
	m_hThreadDownload = INVALID_HANDLE_VALUE;
	m_hThreadWatch = INVALID_HANDLE_VALUE;
	m_isWatchDlgClosed = TRUE;
}

int CClientController::InitController()
{
	// Create Handle Message Loop Thread with m_tid!
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::ThreadEntry, this, 0, &m_tid);
	m_statusDlg.Create(IDD_DLG_STATUS, m_clientDlg);
	struct {
		UINT nMsg;
		MSGFUNC func;
	} MsgFuncs[] = {
		{WM_SEND_STATUS, &CClientController::OnShowtatus},
		{WM_SEND_WATCH, &CClientController::OnShowWatch},
		{WM_SEND_MESSAGE, NULL/*TODO*/},
		{UINT(-1), NULL}
	};
	for (int i = 0; MsgFuncs[i].nMsg != -1; i++)
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
		delete m_instance;
		m_instance = NULL;
	}
}

CClientController::~CClientController()
{
	WaitForSingleObject(m_hThread, 100);
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

/**
* @msg: UINT nMsg Code: for cmd code
* @wParam: WPARAM autoclose?
*/
LRESULT CClientController::SendMsg(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent) return -2;
	// nMsg code(cmd) + result code
	MSGINFO info(msg);
	// tid, message code, msginfo(msg code + ret), event
	PostThreadMessage(m_tid, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
	// hEvent Setted represent handle message finished.
	// Wait for hEvent be signaled.
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	return info.result;
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
	return CClientSocket::GetInstance()->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose);
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
	if (fileDlg.DoModal() == IDOK)
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
		// std::thread(&CClientController::ThreadDownloadFile, this).detach();
		m_hThreadDownload = (HANDLE)_beginthread(&CClientController::ThreadDownloadEntry, 0, this);
		// why? just wait for it created?
		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		{
			return -1;
		}
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isWatchDlgClosed = FALSE;
	m_watchDlg = CWatchDlg(m_clientDlg);
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::ThreadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal();
	m_isWatchDlgClosed = TRUE;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::ThreadFunc()
{
	MSG msg; // Message Loop
	while (::GetMessage(&msg, NULL, 0, 0))
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

// Need return value ?
void CClientController::ThreadDownloadFile()
{
	FILE* pFile;
	int err = fopen_s(&pFile, m_localPath, "wb+");
	TRACE("Remote Path:[%s], Local Path:[%s] len:[%d]\n", m_remotePath, m_localPath, strlen(m_remotePath));
	if (err != 0 || pFile == NULL)
	{
		AfxMessageBox(L"Open Dlg File Failed\n");
		TRACE("Open Download Path : [%s] Failed\n", m_localPath);
		m_statusDlg.ShowWindow(SW_HIDE);
		m_clientDlg->EndWaitCursor();
		return;
	}
	// send request
	std::list<CPacket> lstAcks;
	int ret = SendCommandPacket(m_clientDlg->GetSafeHwnd(), CMD_DLD_FILE, (BYTE*)m_remotePath, strlen(m_remotePath), FALSE);
	if (ret < 0 || lstAcks.empty())
	{
		AfxMessageBox(L"Donwload File Failed");
		TRACE("Download File Failed ret = %d\n", ret);
		m_statusDlg.ShowWindow(SW_HIDE);
		m_clientDlg->EndWaitCursor();
		return;
	}
	// receive file length
	auto nLength = *(long long*)lstAcks.front().strData.c_str();
	if (nLength == 0)
	{
		AfxMessageBox(L"File size is Zero or Download File Failed");
		TRACE("Download File Failed, File Size = 0\n");
		m_statusDlg.ShowWindow(SW_HIDE);
		m_clientDlg->EndWaitCursor();
		return;
	}
	TRACE("Client Filename:[%s], size:[%lld]\n", m_localPath, nLength);
	long long nCount = 0;
	m_clientDlg->BeginWaitCursor();
	m_statusDlg.m_info.SetWindowText(L"Downloading...");
	m_statusDlg.ShowWindow(SW_SHOW); // active window accept keyboard input
	m_statusDlg.CenterWindow(m_clientDlg);
	m_statusDlg.SetActiveWindow();
	while (nCount < nLength)
	{
		/*ret = CClientController::GetInstance()->DealCommand();*/
		//if (ret < 0)
		//{
		//	AfxMessageBox(L"Transmission Failed\n");
		//	TRACE("Download File Failed, Deal Command Failed ret = %d\n", ret);
		//	m_statusDlg.ShowWindow(SW_HIDE);
		//	m_clientDlg->EndWaitCursor();
		//	break;
		//}
		lstAcks.pop_front();
		std::string data = lstAcks.front().strData;
		nCount += data.size();
		fwrite(data.c_str(), 1, data.size(), pFile);
	}
	fclose(pFile);
	m_statusDlg.ShowWindow(SW_HIDE);
	m_clientDlg->EndWaitCursor();

	CClientController::GetInstance()->CloseSocket();
}

void __stdcall CClientController::ThreadDownloadEntry(void* arg)
{
	CClientController* self = (CClientController*)arg;
	self->ThreadDownloadFile();
	_endthread();
}

UINT __stdcall CClientController::ThreadEntry(void* arg)
{
	CClientController* self = (CClientController*)arg;
	self->ThreadFunc();
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

LRESULT CClientController::OnShowtatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}

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
