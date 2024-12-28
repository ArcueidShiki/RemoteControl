#pragma once
#include "afxdialogex.h"


// CStatusDlg dialog

class CStatusDlg : public CDialog
{
	DECLARE_DYNAMIC(CStatusDlg)

public:
	CStatusDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CStatusDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_STATUS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_info;
};
