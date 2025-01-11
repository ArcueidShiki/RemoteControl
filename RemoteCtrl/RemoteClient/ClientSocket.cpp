#include "pch.h"
#include "ClientSocket.h"

constexpr int BUF_SIZE = 4096000;
#define WM_SEND_PACKET (WM_USER + 1)
#define WM_SEND_PACKET_ACK (WM_USER + 2)

//define and init static member outside class
CClientSocket* CClientSocket::m_instance = NULL;
// help trigger delete deconstructor
CClientSocket::CHelper CClientSocket::m_helper;

enum {
	CSM_AUTOCLOSE = 1, // Client Socket Mode
};

CClientSocket::CClientSocket()
	: m_socket(INVALID_SOCKET)
	, m_nIp(INADDR_ANY)
	, m_nPort(0)
	, m_isAutoClose(TRUE)
	, m_nTid(UINT(-1))
	, m_aRunning(FALSE)
{
	if (!InitSocketEnv())
	{
		MessageBox(NULL, _T("Cannot Initiate socket environment, please check network setting!"), _T("Soccket Initialization Error!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_buf.resize(BUF_SIZE);
	memset(m_buf.data(), 0, BUF_SIZE);
	struct { 
		UINT msg;
		MSGFUNC handler;
	} msgHandlers[] =
	{
		{WM_SEND_PACKET, &CClientSocket::SendPacket},
	};
	size_t size = sizeof(msgHandlers) / sizeof(msgHandlers[0]);
	for (int i = 0; i < size; i++)
	{
		if (!m_mapMsgHandlers.insert(std::pair<UINT, MSGFUNC>(msgHandlers[i].msg,
									msgHandlers[i].handler)).second)
		{
			TRACE("Insert MsgHandler Failed\n");
		}
	}
	// attr, manual reset, initial state, name
	m_hEeventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::ThreadEntryMessageLoop, this, 0, &m_nTid);
	if (m_hEeventInvoke)
	{
		if (WaitForSingleObject(m_hEeventInvoke, 100) == WAIT_TIMEOUT)
		{
			TRACE("Socket Message loop thread start failed\n");
		}
		CloseHandle(m_hEeventInvoke); // reduce reference count
	}
	else
	{
		TRACE("Socket create event failed\n");
	}
}

CClientSocket::CClientSocket(const CClientSocket& other)
{ 
	m_helper = other.m_helper;
	m_instance = other.m_instance;
	m_socket = other.m_socket;
	m_packet = other.m_packet;
	m_buf = other.m_buf;
	m_nIp = other.m_nIp;
	m_nPort = other.m_nPort;
	m_isAutoClose = other.m_isAutoClose;
	m_mapAck = other.m_mapAck;
	m_mapAutoClose = other.m_mapAutoClose;
	m_queueSend = other.m_queueSend;
	m_hThread = other.m_hThread;
	m_mapMsgHandlers = other.m_mapMsgHandlers;
	m_nTid = other.m_nTid;
	m_hEeventInvoke = other.m_hEeventInvoke;
	m_aRunning.store(other.m_aRunning.load());
}

CClientSocket& CClientSocket::operator=(const CClientSocket& other)
{
	if (this != &other)
	{
		m_helper = other.m_helper;
		m_instance = other.m_instance;
		m_socket = other.m_socket;
		m_packet = other.m_packet;
		m_buf = other.m_buf;
		m_nIp = other.m_nIp;
		m_nPort = other.m_nPort;
		m_isAutoClose = other.m_isAutoClose;
		m_mapAck = other.m_mapAck;
		m_mapAutoClose = other.m_mapAutoClose;
		m_queueSend = other.m_queueSend;
		m_hThread = other.m_hThread;
		m_mapMsgHandlers = other.m_mapMsgHandlers;
		m_nTid = other.m_nTid;
		m_hEeventInvoke = other.m_hEeventInvoke;
		m_aRunning.store(other.m_aRunning.load());
	}
	return *this;
}

CClientSocket::~CClientSocket()
{
	m_aRunning.store(FALSE);
	TerminateThread(m_hThread, 0);

	//if (WaitForSingleObject(m_hThread, 500) != WAIT_OBJECT_0)
	//{
	//	TerminateThread(m_hThread, 0);
	//}
	m_hThread = INVALID_HANDLE_VALUE;
	closesocket(m_socket);
	WSACleanup();
}

CClientSocket* CClientSocket::GetInstance()
{
	if (m_instance == NULL)
	{
		// need explicitly delete to trigger deconstructor
		m_instance = new CClientSocket();
	}
	return m_instance;
}

UINT __stdcall CClientSocket::ThreadEntryMessageLoop(void* arg)
{
	CClientSocket* self = (CClientSocket*)arg;
	self->ThreadMessageLoop();
	_endthreadex(0);
	return 0;
}

#if 0
void CClientSocket::ThreadFunc()
{
	std::string strBuf;
	strBuf.resize(BUF_SIZE);
	char* pBuf = const_cast<char *>(strBuf.c_str());
	int index = 0;
	InitSocket();
	while (m_socket != INVALID_SOCKET)
	{
		if (m_queueSend.size() > 0)
		{
			m_mutex.lock();
			CPacket& head = m_queueSend.front();
			m_mutex.unlock();
			if (!Send(head))
			{
				TRACE("Send Packet Failed\n");
				continue;
			}
			// TODO it null check
			auto it = m_mapAck.find(head.hEvent);
			if (it == m_mapAck.end())
			{
				continue;
			}
			BOOL autoClose = m_mapAutoClose[head.hEvent];
			do {
				int n_recv = recv(m_socket, pBuf + index, BUF_SIZE - index, 0);
				if (n_recv > 0 || index > 0)
				{
					if (n_recv > 0) index += n_recv;
					size_t n_parsed = index;
					CPacket packet((BYTE*)pBuf, n_parsed);
					if (int(n_parsed) > 0)
					{
						// Set specific event to signaled state, Allows any threads that are waiting on
						// the event to be release and continue execution, kind like condition variable
						packet.hEvent = head.hEvent;
						it->second->emplace_back(std::move(packet));
						// receive one packet over.
						if (autoClose)
						{
							CloseSocket();
							strBuf.clear();
							index = 0;
							break;
						}
						// continue receiving
						char* unparse_pos = pBuf + n_parsed;
						int n_unparse = int(index - n_parsed);
						if (n_unparse >= 0)
						{
							memmove(pBuf, unparse_pos, n_unparse);
							index -= (int)n_parsed;
						}
						else
						{
							TRACE("Recv 0, but still can parse Packet out, has leftover in buffer, index:[%d] < n_parsed:[%d]\n", index, n_parsed);
						}
					}
				}
				else {
					CloseSocket();
					strBuf.clear();
					index = 0;
					break;
				}
			} while (!autoClose);
			InitSocket();
			m_mutex.lock();
			m_queueSend.pop();
			m_mutex.unlock();
		}
		else {
			Sleep(1);
		}
	}
	CloseSocket();
}
#endif

void CClientSocket::ThreadMessageLoop()
{
	SetEvent(m_hEeventInvoke);
	MSG msg;
	m_aRunning.store(TRUE);
	while (m_aRunning.load() && ::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapMsgHandlers.find(msg.message) != m_mapMsgHandlers.end())
		{
			(this->*m_mapMsgHandlers[msg.message])(msg.message, msg.wParam, msg.lParam);
		} 
	}
}

void CClientSocket::ReleaseInstance()
{
	if (m_instance != NULL)
	{
		delete m_instance;
		m_instance = NULL;
	}
}

/**
* @wParam: value of cache buf (on heap)
* @lParam: HWnd, len? this have problems
* TODO: define MSGINFO struct (data, len, mode)
*		define callback function(HWND, MSG)
*/
void CClientSocket::SendPacket(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (!InitSocket())
	{
		TRACE("Client Socket Invalid\n");
		return;
	}
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;	// ack back to dlg wnd
	int n_sent = send(m_socket, data.strData.c_str(), (int)data.strData.size(), 0);
	if (n_sent > 0)
	{
		int index = 0;
		std::string strBuf;
		strBuf.resize(BUF_SIZE);
		char* pBuf = const_cast<char*>(strBuf.c_str());
		while (m_socket != INVALID_SOCKET)
		{
			int n_recv = recv(m_socket, pBuf + index, BUF_SIZE - index, 0);
			if (n_recv > 0 || index > 0)
			{
				index += n_recv;
				size_t n_parsed = index;
				CPacket packet((BYTE*)pBuf, n_parsed);
				if ((int)n_parsed > 0)
				{
					::SendMessage(hWnd, WM_SEND_PACKET_ACK, (WPARAM)new CPacket(packet), data.wParam);
					if (data.nMode & CSM_AUTOCLOSE)
					{
						CloseSocket();
						return;
					}
					// not auto close, continue receiving
					char* unparse_pos = pBuf + n_parsed;
					int n_unparse = index - (int)n_parsed;
					if (n_unparse >= 0)
					{
						memmove(pBuf, unparse_pos, n_unparse);
						// after moving, the unused spacec for receiving data
						index = n_unparse;
					}
					else
					{
						CloseSocket();
						::SendMessage(hWnd, WM_SEND_PACKET_ACK, NULL, 1);
						TRACE("Server side close, Recv 0, but still can parse Packet out, has leftover in buffer, index:[%d] < n_parsed:[%d]\n", index, n_parsed);
					}
				}
			}
			else
			{
				CloseSocket();
				::SendMessage(hWnd, WM_SEND_PACKET_ACK, NULL, -1);
			}
		}
	}
	else
	{
		TRACE("Send Packet Failed\n");
		CloseSocket();
		::SendMessage(hWnd, WM_SEND_PACKET_ACK, NULL, -2);
	}
}

CClientSocket::CHelper::CHelper()
{
	CClientSocket::GetInstance();
}

CClientSocket::CHelper::~CHelper()
{
	CClientSocket::ReleaseInstance();
}

BOOL CClientSocket::InitSocketEnv()
{
	WSAData wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", GetLastError());
		return FALSE;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("LOBYTE HIBYTE errorNum = %d\n", GetLastError());
		WSACleanup();
		return -1;
	}
	return TRUE;
}

std::string GetErrorInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

BOOL CClientSocket::InitSocket()
{
	if (m_socket != INVALID_SOCKET)
	{
		CloseSocket(); // Close previous socket;
	}
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		MessageBox(NULL, _T("Cannot create socket, please check network setting!"), _T("Soccket Initialization Error!"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	SOCKADDR_IN serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.S_un.S_addr = htonl(m_nIp);
	serv_addr.sin_port = htons(m_nPort);	// default port
	if (serv_addr.sin_addr.S_un.S_addr == INADDR_NONE)
	{
		AfxMessageBox(_T("Invalid IP Address"));
		return FALSE;
	}
	if (connect(m_socket, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		AfxMessageBox(_T("Connect Failed"));
		TRACE("Connect Failed %d %s, ip:[%0X], port:[%d]\n",
				WSAGetLastError(),
				GetErrorInfo(WSAGetLastError()).c_str(),
				m_nIp, m_nPort);
		return FALSE;
	}
	return m_socket != INVALID_SOCKET;
}

int CClientSocket::DealCommand()
{
	if (m_socket == INVALID_SOCKET)
	{
		return -1;
	}
	// tcp no data border
	// desgin tcp package:
	// 1. dstinguish different package or leftover
	// 2. sniffer package, penetration
	// 1~2 byte: package head : FFFE/FEFE
	// 3~4 byte: package length
	// 5~n-2 byte: package data
	// n-1~n byte: package check: md5checksum, crc16, crc32
	// multithreads need to lock the buffer
	char* buf = m_buf.data();
	if (!buf)
	{
		TRACE("Allocate Memory failed\n");
		return -2;
	}
	// if the buffer has data left over remains to read, index is indicating the start point of receiving position
	static int index = 0;
	while (TRUE)
	{
		char* start = buf + index;
		int buf_available = BUF_SIZE - index;
		int n_recv = recv(m_socket, start, buf_available, 0);
		if (n_recv <= 0 && index <= 0)
		{
			// not receiving and used over.
			return -1;
		}
		// update new position
		if (n_recv > 0)
			index += n_recv;
		// update unread bytes len in buffer.
		size_t n_parsed = index;
		// n_parsed here is how many bytes are used for form a full packet
		// if the packet is incomplete to parse, the n_parsed will be set 0, going to next round of receiving
		m_packet = CPacket((BYTE*)buf, n_parsed);
		if (int(n_parsed) > 0)
		{
			// a complete packet parse success.
			// move unparse bytes leftover to head of buffer(parsed bytes),
			// overwrite the used len bytes buffer area
			char* unparse_pos = buf + n_parsed;
			int n_unparse = int(index - n_parsed);
			if (n_unparse >= 0)
			{
				memmove(buf, unparse_pos, n_unparse);
				// after moving, the unused spacec for receiving data
				index -= (int)n_parsed;
			}
			else
			{
				TRACE("Recv 0, but still can parse Packet out, has leftover in buffer, index:[%d] < n_parsed:[%d]\n", index, n_parsed);
			}
			return m_packet.sCmd;
		}
	}
	return -1;
}

BOOL CClientSocket::Send(const char* pData, size_t nSize)
{
	if (m_socket == INVALID_SOCKET) {
		TRACE("Client Socket Invalid\n");
		return FALSE;
	}
	return send(m_socket, pData, (int)nSize, 0) != SOCKET_ERROR;
}

BOOL CClientSocket::Send(const CPacket& packet)
{
	if (m_socket == INVALID_SOCKET) {
		TRACE("Client Socket Invalid\n");
		return FALSE;
	}
	std::string strOut;
	packet.GetData(strOut);
	return send(m_socket, strOut.c_str(), (int)strOut.size(), 0) != SOCKET_ERROR;
}

BOOL CClientSocket::GetFilePath(std::string& strPath)
{
	if (m_packet.sCmd == CMD_DIR || m_packet.sCmd == CMD_RUN_FILE
		|| m_packet.sCmd == CMD_DLD_FILE)
	{
		strPath = m_packet.strData;
		return TRUE;
	}
	return FALSE;
}

/**
* Mouse event: move, right click, left click, double click
*/
BOOL CClientSocket::GetMouseEvent(MOUSEEV& mouse)
{
	if (m_packet.sCmd == CMD_MOUSE)
	{
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
		return TRUE;
	}
	return FALSE;
}

void CClientSocket::CloseSocket()
{
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

void CClientSocket::UpdateAddress(ULONG nIp, USHORT nPort)
{
	m_nIp = nIp;
	m_nPort = nPort;
}

BOOL CClientSocket::SendPacket(HWND hWnd, const CPacket& packet, BOOL bAutoClose, WPARAM wParam)
{
	UINT nMode = bAutoClose ? CSM_AUTOCLOSE : 0;
	std::string data;
	packet.GetData(data);
	PACKET_DATA* pData = new PACKET_DATA(data.c_str(), data.size(), nMode, wParam);
	BOOL ret = PostThreadMessage(m_nTid, WM_SEND_PACKET, (WPARAM)pData, (LPARAM)hWnd);
	if (!ret)
	{
		delete pData;
	}
	return ret;
}