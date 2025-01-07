#include "pch.h"
#include "Thread.h"

CThread::CThread()
{
	m_bRunning = FALSE;
	m_hThread = INVALID_HANDLE_VALUE;
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
	return WaitForSingleObject(m_hThread, 0) != WAIT_TIMEOUT;
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
	return WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
}

BOOL CThread::IsIdle()
{
	return m_pWorker.load()->IsValid();
}

void CThread::UpdateWorker(::ThreadWorker *pWorker)
{
	if (!pWorker->IsValid())
	{
		delete pWorker;
		pWorker = NULL;
		return;
	}
	m_pWorker.exchange(pWorker);
}

void CThread::ThreadWorker()
{
	while (m_bRunning)
	{
		auto worker = m_pWorker.load();
		if (worker->IsValid())
		{
			int ret = (*worker)();
			if (ret < 0)
			{
				m_pWorker.store(NULL);
			}
			else if (ret > 0)
			{
				CString str;
				str.Format(_T("Thread found warning code %d\r\n"), ret);
				OutputDebugString(str);
			}
		}
		else
		{
			Sleep(1);
		}
	}
}

ThreadWorker::ThreadWorker()
{
	base = NULL;
	func = NULL;
}

ThreadWorker::~ThreadWorker()
{
}

ThreadWorker::ThreadWorker(ThreadFuncBase* obj, FUNC f)
{
	base = obj;
	func = f;
}

ThreadWorker::ThreadWorker(const ThreadWorker& other)
{
	base = other.base;
	func = other.func;
}

ThreadWorker& ThreadWorker::operator=(const ThreadWorker& other)
{
	if (this != &other)
	{
		base = other.base;
		func = other.func;
	}
	return *this;
}

ThreadWorker::ThreadWorker(ThreadWorker&& other) noexcept
{
	base = other.base;
	func = other.func;
}

ThreadWorker& ThreadWorker::operator=(ThreadWorker&& other) noexcept
{
	base = other.base;
	func = other.func;
	return *this;
}

BOOL ThreadWorker::IsValid() const
{
	return base && func;
}

int ThreadWorker::operator()()
{
	if (IsValid()) return (base->*func)();
	return -1;
}

ThreadPool::ThreadPool(size_t size)
{
	m_threads.resize(size);
	for (size_t i = 0; i < size; i++)
	{
		m_threads[i] = new CThread();
	}
}

ThreadPool::~ThreadPool()
{
	Stop();
	m_threads.clear();
}

BOOL ThreadPool::Invoke()
{
	BOOL ret = TRUE;
	for (size_t i = 0; i < m_threads.size(); i++)
	{
		if (!m_threads[i]->Start())
		{
			ret = FALSE;
			break;
		}
	}
	if (!ret) Stop();
	return ret;
}

void ThreadPool::Stop()
{
	for (size_t i = 0; i < m_threads.size(); i++)
	{
		if (m_threads[i])
		{
			m_threads[i]->Stop();
			delete m_threads[i];
			m_threads[i] = NULL;
		}
	}
}

int ThreadPool::DispatchWorker(::ThreadWorker *pWorker)
{
	if (!pWorker)
	{
		return -1;
	}
	int index = -1;
	m_lock.lock();
	for (size_t i = 0; i < m_threads.size(); i++)
	{
		// using stack or queue to handle idle thread status is better
		if (m_threads[i]->IsIdle())
		{
			m_threads[i]->UpdateWorker(pWorker);
			index = int(i);
			break;
		}
	}
	if (pWorker)
	{
		delete pWorker;
		pWorker = NULL;
	}
	m_lock.unlock();
	return index;
}

BOOL ThreadPool::CheckThreadValid(size_t index)
{
	if (index < m_threads.size())
	{
		return m_threads[index]->IsValid();
	}
	return FALSE;
}
