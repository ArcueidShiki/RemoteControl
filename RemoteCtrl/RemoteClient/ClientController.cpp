#include "pch.h"
#include "ClientController.h"
#include "Utils.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::GetInstance()
{
	if (!m_instance)
	{
		m_instance = new CClientController();
	}
	return nullptr;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::ThreadEntry, this, 0, &m_tid);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_clientDlg);
	return 0;
}

int CClientController::Invoke(CWnd** pMainWnd)
{
	*pMainWnd = &m_clientDlg;
	return (int)m_clientDlg.DoModal();
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
	// tid, message code, msginfo, uuid
	PostThreadMessage(m_tid, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent);
	WaitForSingleObject(hEvent, INFINITE);
	return info.result;
}

void CClientController::UpdateAddress(ULONG nIp, USHORT nPort)
{
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
	if (!pClient->Send(CPacket(nCmd, pData, nLength)))
	{
		TRACE("Client Send Paccket Failed\r\n");
		return -1;
	}
	// get return msg from server
	int cmd = DealCommand();
	if (cmd == -1)
	{
		TRACE("Get Return msg from server failed\r\n");
		return -1;
	}
	if (bAutoclose) CloseSocket();
	return cmd;
}

/**
* Memory to Image
*/
int CClientController::GetImage(CImage& img)
{
	return CUtils::Bytes2Image(img, CClientSocket::GetInstance()->GetPacket().strData);
}

void CClientController::ReleaseInstance()
{
	if (m_instance)
	{
		delete m_instance;
		m_instance = NULL;
	}
}

CClientController::CClientController()
	: m_statusDlg(&m_statusDlg)
	, m_clientDlg(&m_clientDlg)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_tid = -1;
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
}

CClientController::~CClientController()
{
	WaitForSingleObject(m_hThread, 100);
}

void CClientController::ThreadFunc()
{
	MSG msg;
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

UINT __stdcall CClientController::ThreadEntry(void* arg)
{
	CClientController *self = (CClientController*)arg;
	self->ThreadFunc();
	_endthreadex(0);
	return 0;
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
