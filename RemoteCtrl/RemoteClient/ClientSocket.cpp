#include "pch.h"
#include "ClientSocket.h"

constexpr int BUF_SIZE = 4096000;

//define and init static member outside class
CClientSocket* CClientSocket::m_instance = NULL;
// help trigger delete deconstructor
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket::CClientSocket()
	: m_socket(INVALID_SOCKET)
	, m_nIp(INADDR_ANY)
	, m_nPort(0)
{
	if (!InitSocketEnv())
	{
		MessageBox(NULL, _T("Cannot Initiate socket environment, please check network setting!"), _T("Soccket Initialization Error!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_buf.resize(BUF_SIZE);
	memset(m_buf.data(), 0, BUF_SIZE);
}

CClientSocket::CClientSocket(const CClientSocket& other)
{ 
	m_socket = other.m_socket;
	m_nIp = other.m_nIp;
	m_nPort = other.m_nPort;
}

CClientSocket& CClientSocket::operator=(const CClientSocket& other)
{
	if (this != &other)
	{
		m_socket = other.m_socket;
		m_nIp = other.m_nIp;
		m_nPort = other.m_nPort;
	}
	return *this;
}

CClientSocket::~CClientSocket()
{
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

void CClientSocket::ReleaseInstance()
{
	if (m_instance != NULL)
	{
		delete m_instance;
		m_instance = NULL;
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
	WORD wVersionRequested = MAKEWORD(1, 1);
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", GetLastError());
		return FALSE;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
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
		TRACE("Connect Failed %d %s\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
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
		if (n_recv <= 0 && index == 0)
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
			memmove(buf, unparse_pos, n_unparse);
			// after moving, the unused spacec for receiving data
			index -= (int)n_parsed;
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
	packet.Data(strOut);
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
