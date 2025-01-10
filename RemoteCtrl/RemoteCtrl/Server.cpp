#include <winsock2.h>
#include <MSWSock.h>
#include "pch.h"
#include "Server.h"
#include "Utils.h"

Server::Server(ULONG ip, USHORT port) : m_pool(10)
{
	m_hIOCP = INVALID_HANDLE_VALUE;
	m_socket = INVALID_SOCKET;
	m_addr = {};
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.S_un.S_addr = htonl(ip);
	m_addr.sin_port = htons(port);
	m_worker = ThreadWorker();
	m_spCmd = std::make_shared<Command>();
}

Server::~Server()
{
	closesocket(m_socket);
	//CloseHandle(m_hIOCP);
	m_mapClients.clear();
	WSACleanup();
}

void Server::InitSocket()
{
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0)
	{
		TRACE("WSAStartup failed with error: %d\n", ret);
		return;
	}
	m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
}

inline BOOL Server::SocketError()
{
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
	return FALSE;
}

BOOL Server::StartService()
{
	InitSocket();
	if (bind(m_socket, (SOCKADDR*)&m_addr, sizeof(m_addr)) == -1)
	{
		return SocketError();
	}
	if (listen(m_socket, 3) == -1)
	{
		return SocketError();
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!m_hIOCP || m_hIOCP == INVALID_HANDLE_VALUE)
	{
		m_hIOCP = INVALID_HANDLE_VALUE;
		return SocketError();
	}
	// bind socket with iocp
	CreateIoCompletionPort((HANDLE)m_socket, m_hIOCP, (ULONG_PTR)this, 0);
	m_worker = ThreadWorker(this, (FUNC)&Server::IOCPLoop);
	m_pool.Invoke();
	// thread pool dispatch worker with new connection request pool size defalut 10
	m_pool.DispatchWorker(m_worker);
	return NewAccept();
}

BOOL Server::NewAccept()
{
	PCLIENT pClient = std::make_shared<Client>(m_spCmd);
	pClient->SetOverlapped(pClient.get());
	// when to erase, otherwise it will expand expoentially
	m_mapClients.insert(std::pair<SOCKET, PCLIENT>(pClient->GetSocket(), pClient));
	// async accept
	if (!AcceptEx(
		m_socket,	// server bind and listen socket 
		pClient->GetSocket(), // accpeted client socket
		pClient->GetBuffer(),
		0, // 0 means no additional data is received, after connection is built
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		pClient->GetReceived(),
		pClient->GetAcceptOverlapped())) // if success jump to accept worker
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			TRACE("AcceptEx failed with error: %d\n", err);
			closesocket(m_socket);
			m_hIOCP = INVALID_HANDLE_VALUE;
			return SocketError();
		}
		Sleep(1);
	}
	return TRUE;
}

void Server::BindNewSocket(SOCKET sock)
{
	CreateIoCompletionPort((HANDLE)sock, m_hIOCP, (ULONG_PTR)this, 0);
}

INT Server::IOCPLoop()
{
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	// when socket ready, invoke.
	while (GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, &CompletionKey, &lpOverlapped, INFINITE))
	{
		if (CompletionKey != 0)
		{
			COverlapped *pOverlapped = CONTAINING_RECORD(lpOverlapped, COverlapped, m_overlapped);
			pOverlapped->m_server = this;
			switch (pOverlapped->m_operator)
			{
				// server socket has IO accept event occured
				case Operator::OP_ACCEPT:
				{	// may have pointer offset with base class
					ACCEPT_OVERLAPPED* pOver = (ACCEPT_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);	// AcceptWorker, When worker finished
				}
					break;
				// one client socket in map has recv IO event occured
				case Operator::OP_RECV:
				{
					RECV_OVERLAPPED* pOver = (RECV_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
					break;
				// one client socket in map has send IO event occured
				case Operator::OP_SEND:
				{
					SEND_OVERLAPPED* pOver = (SEND_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
					break;
				case Operator::OP_ERROR:
				{
					ERROR_OVERLAPPED* pOver = (ERROR_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
					break;
			}
		}
		else {
			TRACE("Server ThreadLoop: IOCP(socket) get input data size: %ul\n", dwTransferred);
		}
	}
	// If GetQueuedCompletionStatus returns FALSE, handle the error
	if (!GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, &CompletionKey, &lpOverlapped, INFINITE))
	{
		DWORD error = GetLastError();
		TRACE("GetQueuedCompletionStatus failed with error: %d\n", error);
		// Handle specific errors if needed
		switch (error)
		{
		case ERROR_ABANDONED_WAIT_0:
			TRACE("The I/O completion port handle was closed.\n");
			break;
		case ERROR_INVALID_HANDLE:
			TRACE("The handle is invalid.\n");
			break;
		case ERROR_OPERATION_ABORTED:
			TRACE("The I/O operation has been aborted.\n");
			break;
		default:
			TRACE("An unknown error occurred.\n");
			break;
		}
	}
	return 0;
}