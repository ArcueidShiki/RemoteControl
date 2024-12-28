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
END_MESSAGE_MAP()


// CWatchDlg message handlers


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
