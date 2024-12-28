
// RemoteClientDlg.h : header file
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

// SEND PACKET MESSAGE
#define WM_SEND_PACKET (WM_USER + 1)
// CRemoteClientDlg dialog
class CRemoteClientDlg : public CDialogEx
{
// Construction
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// standard constructor
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif
	afx_msg void OnBnClickedBtnTest();
	afx_msg void OnBnClickedBtnFileinfo();
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnOpenFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartWatch();
	BOOL isFull() const;
	CImage& GetImage();
	void SetImageStatus(BOOL isFull = TRUE);
	CListCtrl m_list;
	CTreeCtrl m_tree;
	CString m_port;
	DWORD m_server_address;

protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	DECLARE_MESSAGE_MAP()
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

private:
	int SendCommandPacket(int nCmd, BOOL autoclose = TRUE, BYTE* pData = NULL, size_t nLength = 0);
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadDirectory();
	void LoadFiles();
	static void ThreadEntryDownloadFile(void* arg);
	static void ThreadEntryWatchData(void* arg);
	void ThreadDownloadFile();
	void ThreadWatchData();
	CClientSocket* pClient;
	CImage m_img;
	BOOL m_isFull;
};
