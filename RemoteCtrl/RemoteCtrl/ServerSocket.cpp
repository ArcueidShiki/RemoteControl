#include "pch.h"
#include "ServerSocket.h"

//define and init static member outside class
CServerSocket* CServerSocket::m_instance = NULL;
// help trigger delete deconstructor
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket::CServerSocket()
	: m_client(INVALID_SOCKET)
{
	if (!InitSocketEnv())
	{
		MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	if (!InitSocket())
	{
		MessageBox(NULL, _T("无法初始化套接字，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
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
	if (m_socket == INVALID_SOCKET) return FALSE;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(9999);
	if (bind(m_socket, (SOCKADDR*)&serv_addr, sizeof(m_socket)) == SOCKET_ERROR)
	{
		return FALSE;
	}
	if (listen(m_socket, 1) == SOCKET_ERROR)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CServerSocket::AcceptClient()
{
	sockaddr_in client_addr;
	int client_addr_size = sizeof(client_addr);
	m_client = accept(m_socket, (SOCKADDR*)&client_addr, &client_addr_size);
	if (m_client == INVALID_SOCKET)
	{
		return FALSE;
	}
	//recv(client_sock, buf, sizeof(buf), 0);
	//send(client_sock, buf, sizeof(buf), 0);
	return TRUE;
}

int CServerSocket::DealCommand()
{
	if (m_client == INVALID_SOCKET)
	{
		return -1;
	}
	char buf[1024] = { 0 };
	while (TRUE)
	{
		memset(buf, 0, sizeof(buf));
		int len = recv(m_client, buf, sizeof(buf), 0);
		if (len <= 0)
		{
			return -1;
		}
		if (strcmp(buf, "exit") == 0)
		{
			break;
		}
		// TODO handle command
	}
	return 0;
}

BOOL CServerSocket::Send(const char* pData, size_t nSize)
{
	if (send(m_client, pData, int(nSize), 0) == SOCKET_ERROR)
	{
		return FALSE;
	}
	return TRUE;
}