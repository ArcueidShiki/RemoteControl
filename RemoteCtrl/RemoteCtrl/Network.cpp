#include "pch.h"
#include "Network.h"


NServer::NServer(const NServerParam& param)
{
	m_param = param;
	m_thread.UpdateWorker(ThreadWorker(this, (FUNC)&NServer::ThreadLoop));
    m_bRunning.store(FALSE);
    m_args = NULL;
}

NServer::~NServer()
{
	Stop();
}

int NServer::Invoke(void* arg)
{
    m_spSocket = std::make_shared<CSocket>(m_param.m_type);
	//m_spSocket.reset(new CSocket(PTYPE::UDP));

    if (*m_spSocket == INVALID_SOCKET)
    {
        SHOW_SOCKET_ERROR();
        return -1;
    }
    SocketAddrIn client;
    if (m_spSocket->Bind(m_param.m_sIP, m_param.m_port) == SOCKET_ERROR)
    {
        SHOW_SOCKET_ERROR();

        return - 2;
    }
    if (m_param.m_type == PTYPE::TCP)
    {
        if (m_spSocket->Listen() == SOCKET_ERROR)
        {
            SHOW_SOCKET_ERROR();
            return -3;
        }
    }
    if (!m_thread.Start()) return -4;
    m_args = arg;
    return 0;
}

int NServer::Send(SOCKSPTR& client, const Buffer& buf)
{
    // TODO optimization. maybe partially send
    int ret = m_spSocket->Send(buf);
    if (m_param.m_sendCallback)
    {
		m_param.m_sendCallback(m_args, client, ret);
    }
    return ret;
}

int NServer::Sendto(const Buffer& buf, SocketAddrIn& addr)
{
    // TODO optimization. maybe partially send
    int ret = m_spSocket->SendTo(buf, addr);
    if (m_param.m_sendtoCallback)
    {
        m_param.m_sendtoCallback(m_args, addr, ret);
    }
    return ret;
}

int NServer::Stop()
{
    m_spSocket->Close();
    m_spSocket.reset();
	m_bRunning.store(FALSE);
    return 0;
}

int NServer::ThreadLoop()
{
    switch (m_param.m_type)
    {
    case PTYPE::TCP:
		return ThreadTcpLoop();
	case PTYPE::UDP:
		return ThreadUdpLoop();
    default:
        return -1;
    }
}

int NServer::ThreadUdpLoop()
{
    Buffer buf(1024 * 256);
    SocketAddrIn client;
    int ret = 0;
    m_bRunning.store(TRUE);
    while (m_bRunning.load())
    {
        ret = m_spSocket->RecvFrom(buf, client);
        if (ret >= 0)
        {
            if (m_param.m_recvfromCallback)
            {
                m_param.m_recvfromCallback(m_args, buf, client);
            }
#if 0
            if (lstClientAddrs.empty())
            {
                lstClientAddrs.push_back(client);
                //CUtils::Dump((BYTE*)buf, ret);
                printf("%s(%d):%s Server Receive From Host Client ip:[%s], port:[%d]\n", __FILE__, __LINE__, __FUNCTION__, client.GetIPStr(), client.GetPort());
                ret = pSocks->SendTo(buf, client);
                //ret = sendto(*pSocks, buf, ret, 0, client, ADDR_SIZE);
                printf("%s(%d):%s Server Socket Send To Host Client ret:[%d]\n", __FILE__, __LINE__, __FUNCTION__, ret);
            }
            else
            {
                //memcpy(buf, (SOCKADDR*)lstClientAddrs.front(), ADDR_SIZE);
                buf.Update((void*)&lstClientAddrs.front(), ADDR_SIZE);
                pSocks->SendTo(buf, client);
                //ret = sendto(*pSocks, buf, ADDR_SIZE, 0, client, ADDR_SIZE);
                printf("%s(%d):%s Server Socket Send To Servant Client ret:[%d]\n", __FILE__, __LINE__, __FUNCTION__, ret);
            }
#endif
        }
        else
        {
            printf("%s(%d):%s Server Socket Error(%d), ret:[%d]\n", __FILE__, __LINE__, __FUNCTION__, ret, WSAGetLastError());
            break;
        }
    }
	m_bRunning.store(FALSE);
    m_spSocket->Close();
    return 0;
}

int NServer::ThreadTcpLoop()
{
    return 0;
}

NServerParam::NServerParam(const NServerParam& other)
{
    strcpy_s(m_sIP, other.m_sIP);
	m_port = other.m_port;
    m_type = other.m_type;
	m_acceptCallback = other.m_acceptCallback;
	m_recvCallback = other.m_recvCallback;
	m_sendCallback = other.m_sendCallback;
	m_recvfromCallback = other.m_recvfromCallback;
	m_sendtoCallback = other.m_sendtoCallback;
}

NServerParam& NServerParam::operator=(const NServerParam& other)
{
    if (this != &other)
    {
        strcpy_s(m_sIP, other.m_sIP);
        m_port = other.m_port;
        m_type = other.m_type;
        m_acceptCallback = other.m_acceptCallback;
        m_recvCallback = other.m_recvCallback;
        m_sendCallback = other.m_sendCallback;
        m_recvfromCallback = other.m_recvfromCallback;
        m_sendtoCallback = other.m_sendtoCallback;
    }
    return *this;
}

NServerParam::NServerParam(const char* sIP, USHORT port, PTYPE type)
{
    strcpy_s(m_sIP, sIP);
	m_port = port;
	m_type = type;
	m_acceptCallback = NULL;
	m_recvCallback = NULL;
	m_sendCallback = NULL;
	m_recvfromCallback = NULL;
	m_sendtoCallback = NULL;
}

NServerParam::NServerParam(const char* sIP, USHORT port, PTYPE type, AcceptCallback accept, RecvCallback recv, SendCallback send, RecvFromCallback recvfrom, SendToCallback sendto)
{
    strcpy_s(m_sIP, sIP);
	m_port = port;
	m_type = type;
	m_acceptCallback = accept;
	m_recvCallback = recv;
	m_sendCallback = send;
	m_recvfromCallback = recvfrom;
	m_sendtoCallback = sendto;
}

NServerParam& NServerParam::operator<<(AcceptCallback func)
{
    m_acceptCallback = func;
    return *this;
}

NServerParam& NServerParam::operator<<(RecvCallback func)
{
	m_recvCallback = func;
    return *this;
}

NServerParam& NServerParam::operator<<(SendCallback func)
{
	m_sendCallback = func;
    return *this;
}

NServerParam& NServerParam::operator<<(RecvFromCallback func)
{
	m_recvfromCallback = func;
    return *this;
}

NServerParam& NServerParam::operator<<(SendToCallback func)
{
    m_sendtoCallback = func;
    return *this;
}

NServerParam& NServerParam::operator<<(const char* sIP)
{
	strcpy_s(m_sIP, sIP);
    return *this;
}

NServerParam& NServerParam::operator<<(USHORT port)
{
    m_port = port;
    return *this;
}

NServerParam& NServerParam::operator<<(PTYPE type)
{
	m_type = type;
    return *this;
}

// -------------input----------------
// -------------output---------------

NServerParam& NServerParam::operator>>(AcceptCallback& func)
{
    func = m_acceptCallback;
    return *this;
}

NServerParam& NServerParam::operator>>(RecvCallback& func)
{
    func = m_recvCallback;
    return *this;
}

NServerParam& NServerParam::operator>>(SendCallback& func)
{
    func = m_sendCallback;
    return *this;
}

NServerParam& NServerParam::operator>>(RecvFromCallback& func)
{
    func = m_recvfromCallback;
    return *this;
}

NServerParam& NServerParam::operator>>(SendToCallback& func)
{
    func = m_sendtoCallback;
    return *this;
}

NServerParam& NServerParam::operator>>(char* sIP)
{
    memcpy(sIP, m_sIP, sizeof(m_sIP));
    return *this;
}

NServerParam& NServerParam::operator>>(USHORT& port)
{
    port = m_port;
    return *this;
}

NServerParam& NServerParam::operator>>(PTYPE& type)
{
    type = m_type;
    return *this;
}
