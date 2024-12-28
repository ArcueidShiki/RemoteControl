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


CPoint CWatchDlg::UserPoint2RemoteScreen(CPoint& pt)
{
	CRect clientRect;
	ScreenToClient(&pt);
	m_picture.GetWindowRect(&clientRect);
	// local coordinate to remote screen coordinate
	int x = pt.x * 3440 / clientRect.Width();
	int y = pt.y * 1440 / clientRect.Height();

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


void CWatchDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_LEFT, MOUSE_DBCLICK, remote);
	CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	pClient->Send(packet);
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	//CPoint remote = UserPoint2RemoteScreen(point);
	//// POINT is tagPOINT: the parent of CPoint
	//MOUSEEV event(LEFT, DOWN, remote);
	//CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	//pClient->Send(packet);
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	//CPoint remote = UserPoint2RemoteScreen(point);
	//// POINT is tagPOINT: the parent of CPoint
	//MOUSEEV event(LEFT, UP, remote);
	//CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	//pClient->Send(packet);
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_RIGHT, MOUSE_DBCLICK, remote);
	CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	pClient->Send(packet);
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_RIGHT, MOUSE_DOWN, remote);
	CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	pClient->Send(packet);
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_RIGHT, MOUSE_UP, remote);
	CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	pClient->Send(packet);
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreen(point);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_MOVE, MOUSE_MOVE, remote);
	CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	pClient->Send(packet);
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDlg::OnStnClickedWatch()
{
	CPoint pt;
	GetCursorPos(&pt);
	CPoint remote = UserPoint2RemoteScreen(pt);
	// POINT is tagPOINT: the parent of CPoint
	MOUSEEV event(MOUSE_LEFT, MOUSE_DOWN, remote);
	CPacket packet(CMD_MOUSE, (BYTE*)&event, sizeof(MOUSEEV));
	pClient->Send(packet);
}
