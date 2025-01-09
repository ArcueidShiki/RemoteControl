#include "pch.h"
#include "Client.h"
#include "Utils.h"
#include "Packet.h"

using SEND_CALLBACK = SendQueue<std::vector<char>>::CB;

Client::Client()
	: m_qSend(this, (SEND_CALLBACK)&Client::SendData)
{
	m_socket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024, 0);
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
}

Client::~Client()
{
	if (m_lock.load()) return;
	m_lock.exchange(TRUE);
	closesocket(m_socket);
	m_buffer.clear();
	m_laddr = NULL;
	m_raddr = NULL;
	m_inUse = FALSE;
	m_accept.reset();
	m_recv.reset();
	m_send.reset();
	m_error.reset();
	m_received = 0;
	m_flags = 0;
	m_used = 0;
	m_qSend.Clear();
}

int Client::Recv()
{
	//int recved = recv(m_socket, m_recv->m_buffer.data() + m_used, int(m_recv->m_buffer.size() - m_used), 0);
	//if (recved <= 0) return -1;
	//m_used += recved;
	//CUtils::Dump((BYTE*)m_recv->m_buffer.data(), m_received);
	//CUtils::Dump((BYTE*)m_recv->m_wsabuf.buf, m_received);
	//TRACE("Get : [%lu] Bytes From Client; recv buff position:[%p], buf size:[%zu]\n", m_received, m_recv->m_buffer.data(), m_recv->m_buffer.size());
	//TRACE("Get : [%lu] Bytes From Client; recv  wsabuff position:[%p], wsabuf size:[%zu]\n", m_received, m_recv->m_wsabuf.buf, m_recv->m_wsabuf.len);
	return -1;
}

int Client::Send(void* buf, size_t size)
{
	std::vector<char> vec(size);
	memcpy(vec.data(), buf, size);
	if (m_qSend.PushBack(vec))
	{
		return 0;
	}
	return -1;
}

void Client::SetOverlapped(Client* ptr)
{
	m_accept->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
	m_error->m_client = ptr;
}

int Client::SendData(std::vector<char>& data)
{
	if (m_qSend.Size())
	{
		int ret = WSASend(m_socket, GetSendWSABuf(),
			1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0)
		{
			int err = WSAGetLastError();
			TRACE("Client, Send Queue err: %d\n", err);
			if (err != WSA_IO_PENDING)
			{
				CUtils::ShowError();
				return ret;
			}
		}
	}
	return 0;
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

DWORD& Client::GetFlags()
{
	return m_flags;
}

LPWSABUF Client::GetRecvWSABuf()
{
	return &m_recv->m_wsabuf;
}

LPWSABUF Client::GetSendWSABuf()
{
	return &m_send->m_wsabuf;
}