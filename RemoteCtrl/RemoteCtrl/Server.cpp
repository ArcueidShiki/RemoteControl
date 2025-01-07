#include "pch.h"
#include "Server.h"

Server::Server(ULONG ip, USHORT port) : m_pool(10)
{
	m_hIOCP = INVALID_HANDLE_VALUE;
	m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
	SOCKADDR_IN addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(ip);
	addr.sin_port = htons(port);
	if (bind(m_socket, (SOCKADDR*)&addr, sizeof(addr)) == -1)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return;
	}
	if (listen(m_socket, 3) == -1)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return;
	}

	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!m_hIOCP || m_hIOCP == INVALID_HANDLE_VALUE)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		m_hIOCP == INVALID_HANDLE_VALUE;
		return;
	}
	CreateIoCompletionPort((HANDLE)m_socket, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.DispatchWorker(ThreadWorker(this, (FUNC)&Server::ThreadLoop));
}

Server::~Server()
{
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
					ACCEPT_OVERLAPPED *pOver = (ACCEPT_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				case Operator::OP_RECV:
					RECV_OVERLAPPED* pOver = (RECV_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				case Operator::OP_SEND:
					SEND_OVERLAPPED* pOver = (SEND_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
				case Operator::OP_ERROR:
					ERROR_OVERLAPPED* pOver = (ERROR_OVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
					break;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}
