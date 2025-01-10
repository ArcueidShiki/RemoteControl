#pragma once
#include "Socket.h"
#include "ThreadPool.h"

#define SHOW_SOCKET_ERROR() printf("%s(%d):%s Server Socket Error(%d)\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError())

class CNetwork
{
public:

};

// shared_ptr passed by reference avoiding copy create and reference count increase
typedef int (*AcceptCallback)(void* arg, SOCKSPTR &client);
using RecvCallback = int(*)(void* arg, const Buffer& buf);
using SendCallback = int(*)(void* arg, SOCKSPTR &client, int ret);
using RecvFromCallback = int(*)(void* arg, const Buffer& buf, SocketAddrIn& addr);
using SendToCallback = int(*)(void* arg, const SocketAddrIn &addr, int ret);

class NServerParam
{
public:
	NServerParam(const NServerParam& other);
	NServerParam& operator=(const NServerParam& other);
	NServerParam(const char* sIP, USHORT port, PTYPE type);
	NServerParam(
		const char* sIP = "0.0.0.0",
		USHORT port = 30000,
		PTYPE type = PTYPE::TCP,
		AcceptCallback accept = NULL,
		RecvCallback recv = NULL,
		SendCallback send = NULL,
		RecvFromCallback recvfrom = NULL,
		SendToCallback sendto = NULL
	);
	NServerParam& operator<<(AcceptCallback func);
	NServerParam& operator<<(RecvCallback func);
	NServerParam& operator<<(SendCallback func);
	NServerParam& operator<<(RecvFromCallback func);
	NServerParam& operator<<(SendToCallback func);
	NServerParam& operator<<(const char *sIP);
	NServerParam& operator<<(USHORT port);
	NServerParam& operator<<(PTYPE type);

	NServerParam& operator>>(AcceptCallback& func);
	NServerParam& operator>>(RecvCallback& func);
	NServerParam& operator>>(SendCallback& func);
	NServerParam& operator>>(RecvFromCallback& func);
	NServerParam& operator>>(SendToCallback& func);
	NServerParam& operator>>(char* sIP);
	NServerParam& operator>>(USHORT &port);
	NServerParam& operator>>(PTYPE &type);
	char m_sIP[16];
	USHORT m_port;
	PTYPE m_type;
	AcceptCallback m_acceptCallback;
	RecvCallback m_recvCallback;
	SendCallback m_sendCallback;
	RecvFromCallback m_recvfromCallback;
	SendToCallback m_sendtoCallback;
};

class NServer : public ThreadFuncBase
{
public:
	NServer(const NServerParam &param);
	~NServer();
	int Invoke(void* arg);
	int Send(SOCKSPTR& client, const Buffer& buf);
	int Sendto(const Buffer &buf, SocketAddrIn& addr);
	int Stop();
private:
	int ThreadLoop();
	int ThreadUdpLoop();
	int ThreadTcpLoop();
private:
	NServerParam m_param;
	CThread m_thread;
	SOCKSPTR m_spSocket;
	void* m_args;
	std::atomic<BOOL> m_bRunning;
};