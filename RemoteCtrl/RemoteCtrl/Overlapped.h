#pragma once
#include "pch.h"
#include <MSWSock.h>
#include "Thread.h"
#include "Server.h"
#include "Operator.h"
#include "Utils.h"
#pragma warning(disable:4407)

class Client;
class Server;
/*
1. passed by value, thread safe, but more overhead, when you need to take over ownership
2. passed by ref, less overhead, no copy, and thread safe operation on ptr, but need to ensure lifecycle, avoid moving ownership
-------------------------------
struct MyStruct {
	typedef std::vector<T> Vec;
};
-------------------------------
template <typename T>
using Vec = std::vector<T>;

template <typename T>
struct MyStruct {
	using Vec = std::vector<T>;
};
*/

class COverlapped
{
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	std::vector<char> m_buffer;
	ThreadWorker m_worker; // corresponding callback
	Server* m_server;
	Client* m_client;
	WSABUF m_wsabuf;
	COverlapped();
	virtual ~COverlapped();
};

// explicit instantiation, so that the compiler can generate the code
// implementation can be put in cpp file
template <Operator op>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped();
	int AcceptWorker();	// assign to m_worker when constructing.
};

template <Operator op>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
	RecvOverlapped();
	int RecvWorker();
};

template <Operator op>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
	SendOverlapped();
	int SendWorker();
};

template <Operator op>
class ErrorOverlapped : public COverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped();
	int ErrorWorker();
};

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
	if (!m_client) return -1;
	if (!m_server) return -1;
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufSize())
	{
		GetAcceptExSockaddrs(
			m_client->GetBuffer(), 0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			(SOCKADDR**)m_client->GetLocalAddr(), &lLength,
			(SOCKADDR**)m_client->GetRemoteAddr(), &rLength);
		// add accepted client socket to IOCP handle
		m_server->BindNewSocket(m_client->GetSocket());
		int ret = WSARecv(
			m_client->GetSocket(),
			m_client->GetRecvWSABuf(),
			1,
			m_client->GetReceived(),
			m_client->GetFlags(),
			m_client->GetRecvOverLapped(),
			NULL);
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			// 997 WSA_IO_PENDING is not considered as an error in nonblock mode.
			TRACE("WSARecv failed with error: %d\n", WSAGetLastError());
			CUtils::ShowError();
			return -1;
		}
		if (!m_server->NewAccept())
		{
			return -1;
		}
	}
	// ret < 0 end worker.
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
	if (!m_client) return -1;
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
	if (!m_client) return -1;
	// Send Action won't complete immediately
	// TODO
	return 0;
}

template<Operator op>
inline ErrorOverlapped<op>::ErrorOverlapped()
{
	if (!m_client) return;
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
