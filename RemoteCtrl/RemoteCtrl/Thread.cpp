#include "pch.h"
#include "Thread.h"

CThread::CThread()
{
	m_aRunning.store(FALSE);
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
	if (IsValid()) m_aRunning.store(TRUE);
	return m_aRunning.load();
}

BOOL CThread::Stop()
{
	m_aRunning.store(FALSE);
	TerminateThread(m_hThread, 0);
#if 0
	if (WaitForSingleObject(m_hThread, 500) != WAIT_OBJECT_0)
	{
		TerminateThread(m_hThread, 0);
	}
#endif
	m_hThread = INVALID_HANDLE_VALUE;
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
	m_aRunning.store(TRUE);
	while (m_aRunning.load())
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