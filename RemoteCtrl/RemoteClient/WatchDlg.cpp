// WatchDlg.cpp : implementation file
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDlg.h"
#include "ClientController.h"
#include "Utils.h"

// CWatchDlg dialog
#ifndef WM_SEND_PACKET
#define WM_SEND_PACKET (WM_USER + 1)
#endif

#ifndef WM_SEND_PACKET_ACK
#define WM_SEND_PACKET_ACK (WM_USER + 2)
#endif

IMPLEMENT_DYNAMIC(CWatchDlg, CDialog)

CWatchDlg::CWatchDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_nRemoteWidth = -1;
	m_nRemoteHeight = -1;
	m_isImageBufFull = FALSE;
}

CWatchDlg& CWatchDlg::operator=(const CWatchDlg& other)
{
	if (this != &other)
	{
		m_nRemoteWidth = other.m_nRemoteWidth;
		m_nRemoteHeight = other.m_nRemoteHeight;
		m_isImageBufFull = other.m_isImageBufFull;
	}
	return *this;
}

CWatchDlg::~CWatchDlg()
{
}

void CWatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}

BEGIN_MESSAGE_MAP(CWatchDlg, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDlg::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDlg::OnBnClickedBtnUnlock)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDlg::OnBnClickedBtnLock)
	ON_MESSAGE(WM_SEND_PACKET_ACK, &CWatchDlg::OnSendPacketAck)
END_MESSAGE_MAP()

// CWatchDlg message handlers

CPoint CWatchDlg::UserPoint2RemoteScreen(CPoint& pt, BOOL isScreen/* = FALSE*/)
{
	CRect clientRect;
	if (!isScreen) 
	{
		ClientToScreen(&pt);
	}
	m_picture.ScreenToClient(&pt);
	m_picture.GetWindowRect(&clientRect);
	// local coordinate to remote screen coordinate
	int x = pt.x * m_nRemoteWidth / clientRect.Width();
	int y = pt.y * m_nRemoteHeight / clientRect.Height();
	return CPoint(x, y);
}

BOOL CWatchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	//SetTimer(0, 50, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CWatchDlg::isImageBufFull() const
{
	return m_isImageBufFull;
}

void CWatchDlg::SetIsImageBufFull(BOOL isFull)
{
	m_isImageBufFull = isFull;
}

void CWatchDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (m_isImageBufFull)
		{
			CRect rect;
			m_picture.GetWindowRect(&rect);
			m_nRemoteWidth = m_img.GetWidth();
			m_nRemoteHeight = m_img.GetHeight();
			m_img.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			m_img.Destroy();
			m_isImageBufFull = FALSE;
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CWatchDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nRemoteHeight == -1 || m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint remote = UserPoint2RemoteScreen(point);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_LEFT, MOUSE_DBCLICK, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CWatchDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nRemoteHeight == -1 && m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_LEFT, MOUSE_DOWN, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
	CDialog::OnLButtonDown(nFlags, point);
}

void CWatchDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nRemoteHeight == -1 && m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_LEFT, MOUSE_UP, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
	CDialog::OnLButtonUp(nFlags, point);
}

void CWatchDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nRemoteHeight == -1 && m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_RIGHT, MOUSE_DBCLICK, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
	CDialog::OnRButtonDblClk(nFlags, point);
}

void CWatchDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nRemoteHeight == -1 && m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_RIGHT, MOUSE_DOWN, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
	CDialog::OnRButtonDown(nFlags, point);
}

void CWatchDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nRemoteHeight == -1 && m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_RIGHT, MOUSE_UP, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
	CDialog::OnRButtonUp(nFlags, point);
}

void CWatchDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nRemoteHeight == -1 && m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_MOVE, MOUSE_MOVE, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
	CDialog::OnMouseMove(nFlags, point);
}

void CWatchDlg::OnStnClickedWatch()
{
	if (m_nRemoteHeight == -1 && m_nRemoteWidth == -1)
	{
		return;
	}
	CPoint pt;
	GetCursorPos(&pt);
	CPoint remote = UserPoint2RemoteScreen(pt, TRUE);
	MOUSEEV event(MOUSE_LEFT, MOUSE_DOWN, remote);
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_MOUSE, (BYTE*)&event, sizeof(event));
}

void CWatchDlg::OnOK()
{
	//CDialog::OnOK();
}


void CWatchDlg::OnBnClickedBtnUnlock()
{
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_UNLOCK_MACHINE);
}


void CWatchDlg::OnBnClickedBtnLock()
{
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_LOCK_MACHINE);
}

void CWatchDlg::ShowImage(std::string &data)
{
	if (m_isImageBufFull)
	{
		return;
	}
	if (CUtils::Bytes2Image(m_img, data) == 0)
	{
		m_isImageBufFull = TRUE;
	}
	CRect rect;
	m_picture.GetWindowRect(&rect);
	m_nRemoteWidth = m_img.GetWidth();
	m_nRemoteHeight = m_img.GetHeight();
	m_img.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
	m_picture.InvalidateRect(NULL);
	m_img.Destroy();
	m_isImageBufFull = FALSE;
}

LRESULT CWatchDlg::OnSendPacketAck(WPARAM wParm, LPARAM lParam)
{
	if (lParam < 0)
	{
		// TODO: error handle
	}
	else if (lParam == 1)
	{
		// server side close socket.
	}
	else
	{
		CPacket* pPacket = (CPacket*)wParm;
		CPacket head;
		if (pPacket)
		{
			head = *pPacket;
			delete pPacket;
			switch (head.sCmd)
			{
			case CMD_SEND_SCREEN:
				ShowImage(head.strData);
				break;
			case CMD_MOUSE:
				break;
			case CMD_LOCK_MACHINE:
				break;
			case CMD_UNLOCK_MACHINE:
				break;
			default:
				break;
			}
		}
	}
	return 0;
}
