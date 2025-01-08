#include "pch.h"
#include "ThreadPool.h"

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
	StopPool();
	m_threads.clear();
}

BOOL ThreadPool::Invoke()
{
	BOOL ret = TRUE;
	for (size_t i = 0; i < m_threads.size(); i++)
	{
		// Start failed
		if (!m_threads[i] || !m_threads[i]->Start())
		{
			ret = FALSE;
			break;
		}
	}
	if (!ret) StopPool();
	return ret;
}

void ThreadPool::StopPool()
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

int ThreadPool::DispatchWorker(::ThreadWorker* pWorker)
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
		// TODO IsIdle logic has problems
		if (m_threads[i] && m_threads[i]->IsIdle())
		{
			m_threads[i]->UpdateWorker(pWorker);
			index = int(i);
			break;
		}
	}
	// Don't need release memory here,
	// let the sub thread relese after using.
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
