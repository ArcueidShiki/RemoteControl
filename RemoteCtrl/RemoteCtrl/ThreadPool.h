#pragma once
#include "Thread.h"

class ThreadPool
{
public:
	ThreadPool() {}
	ThreadPool(size_t size);
	~ThreadPool();
	BOOL Invoke();
	void StopPool();
	int DispatchWorker(const ThreadWorker &worker);
	BOOL CheckThreadValid(size_t index);
private:
	std::mutex m_lock;
	std::vector<CThread*> m_threads;
};