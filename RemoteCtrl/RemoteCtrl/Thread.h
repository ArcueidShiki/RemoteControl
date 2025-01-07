#pragma once
#include <atomic>
#include <vector>
class ThreadFuncBase
{
public:

};

class ThreadWorker
{
public:
	typedef int (ThreadFuncBase::* FUNC)();
	ThreadWorker();
	ThreadWorker(ThreadFuncBase* obj, FUNC f);
	ThreadWorker(const ThreadWorker& other);
	ThreadWorker& operator=(const ThreadWorker& other);
	int operator()();
	BOOL IsValid();
private:
	ThreadFuncBase *base;
	FUNC func; // member function pointer
};

class CThread
{
public:
	CThread();
	~CThread();
	static void ThreadEntry(void* arg);
	BOOL IsValid();
	BOOL Start();
	BOOL Stop();
	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker());
protected:
private:
	void ThreadWorker();
	HANDLE m_hThread;
	BOOL m_bRunning;
	std::atomic<::ThreadWorker> m_worker;
};

class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(size_t size);
	~ThreadPool();
	BOOL Invoke();
	void Stop();
	int DispatchWorker(const ThreadWorker& worker);
private:
	std::vector<CThread> m_thread;
};