#pragma once
#include "ClientSocket.h"
#include "WatchDlg.h"
#include "StatusDlg.h"
#include "RemoteClientDlg.h"
#include <map>

#define WM_SEND_STATUS (WM_USER + 3)
#define WM_SEND_WATCH (WM_USER + 4)
#define WM_SEND_MESSAGE (WM_USER + 0x1000)

class CClientController
{
public:
	static CClientController* GetInstance();
	int InitController();
	int Invoke(CWnd** pMainWnd);
	int DealCommand();
	int GetImage(CImage& img);
	int DownloadFile(CString strPath);
	void CloseSocket();
	void DownloadEnd();
	void StartWatchScreen();
	void UpdateAddress(ULONG nIp, USHORT nPort);
	BOOL SendCommandPacket(HWND hWnd, int nCmd, BYTE* pData = NULL,
						  size_t nLength = 0, BOOL bAutoClose = TRUE, WPARAM wParam = 0);
protected:
	CClientController();
	~CClientController();
	static UINT __stdcall ThreadEntryMessageLoop(void* arg);
	static void __stdcall ThreadWatchScreenEntry(void* arg);
	static void ReleaseInstance();
	void MessageLoop();
	void ThreadWatchScreen();
#if 0
	LRESULT OnShowtatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam);
#endif
private:
	class CHelper {
	public:
		CHelper();
		~CHelper();
	};
	typedef struct MsgInfo {
		MsgInfo(MSG m);
		MsgInfo(const MsgInfo& m);
		struct MsgInfo& operator=(const MsgInfo& m);
		MSG msg;
		LRESULT result;
	} MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static CClientController* m_instance;
	static std::map<UINT, MSGFUNC> m_mapFunc;
	CRemoteClientDlg *m_clientDlg;
	CWatchDlg m_watchDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadWatch;
	UINT m_tid;	// message loop thread id
	static CHelper m_helper;
	char *m_localPath;
	char *m_remotePath;
	BOOL m_isWatchDlgClosed;
	std::atomic<BOOL> m_aRunning;
};

