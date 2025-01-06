#pragma once
#include <list>
template<class T>
class CQueue
{
	// thread safe queue using IOCP
public:
	CQueue();
	~CQueue();
	void PushBack(const T& data);
	void PopFront(T& data);
	size_t Size();
	void Clear();
public:
	enum {
		QPush,
		QPop,
		QSize,
		QClrear,
	};

	typedef struct IocpParam
	{
		int nOpt;
		std::string strData;
		_beginthread_proc_type cbFunc;
		HANDLE hEvent; // for pop operation
		IocpParam(int nOpt, const std::string& strData)
			: nOpt(nOpt), strData(strData) {}
		IocpParam() {}
	} PPARAM; // POST parameter for IOCP message
private:
	static void ThreadEntry(void* arg);
	static void ThreadMain();
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
};