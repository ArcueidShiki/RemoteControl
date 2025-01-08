#include "pch.h"
#include "ThreadWorker.h"

ThreadWorker::ThreadWorker()
{
	base = NULL;
	func = NULL;
}

ThreadWorker::~ThreadWorker()
{
	base = NULL;
	func = NULL;
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