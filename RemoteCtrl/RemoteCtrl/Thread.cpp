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
	if (!m_hThread || m_hThread == INVALID_HANDLE_VALUE) return FALSE;
	return WaitForSingleObject(m_hThread, INFINITE) != WAIT_TIMEOUT;
}

BOOL CThread::Start()
{
	m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
	if (IsValid()) m_bRunning = TRUE;
	return m_bRunning;
}

BOOL CThread::Stop()
{
	if (!m_bRunning) return TRUE;
	m_bRunning = FALSE;
	auto pWorker = m_pWorker.load();
	if (pWorker)
	{
		delete pWorker;
		pWorker = NULL;
	}
	return WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
}

BOOL CThread::IsIdle()
{
	// potential problems
	auto pWorker = m_pWorker.load();
	return !pWorker || !pWorker->IsValid();
}

void CThread::UpdateWorker(::ThreadWorker *pWorker)
{
	if (!pWorker) return;
	if (!pWorker->IsValid())
	{
		delete pWorker;
		pWorker = NULL;
		return;
	}
	auto pOldWorker = m_pWorker.exchange(pWorker);
	if (pOldWorker)
	{
		delete pOldWorker;
		pOldWorker = NULL;
	}
}

void CThread::ThreadWorker()
{
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
			int ret = (*pWorker)();
			if (ret < 0)
			{
				auto pOldWorker = m_pWorker.exchange(NULL);
				if (pOldWorker)
				{
					delete pOldWorker;
					pOldWorker = NULL;
				}
			}
			else if (ret > 0)
			{
				//CString str;
				//str.Format(_T("Thread found warning code %d\r\n"), ret);
				//OutputDebugString(str); // cause program cannot exit
			}
		}
		else
		{
			Sleep(1);
		}
	}
}