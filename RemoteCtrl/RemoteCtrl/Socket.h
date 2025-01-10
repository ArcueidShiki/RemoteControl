#pragma once
#include <WinSock2.h>
#include <memory>
#include <WS2tcpip.h>

#define ADDR_SIZE (sizeof(SOCKADDR_IN))

class Buffer : public std::string
{
public:
	Buffer(size_t size = 0);
	Buffer(void* buf, size_t size);
	Buffer(const char* str);
	~Buffer();
	operator char* ();
	operator const char* () const;
	operator BYTE* () const;
	operator void* () const;
	Buffer& operator=(const char* str);
	void Update(void* buf, size_t size);
};

class SocketAddrIn
{
public:
	SocketAddrIn();
	SocketAddrIn(ULONG ip, USHORT port);
	SocketAddrIn(const char* sIP, USHORT port);
	SocketAddrIn(const sockaddr_in addr);
	SocketAddrIn(const SocketAddrIn& other);
	SocketAddrIn& operator=(const SocketAddrIn& other);
	inline operator SOCKADDR* () const;
	inline operator socklen_t* () const;
	const char* GetIPStr() const;
	USHORT GetPort() const;
	void Update();
private:
	SOCKADDR_IN m_addr;
	char m_sIP[16];
	USHORT m_port;
	ULONG m_nIP;
	socklen_t size;
};

enum class PTYPE
{
	TCP = 1,
	UDP,
};
class CSocket
{
public:
	CSocket(PTYPE type = PTYPE::TCP, int nProtocol = 0);
	CSocket(const CSocket& other);
	CSocket& operator=(const CSocket& other);
	BOOL operator==(SOCKET s) const;
	operator SOCKET() const;
	~CSocket();
	void Close();
	int Listen(int backlog = 5);
	int Bind(const char* ip, USHORT port);
	//int Accept();
	//int Connect(const char* ip, USHORT port);
	int Send(const Buffer &buf);
	int Recv(Buffer &buf);
	int SendTo(const Buffer &buf, SocketAddrIn &to);
	int RecvFrom(Buffer &buf, SocketAddrIn &from);
private:
	SOCKET m_socket;
	PTYPE m_type;
	int m_protocol;
	SocketAddrIn m_addr;
};

using SOCKSPTR = std::shared_ptr<CSocket>;

