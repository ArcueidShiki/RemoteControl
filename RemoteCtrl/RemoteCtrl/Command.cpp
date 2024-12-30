#include "pch.h"
#include "Command.h"

CCommand::CCommand()
{
	struct {
		int nCmd;
		CMDFUNC func;
	} data[] = {
		{ CMD_DRIVER,& CCommand::MakeDriverInfo },
		{ CMD_DIR, &CCommand::MakeDirectoryInfo },
		{ CMD_RUN_FILE, &CCommand::RunFile },
		{ CMD_DLD_FILE, &CCommand::DownloadFile },
		{ CMD_DEL_FILE, &CCommand::DelFile },
		{ CMD_MOUSE, &CCommand::MouseEvent },
		{ CMD_SEND_SCREEN, &CCommand::SendScreen },
		{ CMD_LOCK_MACHINE, &CCommand::LockMachine },
		{ CMD_UNLOCK_MACHINE, &CCommand::UnlockMachine},
		{-1, NULL},
	};
	for (int i = 0; data[i].nCmd != -1; i++)
	{
		m_mapCmd.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
    tid = 0;
	pServer = CServerSocket::GetInstance();
}

CCommand::~CCommand()
{
}

void CCommand::Init()
{
}

/**
* Lookup files, need driver partitions
*/
int CCommand::MakeDriverInfo()
{
    std::string result;
    // 1 == A, 2 == B, 3 == C, 26 == Z
    for (int i = 1; i <= 26; i++)
    {
        if (_chdrive(i) == 0)
        {
            if (result.size() > 0)
                result += ',';
            result += 'A' + i - 1;
        }
    }

    // sCmd, data "C,D,E"
    CPacket packet(CMD_DRIVER, (BYTE*)result.c_str(), result.size());
    // FF FE(head) 09 00 00 00(length = datasize + cmd+sum) 01 00(cnd) 43 2C 44 2C 45(C(43),D(44),E(45)) 24 01(01 24 = 43 + 2C + 44 + 2C + 45)
    CUtils::Dump((BYTE*)packet.Data(), packet.Size());

    pServer->Send(packet);
    return 0;
}

/**
* Lookup directories
*/
int CCommand::MakeDirectoryInfo()
{
    std::string strPath;
    if (!pServer->GetFilePath(strPath))
    {
        OutputDebugString(_T("GetFilePath failed, Current cmd is not get file list, parse failed!!!\n"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0)
    {
        FILEINFO finfo;
        finfo.IsDirectory = TRUE;
        finfo.IsValid = FALSE;
        finfo.HasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
        OutputDebugString(_T("No permission to access the directory\n"));
        CPacket packet(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
        pServer->Send(packet);
        return -2;
    }
    _finddata_t fdata;
    // If not sure the type, use auto
    intptr_t hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1)
    {
        OutputDebugString(_T("No files in the directory\n"));
        return -3;
    }
    int ret;
    int id = 0;
    do {
        FILEINFO finfo;
        finfo.IsDirectory = fdata.attrib & _A_SUBDIR;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        CPacket packet(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
        pServer->Send(packet); // NONBLOCK
        TRACE("Server Send filename id : [%d], name: [%s], HasNext: [%d]\r\n", ++id, fdata.name, finfo.HasNext);
        // it will blow up the buffer of client
        // using ack or sleep
        //Sleep(10);
        ret = _findnext(hfind, &fdata);
    } while (ret == 0);
    // when tmpfiles or logfiles, are huge, so send one file every time read.
    FILEINFO finfo;
    // tell client, end.
    finfo.HasNext = FALSE;
    CPacket packet(CMD_DIR, (BYTE*)&finfo, sizeof(finfo));
    pServer->Send(packet);
    _findclose(hfind);
    return 0;
}

int CCommand::RunFile()
{
    std::string strPath;
    pServer->GetFilePath(strPath);
    // open / run file.
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CPacket packet(CMD_RUN_FILE, NULL, 0);
    pServer->Send(packet);
    return 0;
}

int CCommand::DownloadFile()
{
#define DLD_BUF_SIZE 1024 // local scope macro, is not accessible globally.
    std::string strPath;
    pServer->GetFilePath(strPath);
    FILE* pFile = NULL;
    long long size = 0;
    // if you open first, then, other process open this in exclusive mode, you have the pointer, but cannot do anything.
    // Or property->C/C++->preprocessor->_CRT_SECURE_NO_WARNINGS
    // Or #pragma warning(disable:4996)
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0)
    {
        CPacket packet(CMD_DLD_FILE, (BYTE*)&size, sizeof(size));
        pServer->Send(packet);
        TRACE("Open File failed filename:[%s], name len: %zu\n", strPath.c_str(), strPath.size());
        return -1;
    }
    if (pFile)
    {

        fseek(pFile, 0, SEEK_END);
        size = _ftelli64(pFile);
        TRACE("Server Filename:[%s], size:[%lld]\n", strPath.c_str(), size);
        CPacket head(CMD_DLD_FILE, (BYTE*)&size, sizeof(size));
        pServer->Send(head);
        fseek(pFile, 0, SEEK_SET);
        char buf[DLD_BUF_SIZE] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buf, 1, DLD_BUF_SIZE, pFile);
            CPacket packet(CMD_DLD_FILE, (BYTE*)buf, rlen);
            pServer->Send(packet);
        } while (rlen >= DLD_BUF_SIZE);
        fclose(pFile);
    }
    return 0;
}

int CCommand::DelFile()
{
    std::string strPath;
    pServer->GetFilePath(strPath);
    TRACE("strPath: [%s]\n", strPath.c_str());
    WCHAR sPath[MAX_PATH] = _T("");
    size_t nConverted;
    // this fail,
    errno_t err = mbstowcs_s(&nConverted, sPath, MAX_PATH, strPath.c_str(), strPath.size());
    if (err != 0)
    {
        TRACE("mbstowcs_s failed, error code: %d, nConverted: %zu\n", err, nConverted);
        return -1;
    }
    if (!DeleteFile((LPCWSTR)sPath))
    {
        TRACE("DeleteFile failed, error code: %d\n", GetLastError());
    }

    TRACE("Converted path: %ws\n", sPath);
    memset(sPath, 0, MAX_PATH);
    // This success
    MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), (int)strPath.size(), sPath, MAX_PATH);
    if (!DeleteFileW(sPath))
    {
        TRACE("DeleteFileW failed, error code: %d\n", GetLastError());
    }
    DeleteFileA(strPath.c_str());
    CPacket packet(CMD_DEL_FILE, NULL, 0);
    BOOL ret = pServer->Send(packet);
    TRACE("Send ret = %d\n", ret);
    return 0;
}

int CCommand::MouseEvent()
{
    MOUSEEV mouse;
    if (pServer->GetMouseEvent(mouse))
    {
        DWORD operation = mouse.nButton;
        if (operation != MOUSE_MOVE)
            SetCursorPos(mouse.point.x, mouse.point.y);
        operation |= mouse.nAction;
        TRACE("Mouse operation: %08X, x: %d, y: %d\n", operation, mouse.point.x, mouse.point.y);
        switch (operation)
        {
        case MOUSE_LEFT | MOUSE_CLICK:
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_LEFT | MOUSE_DBCLICK:
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_LEFT | MOUSE_DOWN:
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_LEFT | MOUSE_UP:
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_RIGHT | MOUSE_CLICK:
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_RIGHT | MOUSE_DBCLICK:
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_RIGHT | MOUSE_DOWN:
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_RIGHT | MOUSE_UP:
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_MIDDLE | MOUSE_CLICK:
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_MIDDLE | MOUSE_DBCLICK:
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_MIDDLE | MOUSE_DOWN:
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_MIDDLE | MOUSE_UP:
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case MOUSE_MOVE:
            mouse_event(MOUSEEVENTF_MOVE, mouse.point.x, mouse.point.y, 0, GetMessageExtraInfo());
        }
        CPacket packet(CMD_MOUSE, NULL, 0);
        pServer->Send(packet);
    }
    else
    {
        OutputDebugString(_T("GetMouseEvent failed, Current cmd is not mouse event, parse failed!!!\n"));
        return -1;
    }
    return 0;
}

int CCommand::SendScreen()
{
    CImage screen; // GDI: Global Device Interface
    HDC hScreen = ::GetDC(NULL); // device context
    int bBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);// rgba 8888 32bit, rgb565 24 bit, rgb444
    int nWidth = GetDeviceCaps(hScreen, HORZRES);
    int nHeight = GetDeviceCaps(hScreen, VERTRES);
    screen.Create(nWidth, nHeight, bBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);
#if 0
    DWORD tick = GetTickCount64();
    screen.Save(_T("test2024.png"), Gdiplus::ImageFormatPNG);
    TRACE("PNG: %d\n", GetTickCount64() - tick);
    tick = GetTickCount64();
    screen.Save(_T("test2024.jpg"), Gdiplus::ImageFormatJPEG);
    TRACE("JPG: %d\n", GetTickCount64() - tick);
#endif
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL) return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (ret != S_OK)
    {
        screen.ReleaseDC();
        return -1;
    }
    screen.Save(pStream, Gdiplus::ImageFormatJPEG);
    LARGE_INTEGER li = { 0 };
    pStream->Seek(li, STREAM_SEEK_SET, NULL);
    PBYTE pData = (PBYTE)GlobalLock(hMem);
    SIZE_T nSize = GlobalSize(hMem);
    CPacket packet(CMD_SEND_SCREEN, pData, nSize);
    pServer->Send(packet);
    GlobalUnlock(hMem);
    pStream->Release();
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}

unsigned __stdcall CCommand::ThreadLockDlg(void* arg)
{
	CCommand* self = (CCommand*)arg;
    self->ThreadLockDlgMain();
    _endthreadex(0);
    return 0;
}

void CCommand::ThreadLockDlgMain()
{
    TRACE("%s (%d):%d\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    ShowCursor(FALSE);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
    // non modal dlg: can handle other window
    dlg.Create(IDD_DIALOG_INFO, NULL);
    dlg.ShowWindow(SW_SHOW);
    dlg.SetWindowPos(&dlg.wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXSCREEN); // same as GetDeviceCaps( hdcPrimaryMonitor, HORZRES)
    rect.bottom = LONG(GetSystemMetrics(SM_CYSCREEN) * 1.1); // same as GetDeviceCaps( hdcPrimaryMonitor, VERTRES)
    TRACE("right = %d, bottom = %d\n", rect.right, rect.bottom);
    dlg.MoveWindow(&rect);
    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
    if (pText)
    {
        CRect rtText;
        // you are actually passing a reference to the object. In C++, when you pass an object to a function that expects a pointer, the compiler automatically converts the object to a pointer to its first member. Since CRect inherits from RECT, this conversion is straightforward.
        pText->GetWindowRect(&rtText);
        int x = (rect.right - rtText.Width()) / 2;
        int y = (rect.bottom - rtText.Height()) / 2;
        pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
    }
    //dlg.GetWindowRect(&rect);
    ClipCursor(&rect); // restrict mouse move range.
    MSG msg;
    // bind to current thread, only can get message from this thread
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN)
        {
            TRACE("msg:%08X, wparam:%08X, lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == VK_ESCAPE) // ESC
            {
                break;
            }
        }
    }
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
    dlg.DestroyWindow();
    ClipCursor(NULL);
    ShowCursor(TRUE);
}

int CCommand::LockMachine()
{
    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE)
    {
        // create a thread to show dialog
        //_beginthread(ThreadLockDlg, 0, NULL);
        _beginthreadex(NULL, 0, &CCommand::ThreadLockDlg, this, 0, &tid);
        TRACE("Thread id = %u\n", tid);
    }
    CPacket packet(CMD_UNLOCK_MACHINE, NULL, 0);
    pServer->Send(packet);
    return 0;
}

int CCommand::UnlockMachine()
{
    // simulate press ESC
    PostThreadMessage(tid, WM_KEYDOWN, VK_ESCAPE, 0x00010001);
    // send back to client
    CPacket packet(CMD_UNLOCK_MACHINE, NULL, 0);
    pServer->Send(packet);
    return 0;
}

int CCommand::ExecuteCommand(int nCmd)
{
	std::map<int, CMDFUNC>::iterator it = m_mapCmd.find(nCmd);
	if (it == m_mapCmd.end())
	{
		return -1;
	}
	return (this->*(it->second))();
}