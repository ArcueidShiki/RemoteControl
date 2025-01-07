#pragma once
#include "Thread.h"
#include <map>

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
	COverlapped()
	{
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.clear();
	}
};

class Client
{

};

class Server : public ThreadFuncBase
{
public:
	Server(ULONG ip = INADDR_ANY, USHORT port = 20000);
	~Server();
private:
	INT ThreadLoop();
	ThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_socket;
	std::map<SOCKET, std::shared_ptr<Client>> m_mapClients;
};

template <Operator op>
class AcceptOverlapped : public COverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped() : m_operator(op) , m_worker(this, &AcceptOverlapped::AcceptWorker)
	{
		m_overlapped = { 0 };
		m_buffer.resize(1024, 0);
	}
	int AcceptWorker()
	{
		// TODO
	}
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
	int RecvWorker();
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
	int SendWorker();
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
	int ErrorWorker();
};
typedef ErrorOverlapped<OP_ERROR> ERROR_OVERLAPPED;