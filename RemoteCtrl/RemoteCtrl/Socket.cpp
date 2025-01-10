#include "pch.h"
#include "Socket.h"

CSocket::CSocket(PTYPE type, int nProtocol)
{
	m_socket = socket(PF_INET, int(type), nProtocol);
	m_type = type;
	m_protocol = nProtocol;
	m_addr = {};
}

CSocket::CSocket(const CSocket& other)
{
	m_type = other.m_type;
	m_protocol = other.m_protocol;
	m_socket = socket(PF_INET, int(m_type), m_protocol);
	m_addr = other.m_addr;
}

CSocket& CSocket::operator=(const CSocket& other)
{
	if (this != &other)
	{
		m_type = other.m_type;
		m_protocol = other.m_protocol;
		m_socket = socket(PF_INET, int(m_type), m_protocol);
		m_addr = other.m_addr;
	}
	return *this;
}

BOOL CSocket::operator==(SOCKET s) const
{
	return m_socket == s;
}

CSocket::~CSocket()
{
	Close();
}

void CSocket::Close()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

int CSocket::Listen(int backlog)
{
	if (m_type != PTYPE::TCP) return - 1;
	return ::listen(m_socket, backlog);
}

int CSocket::Bind(const char* ip, USHORT port)
{
	m_addr = SocketAddrIn(ip, port);
	return bind(m_socket, m_addr, ADDR_SIZE);
}

int CSocket::Send(const Buffer& buf)
{
	return send(m_socket, buf, (int)buf.size(), 0);
}

int CSocket::Recv(Buffer& buf)
{
	return recv(m_socket, buf, (int)buf.size(), 0);
}

int CSocket::SendTo(const Buffer& buf, SocketAddrIn& to)
{
	return sendto(m_socket, buf, (int)buf.size(), 0, to, ADDR_SIZE);
}

int CSocket::RecvFrom(Buffer& buf, SocketAddrIn& from)
{
	int ret = recvfrom(m_socket, buf, (int)buf.size(), 0, from, from);
	if (ret > 0)
	{
		from.Update();
	}
	return ret;
}

CSocket::operator SOCKET() const
{
	return m_socket;
}

SocketAddrIn::SocketAddrIn()
{
	m_addr = {};
	m_port = 0;
	m_nIP = 0;
	memset(m_sIP, 0, sizeof(m_sIP));
	size = ADDR_SIZE;
}

SocketAddrIn::SocketAddrIn(ULONG ip, USHORT port)
{
	m_port = port;
	m_addr.sin_port = htons(m_port);

	m_nIP = ip;
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.S_un.S_addr = htonl(m_nIP);
	memset(m_sIP, 0, sizeof(m_sIP));
	inet_ntop(AF_INET, &m_addr.sin_addr, m_sIP, sizeof(m_sIP));
	size = ADDR_SIZE;
}

SocketAddrIn::SocketAddrIn(const char* sIP, USHORT port)
{
	m_port = port;
	m_addr.sin_port = htons(m_port);

	inet_pton(AF_INET, sIP, &m_addr.sin_addr);
	m_addr.sin_family = AF_INET;
	//m_addr.sin_addr.S_un.S_addr = htonl(m_nIP);
	memcpy(m_sIP, sIP, strlen(sIP));
	size = ADDR_SIZE;
}

SocketAddrIn::SocketAddrIn(const sockaddr_in addr)
{
	m_addr = addr;
	m_port = ntohs(addr.sin_port);
	inet_ntop(AF_INET, &addr.sin_addr, m_sIP, sizeof(m_sIP));
	m_nIP = ntohl(addr.sin_addr.S_un.S_addr);
	size = ADDR_SIZE;
}

SocketAddrIn::SocketAddrIn(const SocketAddrIn& other)
{
	m_addr = other.m_addr;
	m_port = other.m_port;
	m_nIP = other.m_nIP;
	memcpy(m_sIP, other.m_sIP, sizeof(m_sIP));
	size = ADDR_SIZE;
}

SocketAddrIn& SocketAddrIn::operator=(const SocketAddrIn& other)
{
	if (this != &other)
	{
		m_addr = other.m_addr;
		m_port = other.m_port;
		m_nIP = other.m_nIP;
		memcpy(m_sIP, other.m_sIP, sizeof(m_sIP));
		size = ADDR_SIZE;
	}
	return *this;
}

inline SocketAddrIn::operator SOCKADDR* () const
{
	return (SOCKADDR*)&m_addr;
}

inline SocketAddrIn::operator socklen_t* () const
{
	return (socklen_t*)&size;
}

const char* SocketAddrIn::GetIPStr() const
{
	return m_sIP;
}

USHORT SocketAddrIn::GetPort() const
{
	return m_port;
}

void SocketAddrIn::Update()
{
	m_nIP = ntohl(m_addr.sin_addr.S_un.S_addr);
	m_port = ntohs(m_addr.sin_port);
	inet_ntop(AF_INET, &m_addr.sin_addr, m_sIP, sizeof(m_sIP));
	size = sizeof(m_addr);
}

Buffer::Buffer(size_t size)
	: std::string()
{
	resize(size, 0);
	memset((void*)data(), 0, this->size());
}

Buffer::Buffer(void* buf, size_t size)
	: std::string()
{
	resize(size, 0);
	memcpy((void*)data(), buf, size);
}

Buffer::Buffer(const char* str)
{
	resize(strlen(str));
	memcpy((void*)c_str(), str, strlen(str));
}

Buffer::~Buffer()
{
	std::string::~basic_string();
}

Buffer::operator char* ()
{
	return const_cast<char*>(c_str());
}

Buffer::operator const char* () const
{
	return c_str();
}

Buffer::operator BYTE* () const
{
	return (BYTE*)c_str();
}

Buffer::operator void* () const
{
	return (void*)c_str();
}

Buffer& Buffer::operator=(const char* str)
{
	std::string::operator=(str);
	return *this;
}

void Buffer::Update(void* buf, size_t size)
{
	resize(size);
	memcpy((void*)data(), buf, size);
}
