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
	m_localPath = NULL;
	m_remotePath = NULL;
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
		{WM_SEND_PACKET, &CClientController::OnSendPacket},
		{WM_SEND_DATA, &CClientController::OnSendData},
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

BOOL CClientController::SendPacket(const CPacket& packet)
{
	CClientSocket* pClient = CClientSocket::GetInstance();
	if (!pClient) return FALSE;
	if (!pClient->InitSocket()) return FALSE;
	pClient->Send(packet);
	return 0;
}

int CClientController::SendCommandPacket(int nCmd, BOOL bAutoclose, BYTE* pData, size_t nLength)
{
	CClientSocket* pClient = CClientSocket::GetInstance();
	if (!pClient) return FALSE;
	if (!pClient->InitSocket()) return FALSE;
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// TODO: Shouldn't directly send, add packet to queue.
	if (!pClient->Send(CPacket(nCmd, pData, nLength, hEvent)))
	{
		TRACE("Client Send Paccket Failed\r\n");
		return -1;
	}
	// get return msg from server
	int rCmd = DealCommand();
	if (rCmd != nCmd)
	{
		TRACE("Get Return msg from server failed: rCmd: [%d]\r\n", rCmd);
		return -1;
	}
	if (bAutoclose) CloseSocket();
	return rCmd;
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
		CT2A DPath(fileDlg.GetPathName());
		m_localPath = DPath;

		CW2A asciiFullPath(strPath);
		m_remotePath = asciiFullPath;
		size_t len = strlen(m_remotePath);

		// std::thread(&CClientController::ThreadDownloadFile, this).detach();
		m_hThreadDownload = (HANDLE)_beginthread(&CClientController::ThreadDownloadEntry, 0, this);
		// why? just wait for it created?
		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		{
			return -1;
		}
		m_clientDlg->BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(L"Downloading...");
		m_statusDlg.ShowWindow(SW_SHOW); // active window accept keyboard input
		m_statusDlg.CenterWindow(m_clientDlg);
		m_statusDlg.SetActiveWindow();
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
	if (err != 0 || pFile == NULL)
	{
		AfxMessageBox(L"Open Dlg File Failed\n");
		TRACE("Open Download Path : [%s] Failed\n", m_localPath);
		m_statusDlg.ShowWindow(SW_HIDE);
		m_clientDlg->EndWaitCursor();
		return;
	}
	// send request
	int ret = SendCommandPacket(CMD_DLD_FILE, FALSE, (BYTE*)m_remotePath, strlen(m_remotePath));
	if (ret < 0)
	{
		AfxMessageBox(L"Donwload File Failed");
		TRACE("Download File Failed ret = %d\n", ret);
		m_statusDlg.ShowWindow(SW_HIDE);
		m_clientDlg->EndWaitCursor();
		return;
	}
	// receive file length
	auto nLength = *(long long*)CClientSocket::GetInstance()->GetPacket().strData.c_str();
	if (nLength == 0)
	{
		AfxMessageBox(L"File size is Zero or Download File Failed");
		TRACE("Download File Failed, File Size = 0\n");
		m_statusDlg.ShowWindow(SW_HIDE);
		m_clientDlg->EndWaitCursor();
		return;
	}
	TRACE("Client Filename:[%s], size:[%lld]\n", m_remotePath, nLength);
	long long nCount = 0;
	while (nCount < nLength)
	{
		ret = CClientController::GetInstance()->DealCommand();
		if (ret < 0)
		{
			AfxMessageBox(L"Transmission Failed\n");
			TRACE("Download File Failed, Deal Command Failed ret = %d\n", ret);
			m_statusDlg.ShowWindow(SW_HIDE);
			m_clientDlg->EndWaitCursor();
			break;
		}
		std::string data = CClientSocket::GetInstance()->GetPacket().strData;
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
	CClientController *self = (CClientController*)arg;
	self->ThreadFunc();
	_endthreadex(0);
	return 0;
}

void CClientController::ThreadWatchScreen()
{
	Sleep(50);
	while (!m_isWatchDlgClosed)
	{
		if (!m_watchDlg.isImageBufFull())
		{
			if (SendCommandPacket(CMD_SEND_SCREEN, FALSE, NULL, 0)
				== CMD_SEND_SCREEN)
			{
				if (GetImage(m_clientDlg->GetImage()) == 0)
				{
					m_watchDlg.SetIsImageBufFull(TRUE);
				}
				else
				{
					TRACE("Get Image Failed\n");
				}
			}
		}
		else
		{
			Sleep(1);
		}
	}
}

void __stdcall CClientController::ThreadWatchScreenEntry(void* arg)
{
	CClientController* self = (CClientController*)arg;
	self->ThreadWatchScreen();
	_endthread();
}

LRESULT CClientController::OnSendPacket(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CPacket* packet = (CPacket*)wParam;
	return CClientSocket::GetInstance()->Send(*packet);
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	char *pBuf = (char*)wParam;
	return CClientSocket::GetInstance()->Send(pBuf, (size_t)lParam);
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
