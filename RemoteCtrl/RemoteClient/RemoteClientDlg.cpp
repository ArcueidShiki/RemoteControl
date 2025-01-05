
// RemoteClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDlg.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#ifndef WM_SEND_PACKET_ACK
#define WM_SEND_PACKET_ACK (WM_USER + 2)
#endif

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg dialog

CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_port(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_port);
	DDX_Control(pDX, IDC_TREE_DIR, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_list);
	DDX_Control(pDX, IDC_LIST_FILE, m_list);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_OPEN_FILE, &CRemoteClientDlg::OnOpenFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACKET_ACK, &CRemoteClientDlg::OnSendPacketAck)
END_MESSAGE_MAP()

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	InitUiData();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRemoteClientDlg::OnBnClickedBtnTest()
{
	CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_DRIVER);
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	if (CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_DRIVER, NULL, 0) == -1)
	{
		AfxMessageBox(_T("Send Command Packet Failed"));
	}
}

/**
* Get the full directory path of clicked tree item.
* @param hTree: tree node
* @return CString: full directory path
*/
CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strParent;
	strRet = m_tree.GetItemText(hTree);
	// from leaf to root
	HTREEITEM hParent = m_tree.GetParentItem(hTree);
	while (hParent) {
		strParent = m_tree.GetItemText(hParent);
		strRet = strParent + L"\\" + strRet;
		hParent = m_tree.GetParentItem(hParent);
	}
	strRet += L"\\";
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_tree.GetChildItem(hTree);
		if (hSub) m_tree.DeleteItem(hSub);
	} while (hSub);
}

// TODO using cache
void CRemoteClientDlg::LoadDirectory()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse); // global point to screen
	m_tree.ScreenToClient(&ptMouse); // screen to client
	TRACE("PT mouse: x[%d] y[%d]\n", ptMouse.x, ptMouse.y);
	m_hTreeSelected = m_tree.HitTest(ptMouse, 0);
	if (m_hTreeSelected == NULL)
	{
		TRACE("No Item Selected\n");
		return;
	}

	UINT state = m_tree.GetItemState(m_hTreeSelected, TVIS_EXPANDED);
	if (state & TVIS_EXPANDED)
	{
		// The item is already expanded, so collapse it
		m_tree.Expand(m_hTreeSelected, TVE_COLLAPSE);
		return;
	}
	DeleteTreeChildrenItem(m_hTreeSelected);
	m_list.DeleteAllItems();
	CString strPath = GetPath(m_hTreeSelected);
	CT2A asciiPath(strPath);
	const char* path = asciiPath;
	size_t strLen = strlen(asciiPath);
	TRACE("strPath: %s Length: %zu\n", path, strLen);
	int nCmd = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_DIR, (BYTE*)path, strLen, FALSE);
}

void CRemoteClientDlg::LoadFiles()
{
	HTREEITEM hTreeSelected = m_tree.GetSelectedItem();
	CString strPath = GetPath(hTreeSelected);
	m_list.DeleteAllItems();
	CT2A asciiPath(strPath);
	const char* path = asciiPath;
	size_t strLen = strlen(asciiPath);
	TRACE("strPath: %s Length: %zu\n", path, strLen);
	int nCmd = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_DIR, (BYTE*)path, strLen, FALSE);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadDirectory();
}

void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadDirectory();
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_list.ScreenToClient(&ptList);
	int ListSelected = m_list.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu *pPopup = menu.GetSubMenu(0);
	if (pPopup)
	{
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_list.GetSelectionMark();
	CString strFile = m_list.GetItemText(nListSelected, 0);
	HTREEITEM hTreeSelected = m_tree.GetSelectedItem();
	CString strPath = GetPath(hTreeSelected) + strFile;

	int ret = CClientController::GetInstance()->DownloadFile(strPath);
	if (ret != 0)
	{
		MessageBox(_T("Download File Failed"), _T("Download Failed"));
		TRACE("Download Failed ret = %d\r\n", ret);
	}
}

void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hTreeSelected = m_tree.GetSelectedItem();
	CString dirPath = GetPath(hTreeSelected);
	int nListSelected = m_list.GetSelectionMark();
	CString strFile = m_list.GetItemText(nListSelected, 0);
	CT2A asciiFullPath(dirPath + strFile);
	char* fullpath = asciiFullPath;
	// wcstombs_s(&nConverted, fullpath, MAX_PATH, asciiFullPath, MAX_PATH);
	TRACE("Full File Path : [%s], name len:[%zu]\n", fullpath, strlen(fullpath));
	int ret = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_DEL_FILE, (BYTE*)fullpath, strlen(fullpath));
	if (ret < 0)
	{
		AfxMessageBox(L"Delete File Failed");
		TRACE("Delete File Failed, ret = %d\n", ret);
	}
	else
	{
		MessageBox(L"Delete File Success", L"Delete File", MB_ICONINFORMATION);
	}
}


void CRemoteClientDlg::OnOpenFile()
{
	HTREEITEM hTreeSelected = m_tree.GetSelectedItem();
	CString dirPath = GetPath(hTreeSelected);
	int nListSelected = m_list.GetSelectionMark();
	CString strFile = m_list.GetItemText(nListSelected, 0);
	CT2A asciiFullPath(dirPath + strFile);
	char* fullpath = asciiFullPath;
	TRACE("Full File Path : [%s], name len:[%zu]\n", fullpath, strlen(fullpath));
	int ret = CClientController::GetInstance()->SendCommandPacket(GetSafeHwnd(), CMD_RUN_FILE, (BYTE*)fullpath, strlen(fullpath));
	if (ret < 0)
	{
		AfxMessageBox(L"Open File Failed");
		TRACE("Open File Failed, ret = %d\n", ret);
	}
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::GetInstance()->StartWatchScreen();
}

void CRemoteClientDlg::GetDrivers(CPacket &response)
{

	std::string drivers = response.strData;
	std::string driver;
	m_tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] != ',')
		{
			driver = drivers[i];
			driver += ":";
			HTREEITEM hCur = m_tree.InsertItem(CString(driver.c_str()), TVI_ROOT, TVI_LAST);
			m_tree.InsertItem(L"", hCur, TVI_LAST);
			driver.clear();
		}
	}
	UpdateData(FALSE);
}

void CRemoteClientDlg::GetFile(CPacket &response)
{
	PFILEINFO pInfo = (PFILEINFO)response.strData.c_str();
	if (pInfo->HasNext)
	{
		TRACE("Client receive file, name: [%s] Has Next: [%d] \n", pInfo->szFileName, pInfo->HasNext);
		if (!pInfo->IsDirectory)
		{
			// FILE
			//TRACE("Insert file:%s\n", pInfo->szFileName);
			m_list.InsertItem(0, CString(pInfo->szFileName));
		}
		else if (strcmp(pInfo->szFileName, ".") && strcmp(pInfo->szFileName, ".."))
		{
			// Directory not . or .., NORMAL Dir.
			HTREEITEM hCur = m_tree.InsertItem(CString(pInfo->szFileName), m_hTreeSelected, TVI_LAST);
			//TRACE("Insert Directory:%s\n", pInfo->szFileName);
			m_tree.InsertItem(L"", hCur, TVI_LAST);
		}
	}
	else {
		m_tree.Expand(m_hTreeSelected, TVE_EXPAND);
		//m_tree.UpdateData();
	}
}

void CRemoteClientDlg::DownloadFile(CPacket& response, LPARAM lParam)
{
	static LONGLONG length = 0, nCount = 0;
	FILE* pFile = (FILE*)lParam;
	if (length == 0)
	{
		length = *(long long*)(response.strData.c_str());
		if (length == 0)
		{
			AfxMessageBox(L"File size is Zero or Download File Failed");
			CClientController::GetInstance()->DownloadEnd();
		}
		// else first length packet, file size > 0
		//TRACE("Total bytes:[%lld]\n", length);
	}
	else if(nCount < length)
	{
		fwrite(response.strData.c_str(), 1, response.strData.size(), pFile);
		nCount += response.strData.size();
		//TRACE("Received bytes:[%lld], total bytes:[%lld]\n", nCount, length);
	}
	if (nCount >= length)
	{
		fclose(pFile);
		length = 0;
		nCount = 0;
		CClientController::GetInstance()->DownloadEnd();
	}
}

void CRemoteClientDlg::InitUiData()
{
	UpdateData();
	m_port = _T("20000");
	// 192.169.2.123:0xC0A8027B, 127.0.0.1:0x7F000001 If using virtual box, set network mode to bridge.
	m_server_address = 0xC0A8027B;
	UpdateData(FALSE);
	CClientController::GetInstance()->UpdateAddress(m_server_address, static_cast<USHORT>(atoi(CW2A(m_port))));
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_hTreeSelected = NULL;
}

void CRemoteClientDlg::Response(CPacket &response, LPARAM lParam)
{
	switch (response.sCmd)
	{
		case CMD_DRIVER: GetDrivers(response); break;
		case CMD_DIR: GetFile(response); break;
		case CMD_DLD_FILE: DownloadFile(response, lParam); break; // Assign to a download thread, don't block the main dlg thread
		case CMD_DEL_FILE: LoadFiles(); break;
		case CMD_RUN_FILE: break; // Don't need response
		default: break;
	}
}

/**
* @wParam: response CPacket datak
* @lParam: hSelected handle
*/
LRESULT CRemoteClientDlg::OnSendPacketAck(WPARAM wParm, LPARAM lParam)
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
		CPacket response;
		if (pPacket)
		{
			// TODO change pointer to unique pointer or shared pointer
			response = *pPacket;
			delete pPacket;
			Response(response, lParam);
		}
	}
	return 0;
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	*pResult = 0;
	// default TRUE: push value from componets to m_var, FALSE: pull value from m_var to components
	UpdateData();
	CClientController::GetInstance()->UpdateAddress(m_server_address, static_cast<USHORT>(atoi(CW2A(m_port))));
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	UpdateData();
	CClientController::GetInstance()->UpdateAddress(m_server_address, static_cast<USHORT>(atoi(CW2A(m_port))));
}
