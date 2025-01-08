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
	void UpdateWorker(ThreadWorker* pWorker = new ::ThreadWorker());
private:
	friend class ThreadPool;
	BOOL IsValid();
	BOOL IsIdle();
	BOOL Stop();
	void ThreadWorker();
	HANDLE m_hThread;
	BOOL m_bRunning;
	// it can only be trivial copyable, potential memory leak with pointer
	std::atomic<::ThreadWorker*> m_pWorker;
};