#pragma once
#include <atomic>
#include <vector>
#include <mutex>
#include "ThreadWorker.h"

class CThread
{
public:
	CThread();
	~CThread();
	static void ThreadEntry(void* arg);
	BOOL Start();
	void UpdateWorker(const ::ThreadWorker &worker = ::ThreadWorker());
private:
	friend class ThreadPool;
	BOOL IsValid();
	BOOL IsIdle();
	BOOL Stop();
	void ThreadWorker();
	HANDLE m_hThread;
	std::atomic<BOOL> m_aRunning;
	// it can only be trivial copyable, potential memory leak with pointer
	std::atomic<const ::ThreadWorker*> m_pWorker;
};