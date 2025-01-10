#pragma once
#define REG_SUBKEY _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")
#define REG_VALUE "%SystemRoot%\\system32\\RemoteControlServer.exe"
#define REG_KEY_NAME "RemoteCtrl"
#define USER_PATH "C:\\Users\\"
#define STARTUP_DIR_PATH "\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteControlServer.exe"
#define LINK_NAME "RemoteControlServer.exe"
#define PROGRAM_NAME "RemoteCtrl.exe"
#define WARNING_MSG _T("This program is only allow legal usage!\n"									\
					"This machine will be under survelience if continue using this program!\n"		\
					"Click Cancel Button to exit, if you don't want to\n"							\
					"Click Yes, this program will copy to your mahine, and startup when booting\n"	\
					"Click No, this program will only run once, it won't left anything on your system")
class CUtils
{
public:
    static void Dump(BYTE* pData, size_t nSize);
	static void ShowError();
	static BOOL IsAdmin();
	static BOOL RunAsAdmin();
	static BOOL MakeLink();
	static BOOL WriteStartupDir(const char *who);
	static BOOL WriteRegistryTable();
	static BOOL ChooseBootStartUp(const char* who);
	static BOOL Init();
};

