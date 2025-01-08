#pragma once
#include "Thread.h"
#include "Queue.h"
#include <map>

#pragma warning(disable:4407)
class Client;
class Server;
using PCLIENT = std::shared_ptr<Client>;
enum Operator
{
	OP_NONE,
	OP_ACCEPT,
	OP_RECV,
	OP_SEND,
	OP_ERROR
};

template <Operator op> class AcceptOverlapped;
template <Operator op> class RecvOverlapped;
template <Operator op> class SendOverlapped;
template <Operator op> class ErrorOverlapped;
using ACCEPT_OVERLAPPED = AcceptOverlapped<OP_ACCEPT>;
using RECV_OVERLAPPED = RecvOverlapped<OP_ACCEPT>;
using SEND_OVERLAPPED = SendOverlapped<OP_SEND>;
using ERROR_OVERLAPPED = ErrorOverlapped<OP_ERROR>;

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
	Server *m_server; //TODO  potential memory leak
	PCLIENT m_client;
	WSABUF m_wsabuf;
	COverlapped();
};

template <Operator op>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped();
	int AcceptWorker();
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

class Client
{
public:
	Client();
	~Client();
	int Recv();
	void SetOverlapped(PCLIENT& ptr);

	// type conversion operator
	operator SOCKET() const { return m_socket; }
	operator PVOID() { return m_buffer.data(); }
	operator LPOVERLAPPED() { return &m_accept->m_overlapped; }
	operator LPDWORD() { return &m_received; }
	SOCKADDR_IN** GetLocalAddr() { return &m_laddr; }
	SOCKADDR_IN** GetRemoteAddr() { return &m_raddr; }
	size_t GetBufSize() const { return m_buffer.size(); }
	DWORD& GetFlags() { return m_flags; }
	LPWSABUF GetRecvWSABuf() { return &m_recv->m_wsabuf; }
	LPWSABUF GetSendWSABuf() { return &m_send->m_wsabuf; }
private:
	BOOL m_inUse;
	DWORD m_received;
	DWORD m_flags;
	size_t m_used;
	std::shared_ptr<ACCEPT_OVERLAPPED> m_accept;
	std::shared_ptr<RECV_OVERLAPPED> m_recv;
	std::shared_ptr<SEND_OVERLAPPED> m_send;
	std::shared_ptr<ERROR_OVERLAPPED> m_error;
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