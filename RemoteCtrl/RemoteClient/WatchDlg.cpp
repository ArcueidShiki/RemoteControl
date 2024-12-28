// WatchDlg.cpp : implementation file
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDlg.h"
#include "RemoteClientDlg.h"

// CWatchDlg dialog

IMPLEMENT_DYNAMIC(CWatchDlg, CDialog)

CWatchDlg::CWatchDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	pClient = CClientSocket::GetInstance();
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
END_MESSAGE_MAP()

// CWatchDlg message handlers

CPoint CWatchDlg::UserPoint2RemoteScreen(CPoint& pt, BOOL isScreen/* = FALSE*/)
{
	CRect clientRect;
	if (isScreen) ScreenToClient(&pt);
	TRACE("%s x:%d, y:d\n", __FUNCTION__, pt.x, pt.y);
	m_picture.GetWindowRect(&clientRect);
	// local coordinate to remote screen coordinate
	int x = pt.x * 1920 / clientRect.Width();
	int y = pt.y * 1080 / clientRect.Height();
	TRACE("%s 2 x:%d, y:d\n", __FUNCTION__,x, y);
	return CPoint(x, y);
}

BOOL CWatchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetTimer(0, 50, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CWatchDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull())
		{
			CRect rect;
			m_picture.GetWindowRect(&rect);
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
			pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->SetImageStatus(FALSE);
		}
	}
	CDialog::OnTimer(nIDEvent);
}

// Should be tested on vmware window >= 10
// network communication should be decoupled with dlg UI.

void CWatchDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_LEFT, MOUSE_DBCLICK, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CWatchDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	TRACE("x:%d, y:d\n", point.x, point.y);
	CPoint remote = UserPoint2RemoteScreen(point);
	TRACE("remote: x:%d, y:d\n", remote.x, remote.y);
	MOUSEEV event(MOUSE_LEFT, MOUSE_DOWN, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
	CDialog::OnLButtonDown(nFlags, point);
}

void CWatchDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_LEFT, MOUSE_UP, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
	CDialog::OnLButtonUp(nFlags, point);
}

void CWatchDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_RIGHT, MOUSE_DBCLICK, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
	CDialog::OnRButtonDblClk(nFlags, point);
}

void CWatchDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_RIGHT, MOUSE_DOWN, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
	CDialog::OnRButtonDown(nFlags, point);
}

void CWatchDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_RIGHT, MOUSE_UP, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
	CDialog::OnRButtonUp(nFlags, point);
}

void CWatchDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	MOUSEEV event(MOUSE_MOVE, MOUSE_MOVE, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
	CDialog::OnMouseMove(nFlags, point);
}

void CWatchDlg::OnStnClickedWatch()
{
	CPoint pt;
	GetCursorPos(&pt);
	CPoint remote = UserPoint2RemoteScreen(pt, TRUE);
	MOUSEEV event(MOUSE_LEFT, MOUSE_DOWN, remote);
	(CRemoteClientDlg*)m_pParentWnd->SendMessage(WM_SEND_PACKET, CMD_MOUSE << 1 | TRUE, (WPARAM)&event);
}