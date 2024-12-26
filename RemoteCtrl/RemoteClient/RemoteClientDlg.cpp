
// RemoteClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
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
END_MESSAGE_MAP()


// CRemoteClientDlg message handlers

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

	UpdateData();
	m_port = _T("10000");
	m_server_address = 0x7F000001; // 127.0.0.1
	UpdateData(FALSE);

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
	SendCommandPacket(CMD_DRIVER);
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	if (SendCommandPacket(CMD_DRIVER) == -1)
	{
		AfxMessageBox(_T("Send Command Packet Failed"));
		return;
	}
	CClientSocket* pClient = CClientSocket::GetInstance();
	std::string drivers = pClient->GetPacket().strData;
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

int CRemoteClientDlg::SendCommandPacket(int nCmd, BOOL autoclose, BYTE* pData, size_t nLength)
{
	// default TRUE: push value from componets to m_var, FALSE: pull value from m_var to components
	UpdateData();
	CClientSocket* pClient = CClientSocket::GetInstance();
	if (!pClient)
	{
		TRACE("pClient is Null");
		return -1;
	}
	USHORT port = static_cast<USHORT>(atoi(CW2A(m_port)));
	//TRACE("Client Socket Instance Success: ip:%08X, port:%d\r\n", m_server_address, port);
	if (!pClient->InitSocket(m_server_address, port))
	{
		TRACE("Client Init Socket Failed");
		return -1;
	}

	CPacket packet(nCmd, pData, nLength);
	if (!pClient->Send(packet))
	{
		TRACE("Client Send Paccket Failed\r\n");
		return -1;
	}
	//TRACE("Client Send Packet Success\r\n");
	// get return msg from server
	int ret = pClient->DealCommand();
	if (ret == -1)
	{
		TRACE("Get Return msg from server failed: ret = %d\r\n", ret);
		return -1;
	}
	if (autoclose)
	{
		pClient->CloseSocket();
	}
	return nCmd;
}

/**
* Return the full path of clicked tree item.
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

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse); // global point to screen
	m_tree.ScreenToClient(&ptMouse); // screen to client
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
	{
		TRACE("No Item Selected\n");
		return;
	}
	if (m_tree.GetChildItem(hTreeSelected) == NULL)
	{
		return; // no child, no directory, this is a file clicked
	}
	DeleteTreeChildrenItem(hTreeSelected);
	m_list.DeleteAllItems();
	auto pClient = CClientSocket::GetInstance();
	CString strPath = GetPath(hTreeSelected);
	CT2A asciiPath(strPath);
	const char* path = asciiPath;
	int strLen = strlen(asciiPath);
	TRACE("strPath: %s Length: %d\n", path, strLen);
	int nCmd = SendCommandPacket(CMD_DIR, FALSE, (BYTE*)path, strLen);
	PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	int id = 0;
	while (pInfo->HasNext)
	{
		//TRACE("Client receive file id: [%d], name: [%s] Has Next: [%d] \n", ++id, pInfo->szFileName, pInfo->HasNext);
		if (!pInfo->IsDirectory)
		{
			// FILE
			//TRACE("Insert file:%s\n", pInfo->szFileName);
			m_list.InsertItem(0, CString(pInfo->szFileName));
		}
		else if (strcmp(pInfo->szFileName, ".") && strcmp(pInfo->szFileName, ".."))
		{
			// Directory not . or .., NORMAL Dir.
			HTREEITEM hCur = m_tree.InsertItem(CString(pInfo->szFileName), hTreeSelected, TVI_LAST);
			//TRACE("Insert Directory:%s\n", pInfo->szFileName);
			m_tree.InsertItem(L"", hCur, TVI_LAST);
		}
		int cmd = pClient->DealCommand();
		if (cmd < 0) break;
		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
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
