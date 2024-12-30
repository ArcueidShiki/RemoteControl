
// RemoteClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDlg.h"

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
	pClient = CClientSocket::GetInstance();
	m_isFull = FALSE;
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
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
END_MESSAGE_MAP()


// CRemoteClientDlg message handlers

BOOL CRemoteClientDlg::isFull() const
{
	return m_isFull;
}

CImage& CRemoteClientDlg::GetImage()
{
	return m_img;
}

void CRemoteClientDlg::SetImageStatus(BOOL isFull)
{
	m_isFull = isFull;
}

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
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
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

void CRemoteClientDlg::LoadDirectory()
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
	CString strPath = GetPath(hTreeSelected);
	CT2A asciiPath(strPath);
	const char* path = asciiPath;
	size_t strLen = strlen(asciiPath);
	TRACE("strPath: %s Length: %zu\n", path, strLen);
	int nCmd = SendCommandPacket(CMD_DIR, FALSE, (BYTE*)path, strLen);
	PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	int id = 0;
	while (pInfo->HasNext)
	{
		TRACE("Client receive file id: [%d], name: [%s] Has Next: [%d] \n", ++id, pInfo->szFileName, pInfo->HasNext);
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
		int cmd = pClient->DealCommand();
		TRACE("Client Deal Command [ACK]: [%d]\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	}
}

void CRemoteClientDlg::ThreadEntryDownloadFile(void* arg)
{
	CRemoteClientDlg* pThis = (CRemoteClientDlg*)arg;
	pThis->ThreadDownloadFile();
	_endthread();
}

void CRemoteClientDlg::ThreadEntryWatchData(void* arg)
{
	CRemoteClientDlg* pThis = (CRemoteClientDlg*)arg;
	pThis->ThreadWatchData();
	// Thread might not end.
	_endthread();
}

void CRemoteClientDlg::ThreadDownloadFile()
{
	int nListSelected = m_list.GetSelectionMark();
	CString strFile = m_list.GetItemText(nListSelected, 0);
	// Need a dlg;
	// False for Save sa, "extension", "filename", flags, filter, parent
	CFileDialog dlg(FALSE, L"*", strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	if (dlg.DoModal() == IDOK) // need handle first, cannot move to other window
	{
		FILE* pFile;
		CT2A DPath(dlg.GetPathName());
		char* downloadPath = DPath;
		int err = fopen_s(&pFile, downloadPath, "wb+");
		if (err != 0 || pFile == NULL)
		{
			AfxMessageBox(L"Open Dlg File Failed\n");
			TRACE("Open Download Path : [%s] Failed\n", downloadPath);
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		HTREEITEM hTreeSelected = m_tree.GetSelectedItem();
		if (hTreeSelected == NULL)
		{
			AfxMessageBox(L"No Tree Item Selected\n");
			TRACE("Download File Failed, No Item Selected\n");
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		CW2A asciiFullPath(GetPath(hTreeSelected) + strFile);
		char* filepath = asciiFullPath;
		size_t len = strlen(filepath);
		TRACE("Full File Path : [%s], name len:[%zu]\n", filepath, len);
		LRESULT ret = SendMessage(WM_SEND_PACKET, CMD_DLD_FILE << 1, (LPARAM)filepath);
		//int ret = SendCommandPacket(CMD_DLD_FILE, FALSE, (BYTE*)filepath, len);
		if (ret < 0)
		{
			AfxMessageBox(L"Donwload File Failed");
			TRACE("Download File Failed ret = %d\n", ret);
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		auto nLength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nLength == 0)
		{
			AfxMessageBox(L"File size is Zero or Download File Failed");
			TRACE("Download File Failed, File Size = 0\n");
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		TRACE("Client Filename:[%s], size:[%lld]\n", filepath, nLength);
		long long nCount = 0;
		while (nCount < nLength)
		{
			ret = pClient->DealCommand();
			if (ret < 0)
			{
				AfxMessageBox(L"Transmission Failed\n");
				TRACE("Download File Failed, Deal Command Failed ret = %d\n", ret);
				m_dlgStatus.ShowWindow(SW_HIDE);
				EndWaitCursor();
				break;
			}
			std::string data = pClient->GetPacket().strData;
			nCount += data.size();
			fwrite(data.c_str(), 1, data.size(), pFile);
		}
		fclose(pFile);
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("Download File Success"), _T("Download Finish"));
}

/**
* Receiving data from server
*/
void CRemoteClientDlg::ThreadWatchData()
{
	Sleep(50);
	do {
		pClient = CClientSocket::GetInstance();
	} while (!pClient);
	while (!m_isClosed)
	{
		if (m_isFull)
		{
			Sleep(1);
			continue;
		}
		LRESULT ret = SendMessage(WM_SEND_PACKET, CMD_SEND_SCREEN << 1, 0);
		if (ret == CMD_SEND_SCREEN)
		{
			if (!m_isFull)
			{
				// update data ti cache
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL)
				{
					TRACE("Insufficient Memory GlobalAlloc failed\n");
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK)
				{
					ULONG written = 0;
					ULONG len = (ULONG)pClient->GetPacket().strData.size();
					pStream->Write(pData, len, &written);
					LARGE_INTEGER li = { 0 };
					pStream->Seek(li, STREAM_SEEK_SET, NULL);
					if ((HBITMAP)m_img != NULL) m_img.Destroy();
					m_img.Load(pStream);
					m_isFull = TRUE;
				}
			}
		}
		else
		{
			Sleep(1);
		}
	}
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
	_beginthread(CRemoteClientDlg::ThreadEntryDownloadFile, 0, this);
	BeginWaitCursor(); // set cursort to circle represent waiting...
	m_dlgStatus.m_info.SetWindowText(L"Downloading...");
	m_dlgStatus.ShowWindow(SW_SHOW); // active window accept keyboard input
	m_dlgStatus.CenterWindow();
	m_dlgStatus.SetActiveWindow();
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
	int ret = SendCommandPacket(CMD_DEL_FILE, TRUE, (BYTE*)fullpath, strlen(fullpath));
	if (ret < 0)
	{
		AfxMessageBox(L"Delete File Failed");
		TRACE("Delete File Failed, ret = %d\n", ret);
	}
	LoadFiles();
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
	int ret = SendCommandPacket(CMD_RUN_FILE, TRUE, (BYTE*)fullpath, strlen(fullpath));
	if (ret < 0)
	{
		AfxMessageBox(L"Open File Failed");
		TRACE("Open File Failed, ret = %d\n", ret);
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = int(wParam >> 1);
	BOOL autoclose = wParam & 1;
	switch (cmd)
	{
	case CMD_DLD_FILE:
	{
		LPCSTR filepath = (LPCSTR)lParam;
		ret = SendCommandPacket(cmd, autoclose, (BYTE*)lParam, strlen(filepath));
		break;
	}
	case CMD_MOUSE:
	{
		ret = SendCommandPacket(cmd, autoclose, (BYTE*)lParam, sizeof(MOUSEEV));
		break;
	}
	case CMD_SEND_SCREEN:
	case CMD_LOCK_MACHINE:
	case CMD_UNLOCK_MACHINE:
	{
		ret = SendCommandPacket(cmd, autoclose);
		break;
	}
	default:
		ret = -1;
	}
	return ret;
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	m_isClosed = FALSE;
	CWatchDlg dlg(this);
	HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::ThreadEntryWatchData, 0, this);
	//GetDlgItem(IDC_BTN_START_WATCH)->EnableWindow(FALSE);
	dlg.DoModal();
	m_isClosed = TRUE;
	WaitForSingleObject(hThread, 500);
}
