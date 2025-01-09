#include "pch.h"
#include "Thread.h"

CThread::CThread()
{
	m_bRunning = FALSE;
	m_hThread = INVALID_HANDLE_VALUE;
	m_pWorker.store(NULL);
}

CThread::~CThread()
{
	Stop();
}

void CThread::ThreadEntry(void* arg)
{
	CThread* self = (CThread*)arg;
	if (self) self->ThreadWorker();
	_endthread();
}

BOOL CThread::IsValid()
{
	if (!m_hThread || m_hThread == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	// a thread is not in singled state means, it still running
	// otherwise, it has finished, been signaled notify others.
	return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
}

BOOL CThread::Start()
{
	m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
	if (IsValid()) m_bRunning = TRUE;
	return m_bRunning;
}

BOOL CThread::Stop()
{
	m_bRunning = FALSE;
	// wait failed
	if (WaitForSingleObject(m_hThread, 1000) != WAIT_OBJECT_0)
	{
		TerminateThread(m_hThread, 0);
	}
	return TRUE;
}

BOOL CThread::IsIdle()
{
	auto pWorker = m_pWorker.load();
	BOOL ret = !pWorker || !pWorker->IsValid();
	return ret;
}

void CThread::UpdateWorker(const ::ThreadWorker& worker)
{
	if (!worker.IsValid()) return;
	m_pWorker.exchange(&worker);
}

void CThread::ThreadWorker()
{
	m_bRunning = TRUE;
	while (m_bRunning)
	{
		auto pWorker = m_pWorker.load();
		if (!pWorker)
		{
			Sleep(1);
			continue;
		}
		if (pWorker->IsValid())
		{
			if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
			{
				// call worker method;
				int ret = (*pWorker)();
				if (ret != 0)
				{
					CString str;
					str.Format(_T("Thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}
				if (ret < 0)
				{
					m_pWorker.exchange(NULL);
				}
			}
		}
		else
		{
			Sleep(1);
		}
	}
}