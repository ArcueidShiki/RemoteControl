#include "pch.h"
#include "Overlapped.h"

COverlapped::COverlapped()
{
	m_operator = 0;
	m_overlapped = { 0 };
	m_buffer.resize(0);
	m_worker = ThreadWorker();
	m_server = NULL;
	m_client = NULL;
	m_wsabuf = { (ULONG)m_buffer.size(), m_buffer.data() };
}

COverlapped::~COverlapped()
{

}