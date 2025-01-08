#pragma once
// template better define in header: visibility, linking, code reuse
#include <list>
#include <atomic>
#include "Thread.h"

template<class T>
class CQueue
{
	// thread safe queue using IOCP
public:
	CQueue();
	~CQueue();
	BOOL PushBack(const T& data);
	virtual BOOL PopFront(T& data);
	BOOL Clear();
	size_t Size();
	enum {
		QNONE,
		QPUSH,
		QPOP,
		QSIZE,
		QCLEAR,
	};

	typedef struct IocpParam
	{
		size_t nOpt;
		T data;
		_beginthread_proc_type cbFunc;
		HANDLE hEvent; // for pop operation
		IocpParam(int nOpt, const T& data, HANDLE event = NULL)
			: nOpt(nOpt), data(data), hEvent(event) {}
		IocpParam()
		{
			nOpt = QNONE;
			hEvent = NULL;
		}
	} PPARAM; // POST parameter for IOCP message
protected:
	static void ThreadEntry(void* arg); // arg: m_hCompletionPort
	void ThreadMain();
	virtual void DealParam(PPARAM* pParam);
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<BOOL> m_lock;
};


template<class T>
class SendQueue : public CQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* CB)(T &data);
	SendQueue(ThreadFuncBase* obj, CB callback);
	~SendQueue();
protected:
	// using timer to auto pop dispatch message
	BOOL PopFront();
	virtual BOOL PopFront(T& data) { return FALSE; }
	virtual void DealParam(typename CQueue<T>::PPARAM* pParam);
	int ThreadTick();
private:
	ThreadFuncBase* m_base;
	CB m_callback;
	CThread m_thread;
};

template<class T>
inline CQueue<T>::CQueue()
{
	m_lock.store(FALSE);
	m_hThread = INVALID_HANDLE_VALUE;
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	if (m_hCompletionPort && m_hCompletionPort != INVALID_HANDLE_VALUE)
	{
		m_hThread = (HANDLE)_beginthread(&CQueue<T>::ThreadEntry, 0, this);
	}
}

template<class T>
inline CQueue<T>::~CQueue()
{
	if (m_lock.load())
	{
		return;
	}
	Clear();
	m_lock.exchange(TRUE);
	// notify thread to exit
	PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	// wait for thread exit
	WaitForSingleObject(m_hThread, INFINITE);
	if (m_hCompletionPort && m_hCompletionPort != INVALID_HANDLE_VALUE)
	{
		HANDLE hTmp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTmp);
	}
	m_lstData.clear();
}

template<class T>
inline BOOL CQueue<T>::PushBack(const T& data)
{
	if (m_lock.load())
	{
		return FALSE;
	}
	if (!m_hCompletionPort || m_hCompletionPort == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	PPARAM* pParam = new PPARAM(QPUSH, data);
	if (!PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL))
	{
		delete pParam;
		return FALSE;
	}
	return TRUE;
}

template<class T>
inline BOOL CQueue<T>::PopFront(T& data)
{
	if (m_lock.load())
	{
		return FALSE;
	}
	if (!m_hCompletionPort || m_hCompletionPort == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent || hEvent == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	PPARAM pParam(QPOP, data, hEvent);
	if (!PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL))
	{
		CloseHandle(hEvent);
		return FALSE;
	}
	if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
	{
		data = pParam.data;
	}
	CloseHandle(hEvent);
	return TRUE;
}

template<class T>
inline size_t CQueue<T>::Size()
{
	if (m_lock.load())
	{
		return -1;
	}
	if (!m_hCompletionPort || m_hCompletionPort == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent || hEvent == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	if (!m_hCompletionPort)
	{
		CloseHandle(hEvent);
		return -1;
	}
	PPARAM pParam(QSIZE, T(), hEvent);
	if (!PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL))
	{
		CloseHandle(hEvent);
		return -1;
	}
	if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
	{
		CloseHandle(hEvent);
		return pParam.nOpt;
	}
	CloseHandle(hEvent);
	return -1;
}

template<class T>
inline BOOL CQueue<T>::Clear()
{
	if (m_lock.load())
	{
		return FALSE;
	}
	if (!m_hCompletionPort || m_hCompletionPort == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent || hEvent == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	PPARAM pParam(QCLEAR, T(), hEvent);
	BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
	if (!ret)
	{
		CloseHandle(hEvent);
		return FALSE;
	}
	ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
	CloseHandle(hEvent);
	return ret;
}

template<class T>
inline void CQueue<T>::ThreadEntry(void* arg)
{
	CQueue<T>* self = (CQueue<T>*)arg;
	self->ThreadMain();
	_endthread();
}

template<class T>
inline void CQueue<T>::ThreadMain()
{
	PPARAM* pParam = NULL;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED Overlapped = { 0 };
	LPOVERLAPPED pOverlapped = &Overlapped;
	// wait for iocp to be signaled, otherwise sleep
	while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
	{
		if (dwTransferred == 0 || CompletionKey == 0)
		{
			printf("Thread is prepare to exit\n");
			break;
		}
		PPARAM* pParam = (PPARAM*)CompletionKey;
		DealParam(pParam);
	}
	while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0))
	{
		if (dwTransferred == 0 || CompletionKey == 0)
		{
			continue;
		}
		PPARAM* pParam = (PPARAM*)CompletionKey;
		DealParam(pParam);
	}
	CloseHandle(m_hCompletionPort);
	m_hCompletionPort = NULL;
}

template<class T>
inline void CQueue<T>::DealParam(PPARAM* pParam)
{
	switch (pParam->nOpt)
	{
	case QPUSH:
		m_lstData.push_back(pParam->data);
		delete pParam;
		break;
	case QPOP:
		if (!m_lstData.empty())
		{
			pParam->data = m_lstData.front();
			m_lstData.pop_front();
		}
		if (pParam->hEvent) SetEvent(pParam->hEvent);
		break;
	case QSIZE:
		pParam->nOpt = m_lstData.size();
		if (pParam->hEvent) SetEvent(pParam->hEvent);
		break;
	case QCLEAR:
		m_lstData.clear();
		if (pParam->hEvent) SetEvent(pParam->hEvent);
		break;
	default:
		OutputDebugStringA("Unknown operation\n");
		break;
	}
}

// -------------------------------------------------------------------------

template<class T>
inline SendQueue<T>::SendQueue(ThreadFuncBase* obj, CB callback)
	: CQueue<T>()
{
	m_base = obj;
	m_callback = callback;
	m_thread.Start();
	m_thread.UpdateWorker(new ::ThreadWorker(this, (FUNC)& SendQueue<T>::ThreadTick));
}

template<class T>
inline SendQueue<T>::~SendQueue()
{
	if (CQueue<T>::m_lock.load()) return;
	CQueue<T>::Clear();
	m_base = NULL;
	m_callback = NULL;
	//m_thread.Stop(); you don't need to call stop, it will be called at deconstructor.
}

template<class T>
inline BOOL SendQueue<T>::PopFront()
{
	if (CQueue<T>::m_lock.load())
	{
		return FALSE;
	}
	typename CQueue<T>::PPARAM* pParam = new typename CQueue<T>::PPARAM(CQueue<T>::QPOP, T());
	if (!PostQueuedCompletionStatus(CQueue<T>::m_hCompletionPort,
		sizeof(CQueue<T>::PPARAM), (ULONG_PTR)&pParam, NULL))
	{
		delete pParam;
		return FALSE;
	}
	return TRUE;
}

template<class T>
inline void SendQueue<T>::DealParam(typename CQueue<T>::PPARAM* pParam)
{
	switch (pParam->nOpt)
	{
	case CQueue<T>::QPUSH:
		CQueue<T>::m_lstData.push_back(pParam->data);
		delete pParam;
		break;
	case CQueue<T>::QPOP:
		if (!CQueue<T>::m_lstData.empty())
		{
			pParam->data = CQueue<T>::m_lstData.front();
			// async handle
			if ((m_base->*m_callback)(pParam->data))
				CQueue<T>::m_lstData.pop_front();
		}
		delete pParam;
		break;
	case CQueue<T>::QSIZE:
		pParam->nOpt = CQueue<T>::m_lstData.size();
		if (pParam->hEvent) SetEvent(pParam->hEvent);
		break;
	case CQueue<T>::QCLEAR:
		CQueue<T>::m_lstData.clear();
		if (pParam->hEvent) SetEvent(pParam->hEvent);
		break;
	default:
		OutputDebugStringA("Unknown operation\n");
		break;
	}
}

template<class T>
inline int SendQueue<T>::ThreadTick()
{
	if (!CQueue<T>::m_lstData.empty())
	{
		PopFront();
	}
	Sleep(1);
	return 0;
}