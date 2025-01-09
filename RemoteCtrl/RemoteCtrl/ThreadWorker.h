#pragma once

class ThreadFuncBase
{
};

typedef int (ThreadFuncBase::* FUNC)();

class ThreadWorker
{
public:
	ThreadWorker();
	~ThreadWorker();
	ThreadWorker(void* obj, FUNC f);
	ThreadWorker(const ThreadWorker& other);
	ThreadWorker& operator=(const ThreadWorker& other);
	ThreadWorker(ThreadWorker&& other) noexcept;
	ThreadWorker& operator=(ThreadWorker&& other) noexcept;
	int operator()() const;
	BOOL IsValid() const;
private:
	ThreadFuncBase* base;
	FUNC func; // member function pointer
};