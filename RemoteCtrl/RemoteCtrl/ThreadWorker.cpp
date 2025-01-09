#include "pch.h"
#include "ThreadWorker.h"

ThreadWorker::ThreadWorker()
{
	base = NULL;
	func = NULL;
}

ThreadWorker::~ThreadWorker()
{
	//base = NULL; cannot be null, it's has other references
	func = NULL;
}

ThreadWorker::ThreadWorker(void* obj, FUNC f)
{
	base = (ThreadFuncBase*)obj;
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
	BOOL ret = base && func;
	return ret;
}

int ThreadWorker::operator()() const
{
	if (IsValid()) {
		int ret = (base->*func)();
		//TRACE("Worker work result = %d\n", ret);
		return ret;
	}
	return -1;
}