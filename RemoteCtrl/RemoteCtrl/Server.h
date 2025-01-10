#pragma once
#include <map>
#include "Client.h"
#include "ThreadPool.h"
#include <winnt.h>
#include "Command.h"

class Client;
using PCLIENT = std::shared_ptr<Client>;

class Server : public ThreadFuncBase
{
public:
	Server(ULONG ip = INADDR_ANY, USHORT port = 20000);
	~Server();
	BOOL StartService();
	BOOL NewAccept();
	void BindNewSocket(SOCKET sock);
	std::map<SOCKET, PCLIENT> m_mapClients;
private:
	void InitSocket();
	inline BOOL SocketError();
	INT IOCPLoop();
	ThreadPool m_pool;
	HANDLE m_hIOCP;
	// accept socket, binded with IOCP
	SOCKET m_socket;
	//CQueue<Client> m_qClients;
	SOCKADDR_IN m_addr;
	ThreadWorker m_worker;
	// member cmd callback, and its pointer
	CMD_SPTR m_spCmd;
	CMD_CB m_cmdHandler;
};