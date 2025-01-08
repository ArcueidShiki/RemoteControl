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
	~ThreadWorker();
	ThreadWorker(ThreadFuncBase* obj, FUNC f);
	ThreadWorker(const ThreadWorker& other);
	ThreadWorker& operator=(const ThreadWorker& other);
	ThreadWorker(ThreadWorker&& other) noexcept;
	ThreadWorker& operator=(ThreadWorker&& other) noexcept;
	int operator()();
	BOOL IsValid() const;
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
	void UpdateWorker(ThreadWorker* pWorker = new ::ThreadWorker());
protected:
private:
	void ThreadWorker();
	HANDLE m_hThread;
	BOOL m_bRunning;
	std::atomic<::ThreadWorker*> m_pWorker; // potential memory leak
};

class ThreadPool
{
public:
	ThreadPool() {}
	ThreadPool(size_t size);
	~ThreadPool();
	BOOL Invoke();
	void Stop();
	int DispatchWorker(::ThreadWorker *pWorker);
	BOOL CheckThreadValid(size_t index);
private:
	std::mutex m_lock;
	std::vector<CThread*> m_threads;
};