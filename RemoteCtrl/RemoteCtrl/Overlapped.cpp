#include "pch.h"
#include "Overlapped.h"

COverlapped::COverlapped()
{
	m_operator = 0;
	m_overlapped = { 0 };
	m_buffer.resize(1024);
	m_worker = ThreadWorker();
	m_server = NULL;
	m_client = NULL;
	m_wsabuf = WSABUF();
	m_wsabuf.buf = m_buffer.data();
	m_wsabuf.len = ULONG(m_buffer.size());
}

COverlapped::~COverlapped()
{
}