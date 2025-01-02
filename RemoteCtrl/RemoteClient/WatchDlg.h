#pragma once
#include "afxdialogex.h"
// CWatchDlg dialog

class CWatchDlg : public CDialog
{
	DECLARE_DYNAMIC(CWatchDlg)

public:
	CWatchDlg(CWnd* pParent = nullptr);   // standard constructor
	CWatchDlg& operator=(const CWatchDlg& other);
	virtual ~CWatchDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif

public:
	CPoint UserPoint2RemoteScreen(CPoint& pt, BOOL isScreen = FALSE);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	afx_msg void OnBnClickedBtnUnlock();
	afx_msg void OnBnClickedBtnLock();
	BOOL isImageBufFull() const;
	void SetIsImageBufFull(BOOL isFull = TRUE);
	void OnOK();
	CStatic m_picture;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	int m_nRemoteWidth;
	int m_nRemoteHeight;
	BOOL m_isImageBufFull;
};
