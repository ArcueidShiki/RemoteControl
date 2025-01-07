#pragma once
#include "Thread.h"
#include "Queue.h"
#include <map>

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
using PCLIENT = std::shared_ptr<Client>;

enum Operator
{
	OP_NONE,
	OP_ACCEPT,
	OP_RECV,
	OP_SEND,
	OP_ERROR
};

class COverlapped
{
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	std::vector<char> m_buffer;
	ThreadWorker m_worker; // corresponding callback
	Server *m_server;
	COverlapped()
	{
		m_operator = 0;
		m_overlapped = { 0 };
		m_buffer.resize(0);
		m_worker = ThreadWorker();
		m_server = NULL;
	}
};

template <Operator op>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped()
	{
		m_overlapped = { 0 };
		m_buffer.resize(1024, 0);
		m_server = NULL;
		m_operator = op;
		m_worker = ThreadWorker(this, (FUNC)&AcceptOverlapped<op>::AcceptWorker);
	}
	int AcceptWorker()
	{
		INT lLength = 0, rLength = 0;
		if (*(LPDWORD)*m_client > 0)
		{
			GetAcceptExSockaddrs(*m_client, 0,
				sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16,
				(SOCKADDR**)m_client->GetLocalAddr(), &lLength,
				(SOCKADDR**)m_client->GetRemoteAddr(), &rLength);
			if (!m_server->NewAccept())
			{
				return -2;
			}
		}
		return -1;
	}
public:
	PCLIENT m_client;
};
typedef AcceptOverlapped<OP_ACCEPT> ACCEPT_OVERLAPPED;

template <Operator op>
class RecvOverlapped : public COverlapped, ThreadFuncBase
{
public:
	RecvOverlapped() : m_operator(op), m_worker(this, &RecvOverlapped::RecvWorker)
	{
		m_overlapped = { 0 };
		m_buffer.resize(1024 * 256, 0);
	}
	int RecvWorker()
	{

	}
};
typedef RecvOverlapped<OP_ACCEPT> RECV_OVERLAPPED;

template <Operator op>
class SendOverlapped : public COverlapped, ThreadFuncBase
{
public:
	SendOverlapped() : m_operator(op), m_worker(this, &SendOverlapped::SendWorker)
	{
		m_overlapped = { 0 };
		m_buffer.resize(1024 * 256, 0);
	}
	int SendWorker()
	{

	}
};
typedef SendOverlapped<OP_SEND> SEND_OVERLAPPED;

template <Operator op>
class ErrorOverlapped : public COverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped() : m_operator(op), m_worker(this, &ErrorOverlapped::ErrorWorker)
	{
		m_overlapped = { 0 };
		m_buffer.resize(1024, 0);
	}
	int ErrorWorker()
	{

	}
};
typedef ErrorOverlapped<OP_ERROR> ERROR_OVERLAPPED;

class Client
{
public:
	Client();
	~Client();
	// type conversion operator
	operator SOCKET();
	operator PVOID();
	operator LPOVERLAPPED();
	operator LPDWORD();
	void SetOverlapped(PCLIENT& ptr);
	SOCKADDR_IN** GetLocalAddr() { return &m_laddr; }
	SOCKADDR_IN** GetRemoteAddr() { return &m_raddr; }
private:
	BOOL m_inUse;
	DWORD m_received;
	std::shared_ptr<ACCEPT_OVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;

	SOCKET m_socket;
	SOCKADDR_IN *m_laddr;
	SOCKADDR_IN *m_raddr;
};

class Server : public ThreadFuncBase
{
public:
	Server(ULONG ip = INADDR_ANY, USHORT port = 20000);
	~Server();
	BOOL StartService();
	BOOL NewAccept();
	std::map<SOCKET, PCLIENT> m_mapClients;
private:
	void InitSocket();
	INT ThreadLoop();
	ThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_socket;
	CQueue<Client> m_qClients;
	SOCKADDR_IN m_addr;
};