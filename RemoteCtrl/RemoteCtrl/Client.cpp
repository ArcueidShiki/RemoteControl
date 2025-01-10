#include "pch.h"
#include "Client.h"
#include "Utils.h"

constexpr int BUF_SIZE = 4096;
CQueue<CPacket>* CQueue<CPacket>::m_instance = NULL;
CQueue<CPacket>::CHelper CQueue<CPacket>::m_helper;

Client::Client(CMD_SPTR &cmd, CMD_CB callback)
{
	m_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(BUF_SIZE, 0);
	m_laddr = NULL;
	m_raddr = NULL;
	m_inUse = FALSE;
	m_accept = std::make_shared<ACCEPT_OVERLAPPED>();
	m_recv = std::make_shared<RECV_OVERLAPPED>();
	m_send = std::make_shared<SEND_OVERLAPPED>();
	m_error = std::make_shared<ERROR_OVERLAPPED>();
	m_received = 0;
	m_flags = 0;
	m_used = 0;
	m_lock.store(FALSE);
	m_spCmd = cmd;
	m_sent = 0;
	m_queue = CQueue<CPacket>::GetInstance();
}

Client::~Client()
{
	if (m_lock.load()) return;
	m_lock.exchange(TRUE);
	closesocket(m_socket);
	m_buffer.clear();
	m_accept.reset();
	m_recv.reset();
	m_send.reset();
	m_error.reset();
	//m_queue->Clear();
}

int Client::Recv()
{
#if 0
	CUtils::Dump((BYTE*)m_recv->m_buffer.data(), m_received);
	CUtils::Dump((BYTE*)m_recv->m_wsabuf.buf, m_received);
	TRACE("Get : [%lu] Bytes From Client; recv buff position:[%p], buf size:[%zu]\n", m_received, m_recv->m_buffer.data(), m_recv->m_buffer.size());
	TRACE("Get : [%lu] Bytes From Client; recv  wsabuff position:[%p], wsabuf size:[%zu]\n", m_received, m_recv->m_wsabuf.buf, m_recv->m_wsabuf.len);
#endif
	int ret = WSARecv(
			m_socket,
			&m_recv->m_wsabuf,
			1,
			&m_received,
			&m_flags,
			&m_recv->m_overlapped, // to go recv overlapped
			NULL);
	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		// 997 WSA_IO_PENDING is not considered as an error in nonblock mode.
		TRACE("WSARecv failed with error: %d\n", WSAGetLastError());
		CUtils::ShowError();
		return -1;
	}
	int cmd = ParseCommand();
	if (cmd > 0)
	{
		// command using nCmd + incoming packet(msg from client)
		// push ack packets got from Command to send queue.
		m_spCmd->ExecuteCommand(cmd, *m_queue, m_packet);
		Send();
		return cmd;
	}
	return ret;
}

void Client::SetOverlapped(Client* ptr)
{
	m_accept->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
	m_error->m_client = ptr;
}

// from send queue callback jump to here.
int Client::Send()
{
	while (m_queue->Size() > 0)
	{
		// copy packet to wsa send buffer;
		CPacket packet;
		if (!m_queue->PopFront(packet))
		{
			return -1;
		}
		memset(m_send->m_wsabuf.buf, 0, m_send->m_wsabuf.len);
		memcpy(m_send->m_wsabuf.buf, packet.GetData(), packet.Size());
		int ret = WSASend(m_socket, GetSendWSABuf(),
			1, &m_sent, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0)
		{
			int err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
			{
				TRACE("Client, Send Queue err: %d\n", err);
				CUtils::ShowError();
			}
		}
		return ret;
	}
	return -1;
}

 SOCKET Client::GetSocket() const
{
	return m_socket;
}

 PVOID Client::GetBuffer()
{
	return (void*)m_buffer.data();
}

LPOVERLAPPED Client::GetAcceptOverlapped()
{
	return &m_accept->m_overlapped;
}

LPOVERLAPPED Client::GetRecvOverLapped()
{
	return &m_recv->m_overlapped;
}

LPOVERLAPPED Client::GetSendOverLapped()
{
	return &m_send->m_overlapped;
}

LPOVERLAPPED Client::GetErrorOverLapped()
{
	return &m_error->m_overlapped;
}

int Client::ProcessCommand()
{
	int cmd = ParseCommand();
	if (cmd > 0)
	{
		// command using nCmd + incoming packet(msg from client)
		// push ack packets got from Command to send queue.
		m_spCmd->ExecuteCommand(cmd, *m_queue, m_packet);
		return cmd;
	}
	return -1;
}

int Client::ParseCommand()
{
	char* buf = m_recv->m_wsabuf.buf;
	if (!buf)
	{
		TRACE("m_recv->m_wsabuf.buf is NULL\n");
		return -2;
	}
	while (TRUE)
	{
		if (m_received < 0)
		{
			return -1;
		}

		// potential issue.
		m_used = m_received;
		m_packet = CPacket((BYTE*)buf, m_used);
		if (m_used > 0)
		{
			// move unparse bytes leftover to head of buffer(parsed bytes),
			memmove(buf, buf + m_used, m_recv->m_wsabuf.len - m_used);
			// after moving, the unused spacec for receiving data
			m_received -= m_used;
			return m_packet.sCmd;
		}
		// if cannot parse a packet, continue receiving.
		//int ret = WSARecv(
		//	m_socket,
		//	&m_recv->m_wsabuf,
		//	1,
		//	&m_received,
		//	&m_flags,
		//	&m_recv->m_overlapped,
		//	NULL);
	}
	return -1;
}

 LPDWORD Client::GetReceived()
{
	return &m_received;
}

 SOCKADDR_IN** Client::GetLocalAddr()
{
	return &m_laddr;
}

 SOCKADDR_IN** Client::GetRemoteAddr()
{
	return &m_raddr;
}

 size_t Client::GetBufSize()
{
	return m_buffer.size();
}

 LPDWORD Client::GetFlags()
{
	return &m_flags;
}

 LPWSABUF Client::GetRecvWSABuf()
{
	return &m_recv->m_wsabuf;
}

 LPWSABUF Client::GetSendWSABuf()
{
	return &m_send->m_wsabuf;
}