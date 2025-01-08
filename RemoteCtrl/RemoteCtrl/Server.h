#pragma once
#include <map>
//#include "Queue.h"
#include "Client.h"
#include "ThreadPool.h"

class Client;
using PCLIENT = std::shared_ptr<Client>;

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
	//CQueue<Client> m_qClients;
	SOCKADDR_IN m_addr;
};