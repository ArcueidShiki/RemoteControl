// LockDialog.cpp : implementation file
//

#include "pch.h"
#include "RemoteCtrl.h"
#include "afxdialogex.h"
#include "LockDialog.h"


// CLockDialog dialog

IMPLEMENT_DYNAMIC(CLockDialog, CDialog)

CLockDialog::CLockDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_INFO, pParent)
{

}

CLockDialog::~CLockDialog()
{
}

void CLockDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

/**
only work for modal dialog, non modal dialog, need to handle message loop.
*/
void CLockDialog::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        EndDialog(IDCANCEL); // Close the dialog
    }
    CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

BEGIN_MESSAGE_MAP(CLockDialog, CDialog)
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// CLockDialog message handlers
