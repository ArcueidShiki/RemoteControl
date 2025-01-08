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
			// TODO field has problems
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
	m_accept = std::make_shared<ACCEPT_OVERLAPPED>();
	m_recv = std::make_shared<RECV_OVERLAPPED>();
	m_send = std::make_shared<SEND_OVERLAPPED>();
	m_error = std::make_shared<ERROR_OVERLAPPED>();
	m_accept->m_client.reset(this);
	m_recv->m_client.reset(this);
	m_send->m_client.reset(this);
	m_error->m_client.reset(this);
	m_received = 0;
	m_flags = 0;
	m_used = 0;
}

Client::~Client()
{
	closesocket(m_socket);
}

int Client::Recv()
{
	int recved = recv(m_socket, m_buffer.data() + m_used, int(m_buffer.size() -  m_used), 0);
	if (recved <= 0) return -1;
	m_used += recved;
	// TODO parse packet
	return 0;
}

void Client::SetOverlapped(PCLIENT& ptr)
{
	m_accept->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
	m_error->m_client = ptr;
}

COverlapped::COverlapped()
{
	m_operator = 0;
	m_overlapped = { 0 };
	m_buffer.resize(0);
	m_worker = ThreadWorker();
	m_server = NULL;
	m_client = NULL;
	m_wsabuf = { (ULONG)m_buffer.size(), m_buffer.data() };
}

template<Operator op>
inline AcceptOverlapped<op>::AcceptOverlapped()
{
	m_overlapped = { 0 };
	m_operator = op;
	m_buffer.resize(1024, 0);
	m_server = NULL;
	m_client = NULL;
	m_wsabuf = { (ULONG)m_buffer.size(), m_buffer.data() };
	m_worker = ThreadWorker(this, (FUNC)&AcceptOverlapped<op>::AcceptWorker);
}

template<Operator op>
inline int AcceptOverlapped<op>::AcceptWorker()
{
	INT lLength = 0, rLength = 0;
	if (*(LPDWORD)*m_client > 0)
	{
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			(SOCKADDR**)m_client->GetLocalAddr(), &lLength,
			(SOCKADDR**)m_client->GetRemoteAddr(), &rLength);
		int ret = WSARecv((SOCKET)*m_client,
			m_client->GetRecvWSABuf(),
			1, (LPDWORD)*m_client,
			&m_client->GetFlags(),
			*m_client, NULL);
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			// TODO show error
		}
		if (!m_server || !m_server->NewAccept())
		{
			return -2;
		}
	}
	return -1;
}

template<Operator op>
inline RecvOverlapped<op>::RecvOverlapped()
{
	m_overlapped = { 0 };
	m_operator = op;
	m_buffer.resize(1024 * 256, 0);
	m_server = NULL;
	m_client = NULL;
	m_wsabuf = { (ULONG)m_buffer.size(), m_buffer.data() };
	m_worker = ThreadWorker(this, (FUNC)&RecvOverlapped<op>::RecvWorker);
}

template<Operator op>
inline int RecvOverlapped<op>::RecvWorker()
{
	return m_client->Recv();
}

template<Operator op>
inline SendOverlapped<op>::SendOverlapped()
{
	m_overlapped = { 0 };
	m_operator = op;
	m_buffer.resize(1024 * 256, 0);
	m_server = NULL;
	m_client = NULL;
	m_wsabuf = { (ULONG)m_buffer.size(), m_buffer.data() };
	m_worker = ThreadWorker(this, (FUNC)&SendOverlapped<op>::SendWorker);
}

template<Operator op>
inline int SendOverlapped<op>::SendWorker()
{
	return 0;
}

template<Operator op>
inline ErrorOverlapped<op>::ErrorOverlapped()
{
	m_overlapped = { 0 };
	m_operator = op;
	m_buffer.resize(1024, 0);
	m_server = NULL;
	m_client = NULL;
	m_wsabuf = { (ULONG)m_buffer.size(), m_buffer.data() };
	m_worker = ThreadWorker(this, (FUNC)&ErrorOverlapped<op>::ErrorWorker);
}

template<Operator op>
inline int ErrorOverlapped<op>::ErrorWorker()
{
	return 0;
}
