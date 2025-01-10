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
	virtual ~CQueue();
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
		//_beginthread_proc_type cbFunc;
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
	virtual void HandleOpt(PPARAM* pParam);
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<BOOL> m_aRunning;
private:
	static void ThreadEntry(void* arg); // arg: m_hCompletionPort
	void ThreadQueue();
};

// no explicit instantiation
// so you can only put implementation in header file
template<class T>
inline CQueue<T>::CQueue()
{
	m_aRunning.store(FALSE);
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
	if (m_aRunning.load())
	{
		return;
	}
	Clear();
	m_aRunning.exchange(TRUE);
	// notify thread to exit
	PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	// wait for thread exit
	if (WaitForSingleObject(m_hThread, 500) != WAIT_OBJECT_0)
	{
		TerminateThread(m_hThread, 0);
	}
	m_hThread = INVALID_HANDLE_VALUE;
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
	if (m_aRunning.load())
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
	if (m_aRunning.load())
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
	if (m_aRunning.load())
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
	if (m_aRunning.load())
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
	self->ThreadQueue();
	_endthread();
}

template<class T>
inline void CQueue<T>::ThreadQueue()
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
			printf("Queue Thread is prepare to exit\n");
			break;
		}
		PPARAM* pParam = (PPARAM*)CompletionKey;
		HandleOpt(pParam);
	}
	while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0))
	{
		if (dwTransferred == 0 || CompletionKey == 0)
		{
			continue;
		}
		PPARAM* pParam = (PPARAM*)CompletionKey;
		HandleOpt(pParam);
	}
	CloseHandle(m_hCompletionPort);
	m_hCompletionPort = NULL;
}

template<class T>
inline void CQueue<T>::HandleOpt(PPARAM* pParam)
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