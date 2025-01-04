
// RemoteClientDlg.h : header file
//

#pragma once
#include "StatusDlg.h"
#include "Packet.h"

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
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void GetDrivers(CPacket &packet);
	afx_msg void GetDiretories(CPacket* pPacket);
	afx_msg LRESULT OnSendPacketAck(WPARAM wParm, LPARAM lParam);
	CListCtrl m_list;
	CTreeCtrl m_tree;
	CString m_port;
	ULONG m_server_address;

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
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadDirectory();
	void LoadFiles();
public:
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
};
