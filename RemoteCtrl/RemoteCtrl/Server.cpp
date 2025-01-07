#include "pch.h"
#include "Server.h"
#include <MSWSock.h>

Server::Server(ULONG ip, USHORT port) : m_pool(10)
{
	m_hIOCP = INVALID_HANDLE_VALUE;
	m_socket = INVALID_SOCKET;
	m_addr = {};
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.S_un.S_addr = htonl(ip);
	m_addr.sin_port = htons(port);
}

Server::~Server()
{
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
	CreateIoCompletionPort((HANDLE)m_socket, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	ThreadWorker* pWorker = new ThreadWorker(this, (FUNC)&Server::ThreadLoop);
	m_pool.DispatchWorker(pWorker);
	return NewAccept();
}

void Server::InitSocket()
{
	m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
}

BOOL Server::NewAccept()
{
	PCLIENT pClient(new Client());
	pClient->SetOverlapped(pClient);
	m_mapClients.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
	// async accept
	if (!AcceptEx(m_socket, *pClient, *pClient, 0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		*pClient, *pClient))
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		CloseHandle(m_hIOCP);
		m_hIOCP = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	return TRUE;
}

INT Server::ThreadLoop()
{
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	while (GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, &CompletionKey, &lpOverlapped, INFINITE))
	{
		if (dwTransferred > 0 && CompletionKey != 0)
		{
			COverlapped *pOverlapped = CONTAINING_RECORD(lpOverlapped, COverlapped, m_overlapped);
			switch (pOverlapped->m_operator)
			{
				case Operator::OP_ACCEPT:
				{	// wathc m_worker lifecycle from lpOverlapped
					ACCEPT_OVERLAPPED* pOver = (ACCEPT_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(&pOver->m_worker);
				}
					break;
				case Operator::OP_RECV:
				{
					RECV_OVERLAPPED* pOver = (RECV_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(&pOver->m_worker);
				}
					break;
				case Operator::OP_SEND:
				{
					SEND_OVERLAPPED* pOver = (SEND_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(&pOver->m_worker);
				}
					break;
				case Operator::OP_ERROR:
				{
					ERROR_OVERLAPPED* pOver = (ERROR_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(&pOver->m_worker);
				}
					break;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}

Client::Client()
{
	m_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	m_laddr = NULL;
	m_raddr = NULL;
	m_inUse = FALSE;
	m_overlapped = std::make_shared<ACCEPT_OVERLAPPED>();
	m_overlapped->m_client.reset(this);
	m_received = 0;
}

Client::~Client()
{
	closesocket(m_socket);
}

Client::operator SOCKET()
{
	return m_socket;
}

Client::operator PVOID()
{
	return m_buffer.data();
}

Client::operator LPOVERLAPPED()
{
	return &m_overlapped->m_overlapped;
}

Client::operator LPDWORD()
{
	return &m_received;
}

void Client::SetOverlapped(PCLIENT& ptr)
{
	m_overlapped->m_client = ptr;
}
