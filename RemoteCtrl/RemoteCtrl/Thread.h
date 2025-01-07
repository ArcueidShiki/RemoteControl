#pragma once
#include <atomic>
#include <vector>
#include <mutex>


class ThreadFuncBase
{
public:

};

typedef int (ThreadFuncBase::* FUNC)();

class ThreadWorker
{
public:
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
	BOOL IsIdle();
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
	ThreadPool() {}
	ThreadPool(size_t size);
	~ThreadPool();
	BOOL Invoke();
	void Stop();
	int DispatchWorker(const ThreadWorker& worker);
	BOOL CheckThreadValid(size_t index);
private:
	std::mutex m_lock;
	std::vector<CThread> m_threads;
};