#include "pch.h"
#include "ServerSocket.h"

//define and init static member outside class
CServerSocket* CServerSocket::m_instance = NULL;
// help trigger delete deconstructor
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket::CServerSocket()
	: m_client(INVALID_SOCKET)
	, m_socket(INVALID_SOCKET)
{
	if (!InitSocketEnv())
	{
		MessageBox(NULL, _T("Cannot Initiate socket environment, please check network setting!"), _T("Soccket Initialization Error!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	if (!InitSocket())
	{
		MessageBox(NULL, _T("Cannot Initiate socket, please check network setting!"), _T("Soccket Initialization Error!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	//MessageBox(NULL, _T("初始化套接字成功"), _T("1111111111"), MB_OK | MB_ICONERROR);
}

CServerSocket::CServerSocket(const CServerSocket& other)
{
	m_socket = other.m_socket;
	m_client = other.m_client;
}

CServerSocket& CServerSocket::operator=(const CServerSocket& other)
{
	if (this != &other)
	{
		m_socket = other.m_socket;
		m_client = other.m_client;
	}
	return *this;
}

CServerSocket::~CServerSocket()
{
	closesocket(m_socket);
	WSACleanup();
}

CServerSocket* CServerSocket::GetInstance()
{
	if (m_instance == NULL)
	{
		// need explicitly delete to trigger deconstructor
		m_instance = new CServerSocket();
	}
	return m_instance;
}

void CServerSocket::ReleaseInstance()
{
	if (m_instance != NULL)
	{
		delete m_instance;
		m_instance = NULL;
	}
}

CServerSocket::CHelper::CHelper()
{
	CServerSocket::GetInstance();
}

CServerSocket::CHelper::~CHelper()
{
	CServerSocket::ReleaseInstance();
}

BOOL CServerSocket::InitSocketEnv()
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

BOOL CServerSocket::InitSocket()
{
	if (m_socket != INVALID_SOCKET)
	{
		return TRUE;
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
	serv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(10000);
	if (bind(m_socket, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("Cannot bind socket, please check network setting!"), _T("Socket Bind Error!"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (listen(m_socket, 1) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("Cannot listen socket, please check network setting!"), _T("Socket Listen Error!"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	return m_socket != INVALID_SOCKET;
}

BOOL CServerSocket::AcceptClient()
{
	SOCKADDR_IN client_addr;
	int client_addr_size = sizeof(client_addr);
	m_client = accept(m_socket, (SOCKADDR*)&client_addr, &client_addr_size);
	if (m_client == INVALID_SOCKET)
	{
		TRACE("accept failed client: %zu, m_socket：%zu\n", m_client, m_socket);
		return FALSE;
	}
	return TRUE;
}

/**
@return cmd
*/
int CServerSocket::DealCommand()
{
	// tcp no data border
	// desgin tcp package:
	// 1. dstinguish different package or leftover
	// 2. sniffer package, penetration
	// 1~2 byte: package head : FFFE/FEFE
	// 3~4 byte: package length
	// 5~n-2 byte: package data
	// n-1~n byte: package check: md5checksum, crc16, crc32
	int BUF_SIZE = 4096;
	char* buf = new char[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	size_t index = 0;
	while (TRUE)
	{
		// if client disconnect, during recving msg.
		if (m_client == INVALID_SOCKET)
		{
			TRACE("Client is not connected, waiting for retry");
			while (!AcceptClient())
			{
				Sleep(1000);
			}
		}
		TRACE("Client conncted");
		size_t len = recv(m_client, buf + index, BUF_SIZE - index, 0);
		if (len < 0)
		{
			delete[] buf;
			return -1;
		}
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buf, len);
		if (len > 0)
		{
			// move unparse bytes leftover to head of buffer(parsed bytes),
			memmove(buf, buf + len, BUF_SIZE - len);
			// after moving, the unused spacec for receiving data
			index -= len;
			delete[] buf;
			return m_packet.sCmd;
		}
	}
	delete[] buf;
	return -1;
}

BOOL CServerSocket::Send(const char* pData, size_t nSize)
{
	if (m_client == INVALID_SOCKET) return FALSE;
	return send(m_client, pData, nSize, 0) != SOCKET_ERROR;
}

BOOL CServerSocket::Send(CPacket& packet)
{
	if (m_client == INVALID_SOCKET) return FALSE;
	return send(m_client, packet.Data(), packet.Size(), 0) != SOCKET_ERROR;
}

BOOL CServerSocket::GetFilePath(std::string& strPath)
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
BOOL CServerSocket::GetMouseEvent(MOUSEEV& mouse)
{
	if (m_packet.sCmd == CMD_MOUSE)
	{
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
		return TRUE;
	}
	return FALSE;
}

void CServerSocket::CloseClient()
{
	closesocket(m_client);
	m_client = INVALID_SOCKET;
}