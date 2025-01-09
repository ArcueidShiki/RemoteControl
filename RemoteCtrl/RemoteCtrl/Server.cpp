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

BOOL Server::StartService()
{
	InitSocket();
	if (bind(m_socket, (SOCKADDR*)&m_addr, sizeof(m_addr)) == -1)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return FALSE;
	}
	if (listen(m_socket, 3) == -1)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return FALSE;
	}

	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!m_hIOCP || m_hIOCP == INVALID_HANDLE_VALUE)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	// bind socket with iocp
	CreateIoCompletionPort((HANDLE)m_socket, m_hIOCP, (ULONG_PTR)this, 0);
	m_worker = ThreadWorker(this, (FUNC)&Server::IOCPLoop);
	m_pool.Invoke();
	m_pool.DispatchWorker(m_worker);
	return NewAccept();
}

BOOL Server::NewAccept()
{
	PCLIENT pClient = std::make_shared<Client>();
	pClient->SetOverlapped(pClient.get());
	m_mapClients.insert(std::pair<SOCKET, PCLIENT>(pClient->GetSocket(), pClient));
	// async accept
	if (!AcceptEx(
		m_socket,
		pClient->GetSocket(),
		pClient->GetBuffer(),
		0, // 0 means no additional data is received, after connection is built
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		pClient->GetReceived(),
		pClient->GetAcceptOverlapped())) // incorrect arguments
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			TRACE("AcceptEx failed with error: %d\n", err);
			// TODO 10022
			// Memory violation, heap pointer
			closesocket(m_socket);
			CloseHandle(m_hIOCP);
			m_socket = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return FALSE;
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
			// socket has IO event
			COverlapped *pOverlapped = CONTAINING_RECORD(lpOverlapped, COverlapped, m_overlapped);
			//TRACE("Server recv overlapped opt: %lu\n", pOverlapped->m_operator);
			pOverlapped->m_server = this;
			switch (pOverlapped->m_operator)
			{
				case Operator::OP_ACCEPT:
				{	// may have pointer offset with base class
					ACCEPT_OVERLAPPED* pOver = (ACCEPT_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);	// AcceptWorker, When worker finished
				}
					break;
				case Operator::OP_RECV:
				{
					RECV_OVERLAPPED* pOver = (RECV_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
					break;
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