#pragma once
#include "Thread.h"
#include "Overlapped.h"
#include "Queue.h"
#include "Operator.h"
#include "Command.h"

// Forward declaration due to circular dependency
template <Operator op> class AcceptOverlapped;
template <Operator op> class RecvOverlapped;
template <Operator op> class SendOverlapped;
template <Operator op> class ErrorOverlapped;
using ACCEPT_OVERLAPPED = AcceptOverlapped<OP_ACCEPT>;
using RECV_OVERLAPPED = RecvOverlapped<OP_RECV>;
using SEND_OVERLAPPED = SendOverlapped<OP_SEND>;
using ERROR_OVERLAPPED = ErrorOverlapped<OP_ERROR>;

class Client : public ThreadFuncBase
{
public:
	Client(CMD_SPTR &cmd, CMD_CB callback = NULL);
	~Client();
	int Recv();
	int Send();
	void SetOverlapped(Client* ptr);
	 SOCKET GetSocket() const;
	 PVOID GetBuffer();
	 LPDWORD GetReceived();
	 SOCKADDR_IN** GetLocalAddr();
	 SOCKADDR_IN** GetRemoteAddr();
	 size_t GetBufSize();
	 LPDWORD GetFlags();
	 LPWSABUF GetRecvWSABuf();
	 LPWSABUF GetSendWSABuf();
	LPOVERLAPPED GetAcceptOverlapped();
	LPOVERLAPPED GetRecvOverLapped();
	LPOVERLAPPED GetSendOverLapped();
	LPOVERLAPPED GetErrorOverLapped();
	int ProcessCommand();
public:
	std::shared_ptr<typename ACCEPT_OVERLAPPED> m_accept; 
	std::shared_ptr<typename RECV_OVERLAPPED> m_recv;
	std::shared_ptr<typename SEND_OVERLAPPED> m_send;
	std::shared_ptr<typename ERROR_OVERLAPPED> m_error;
private:
	int ParseCommand();
private:
	BOOL m_inUse;
	DWORD m_received;
	DWORD m_sent;
	DWORD m_flags;
	size_t m_used;
	std::vector<char> m_buffer;

	SOCKET m_socket;
	SOCKADDR_IN* m_laddr;
	SOCKADDR_IN* m_raddr;
	std::atomic<BOOL> m_lock;
	CQueue<CPacket> *m_queue;	// Too many send queue, should be only one singleton
	// client don't own the cmd itself, because, it will copy self times
	// obtain cmd obj from server, and it function pointer callback;
	CPacket m_packet; // incoming packet
	CMD_SPTR m_spCmd;
};