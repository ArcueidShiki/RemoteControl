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
		MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	if (!InitSocket())
	{
		MessageBox(NULL, _T("无法初始化套接字，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	MessageBox(NULL, _T("初始化套接字成功"), _T("1111111111"), MB_OK | MB_ICONERROR);
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
		MessageBox(NULL, _T("无法创建套接字，请检查网络设置"), _T("创建错误!"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	SOCKADDR_IN serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(10000);
	if (bind(m_socket, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("无法绑定套接字，请检查网络设置"), _T("绑定错误!"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (listen(m_socket, 1) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("无法监听套接字，请检查网络设置"), _T("监听错误!"), MB_OK | MB_ICONERROR);
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
		printf("accept failed client: %zu, m_socket：%zu\n", m_client, m_socket);
		return FALSE;
	}
	return TRUE;
}

int CServerSocket::DealCommand()
{
	if (m_client == INVALID_SOCKET)
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
#define BUF_SIZE 4096
	char* buf = new char[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	size_t index = 0;
	while (TRUE)
	{
		size_t len = recv(m_client, buf + index, BUF_SIZE - index, 0);
		if (len <= 0)
		{
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
			return m_packet.sCmd;
		}
	}
	return -1;
}

BOOL CServerSocket::Send(const char* pData, size_t nSize)
{
	if (send(m_client, pData, int(nSize), 0) == SOCKET_ERROR)
	{
		return FALSE;
	}
	return TRUE;
}

CPacket::CPacket()
	: sHead(0)
	, nLength(0)
	, sCmd(0)
	, sSum(0)
{
}

CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	// head
	size_t i = 0;
	for (; i < nSize; i++)
	{
		if (*(WORD*)(pData + i) == PACKET_HEAD)
		{
			sHead += PACKET_HEAD;
			i += sizeof(sHead); // skip head
			break;
		}
	}

	// length			length + cmd + sum
	if (i + sizeof(nLength) + sizeof(sCmd) + sizeof(sSum) > nSize)
	{
		nSize = 0;
		return; // parse packet error.
	}
	nLength = *(DWORD*)(pData + i);
	i += sizeof(nLength);

	// cmd
	if (nLength + i > nSize)
	{
		// message is not received completely, its larger than the packet
		nSize = 0;
		return;
	}
	sCmd = *(WORD*)(pData + i);
	i += sizeof(sCmd);

	// str data
	DWORD dataLen = nLength - sizeof(sCmd) - sizeof(sSum);
	strData.resize(dataLen);
	memcpy((void*)strData.c_str(), pData + i, dataLen);
	i += dataLen;

	// check sum
	sSum = *(WORD*)(pData + i);
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sum += BYTE(strData[i]); // &0xFF
	}
	if (sum != sSum)
	{
		nSize = 0;
		return;
	}
	i += sizeof(sSum);
	// parse packet success.
	nSize = i; // head length data
}

CPacket::CPacket(const CPacket &other)
{
	sHead = other.sHead;
	nLength = other.nLength;
	sCmd = other.sCmd;
	strData = other.strData;
	sSum = other.sSum;
}

CPacket& CPacket::operator=(const CPacket &other)
{
	if (this != &other)
	{
		sHead = other.sHead;
		nLength = other.nLength;
		sCmd = other.sCmd;
		strData = other.strData;
		sSum = other.sSum;
	}
	return *this;
}