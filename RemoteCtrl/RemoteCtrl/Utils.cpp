#include "pch.h"
#include "Utils.h"

void CUtils::Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += '\n';
        snprintf(buf, sizeof(buf), "%02X ", pData[i]);
        strOut += buf;
    }
    strOut += '\n';
    OutputDebugStringA(strOut.c_str());
}

void CUtils::ShowError()
{
    LPSTR lpMessageBuf = NULL;
    // strerror(errno); // standard C library
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, GetLastError(), // GetLastError is thread related
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMessageBuf, 0, NULL);
    OutputDebugStringA(lpMessageBuf);
    MessageBoxA(NULL, lpMessageBuf, "Error", MB_OK | MB_ICONERROR);
    LocalFree(lpMessageBuf);
}

BOOL CUtils::RunAsAdmin()
{
#if 0
    HANDLE hToken = NULL;
    // cmd->secpol.msc->Local Policies->Security Options->
    // Accounts: Administrator account status -> Enable
    // Accounts: Limit local account use of blank passwords to console logon only -> Disable
    // win + x -> cmd(admin) -> net user Administrator *: set password to blank
    // LOGON32_LOGON_BATCH
    if (!LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken))
    {
        ShowError();
        MessageBox(NULL, _T("Logon administrator Failed!"), NULL, MB_ICONERROR | MB_TOPMOST);
        CloseHandle(hToken);
        ::exit(0);
    }
    //MessageBox(NULL, _T("Logon Administrator Success"), NULL, MB_ICONINFORMATION | MB_TOPMOST);
#endif
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    TCHAR sPath[MAX_PATH] = _T("");
    GetModuleFileName(NULL, sPath, MAX_PATH);
    if (!CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi))
    {
        ShowError(); // A required privilege is not held by the client
        MessageBox(NULL, _T("Create process failed"), NULL, MB_ICONERROR | MB_TOPMOST);
        //CloseHandle(hToken);
        return FALSE;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    MessageBox(NULL, _T("Run as administrator success!"), _T("Permission Granted"), MB_ICONINFORMATION | MB_TOPMOST);
    return TRUE;
    //CloseHandle(hToken);
}

BOOL CUtils::IsAdmin()
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        ShowError();
        CloseHandle(hToken);
        return FALSE;
    }
    TOKEN_ELEVATION elevation;
    memset(&elevation, 0, sizeof(elevation));
    DWORD len = 0;
    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &len))
    {
        ShowError();
        CloseHandle(hToken);
        return FALSE;
    }
    CloseHandle(hToken);
    if (len == sizeof(elevation))
    {
        return elevation.TokenIsElevated;
    }
    TRACE("Length of token information is %d\r\n", len);
    return FALSE;
}

BOOL CUtils::MakeLink()
{
    // %Windows/System32%
    char sSysPath[MAX_PATH] = "";
    GetSystemDirectoryA(sSysPath, MAX_PATH);
    strcat_s(sSysPath, LINK_NAME);
    if (PathFileExistsA(sSysPath))
    {
        MessageBox(NULL, L"RemoteControlServer.exe already exists in system directory", NULL, MB_ICONERROR | MB_TOPMOST);
        return TRUE;
    }

    char sPath[MAX_PATH] = "";
    GetCurrentDirectoryA(MAX_PATH, sPath);
    strcat_s(sPath, PROGRAM_NAME);

    char sLinkCmd[MAX_PATH * 2] = "MKLINK ";
    strcat_s(sLinkCmd, sSysPath);
    strcat_s(sLinkCmd, " ");
    strcat_s(sLinkCmd, sPath);
    TRACE("Link Command: [%s]\n", sLinkCmd);
    if (system(sLinkCmd) != 0)
    {
		MessageBox(NULL, _T("Make link failed. Don't have enough permission?\r\n Program start failed!"),
			_T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
		return FALSE;
    }
    return TRUE;
}

BOOL CUtils::WriteRegistryTable()
{
    CString strSubKey = REG_SUBKEY;
    HKEY hKey = NULL;
    // KEY_ALL_ACCESS | KEY_WOW64_64KEY
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("Open Registry key failed. Don't have enough permission?\r\n Program start failed!"),
            _T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
        return FALSE;
    }
    // using REG_EXPAND_SZ auto expand %path%
    char sExePath[MAX_PATH] = REG_VALUE;
    TRACE("sExePath Path: [%s]\n", sExePath);
    // RegSetValueExW(hKey, "RemoteCtrl", 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)sExePath, sExePath.GetLength() * sizeof(THAR))
    if (RegSetValueExA(hKey, REG_KEY_NAME, 0, REG_EXPAND_SZ, (BYTE*)sExePath, (DWORD)strlen(sExePath)) != ERROR_SUCCESS)
    {
        MessageBox(NULL, _T("Set Registry key failed. Don't have enough permission?\r\n Program start failed!"),
            _T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
    }
    RegCloseKey(hKey);
    return TRUE;
}

BOOL CUtils::WriteStartupDir()
{
    DWORD nSize = MAX_PATH;
    char sUser[MAX_PATH] = "";
    GetUserNameA(sUser, &nSize);
    TRACE("User Name: [%s]\n", sUser);
    // startup dir path
    char sStarupDir[MAX_PATH] = USER_PATH;
    strcat_s(sStarupDir, sUser);
    // this path need run as administrator
    strcat_s(sStarupDir, STARTUP_DIR_PATH);
    if (PathFileExistsA(sStarupDir))
    {
        TRACE("RemoteControlServer.exe already exists in startup directory, Override it\n");
    }
    // get program startup cmdline
    LPSTR sCmd = GetCommandLineA(); //it will contain extra "" which unncessary
    //char sPath[MAX_PATH] = "";
    //GetCurrentDirectoryA(MAX_PATH, sPath);
    //strcat_s(sPath, "\\RemoteCtrl.exe"); TCHAR
	//GetModuleNameA(NULL, sPath, MAX_PATH);

    TRACE("Program name: [%s], dst name:[%s]\n", sCmd, sStarupDir);
    // other choices: fopen, CFile, system(copy), OpenFile, mklin, create short cut.
    if (!CopyFileA(sCmd, sStarupDir, FALSE))
    {
        MessageBox(NULL, _T("Copy file to startup menu failed. Don't have enough permission?\r\n Program start failed!"),
            _T("Startup Failed"), MB_ICONERROR | MB_TOPMOST);
        return FALSE;
    }
    return TRUE;
}

/**
* Two ways:
* 1. Modify registry to set startup when booting : during logging
* 2. Add boot statrup option: win + r -> shell:startup -> create shortcut or copy file. : after logged in
* Configuration of Properties -> Advanced -> Use of MFC - > Use MFC in Static Library
* Linker->Manifest File->UAC Execution Level->requireAdministrator / asInvoker
* Computer\\HKEY_LOCAL_MACHINE\\SOFTWARE\Microsoft\\Windows\\CurrentVersion\\Run For boot startup
* your previldge is following invoker, if permission are not the same, it may cause some errors
* boot startup will affects env variables, dependent dll(system32(64bit program)) sysmWow64(32bit program)
* using static library
*/
BOOL CUtils::ChooseBootStartUp()
{
    CString warning = WARNING_MSG;
    int ret = MessageBox(NULL, warning, _T("Warnning"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
        if (!MakeLink())
        {
            return FALSE;
        }
        if (!WriteStartupDir() && !WriteRegistryTable())
        {
            return FALSE;
        }
    }
    else if (ret == IDCANCEL)
    {
        return FALSE;
    }
    return TRUE;
}

BOOL CUtils::Init()
{
    HMODULE hModule = ::GetModuleHandle(nullptr);
    if (!hModule)
    {
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        return FALSE;
    }
    // initialize MFC and print and error on failure
    if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
    {
        // TODO: code your application's behavior here.
        wprintf(L"Fatal Error: MFC initialization failed\n");
        return FALSE;
    }
}