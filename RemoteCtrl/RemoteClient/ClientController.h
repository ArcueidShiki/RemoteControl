#pragma once
#include "ClientSocket.h"
#include "WatchDlg.h"
#include "StatusDlg.h"
#include "RemoteClientDlg.h"
#include "Resource.h"
#include <map>

#define WM_SEND_PACKET (WM_USER + 1)
#define WM_SEND_DATA (WM_USER + 2)
#define WM_SEND_STATUS (WM_USER + 3)
#define WM_SEND_WATCH (WM_USER + 4)
#define WM_SEND_MESSAGE (WM_USER + 0x1000)

class CClientController
{
public:
	static CClientController* GetInstance();
	int InitController();
	int Invoke(CWnd** pMainWnd);
	LRESULT SendMsg(MSG msg);
	void UpdateAddress(ULONG nIp, USHORT nPort);
	int DealCommand();
	void CloseSocket();
	BOOL SendPacket(const CPacket& packet);
	int SendCommandPacket(int nCmd, BOOL bAutoclose = TRUE, BYTE* pData = NULL, size_t nLength = 0);
	int GetImage(CImage& img);
protected:
	static void ReleaseInstance();
	CClientController();
	~CClientController();
	void ThreadFunc();
	static UINT __stdcall ThreadEntry(void* arg);
	LRESULT OnSendPacket(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowtatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam);
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
	CWatchDlg m_watchDlg;
	CRemoteClientDlg m_clientDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	UINT m_tid;
	static CHelper m_helper;
};

