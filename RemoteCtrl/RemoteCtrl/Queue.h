#pragma once
// template better define in header: visibility, linking, code reuse
#include <list>
#include <atomic>
#include "Thread.h"

template<class T>
class CQueue
{
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

	// POST parameter for IOCP message
	typedef struct IocpParam
	{
		int nOpt;
		T data;
		HANDLE hEvent; // for pop operation
		IocpParam(int nOpt, const T& data, HANDLE event = NULL)
			: nOpt(nOpt)
			, data(data)
			, hEvent(event) {}
		IocpParam()
		{
			nOpt = QNONE;
			hEvent = NULL;
		}
	} Q_IOCP_PARAM;

protected:
	virtual void HandleOpt(Q_IOCP_PARAM* pParam);
	virtual void HandlePush(Q_IOCP_PARAM* pParam);
	virtual void HandlePop(Q_IOCP_PARAM* pParam);
	virtual void HandleSize(Q_IOCP_PARAM* pParam);
	virtual void HandleClear(Q_IOCP_PARAM* pParam);
	BOOL IsValid();
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<BOOL> m_aStop;

private:
	static void ThreadEntry(void* instance);
	void ThreadQueue();
};

// T no explicit instantiation
// can only put implementation in header file
template<class T>
inline CQueue<T>::CQueue()
{
	m_aStop.store(FALSE);
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
	if (m_aStop.load()) return;
	Clear();
	m_aStop.exchange(TRUE);
	// notify ThreadQueue to exit
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
	if (!IsValid()) return FALSE;
	Q_IOCP_PARAM* pParam = new Q_IOCP_PARAM(QPUSH, data);
	if (!PostQueuedCompletionStatus(m_hCompletionPort, sizeof(Q_IOCP_PARAM), (ULONG_PTR)pParam, NULL))
	{
		delete pParam;
		return FALSE;
	}
	return TRUE;
}

template<class T>
inline BOOL CQueue<T>::PopFront(T& data)
{
	if (!IsValid()) return FALSE;
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent || hEvent == INVALID_HANDLE_VALUE) return FALSE;
	Q_IOCP_PARAM pParam(QPOP, data, hEvent);
	if (!PostQueuedCompletionStatus(m_hCompletionPort, sizeof(Q_IOCP_PARAM), (ULONG_PTR)&pParam, NULL))
	{
		CloseHandle(hEvent);
		return FALSE;
	}
	if (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0) data = pParam.data;
	CloseHandle(hEvent);
	return TRUE;
}

template<class T>
inline size_t CQueue<T>::Size()
{
	if (!IsValid()) return FALSE;
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent || hEvent == INVALID_HANDLE_VALUE) return -1;
	if (!m_hCompletionPort)
	{
		CloseHandle(hEvent);
		return -1;
	}
	Q_IOCP_PARAM pParam(QSIZE, T(), hEvent);
	if (!PostQueuedCompletionStatus(m_hCompletionPort, sizeof(Q_IOCP_PARAM), (ULONG_PTR)&pParam, NULL))
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
	if (!IsValid()) return FALSE;
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hEvent || hEvent == INVALID_HANDLE_VALUE) return FALSE;
	Q_IOCP_PARAM pParam(QCLEAR, T(), hEvent);
	BOOL ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(Q_IOCP_PARAM), (ULONG_PTR)&pParam, NULL);
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
inline void CQueue<T>::ThreadEntry(void* obj)
{
	CQueue<T>* instance = (CQueue<T>*)obj;
	instance->ThreadQueue();
	_endthread();
}

template<class T>
inline void CQueue<T>::ThreadQueue()
{
	Q_IOCP_PARAM* pParam = NULL;
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
		Q_IOCP_PARAM* pParam = (Q_IOCP_PARAM*)CompletionKey;
		HandleOpt(pParam);
	}
	// ~CQueue -> PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	// there still some data in queue
	while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0))
	{
		if (dwTransferred == 0 || CompletionKey == 0)
		{
			continue;
		}
		Q_IOCP_PARAM* pParam = (Q_IOCP_PARAM*)CompletionKey;
		HandleOpt(pParam);
	}
	CloseHandle(m_hCompletionPort);
	m_hCompletionPort = NULL;
}

template<class T>
inline void CQueue<T>::HandleOpt(Q_IOCP_PARAM* pParam)
{
	switch (pParam->nOpt)
	{
		case QPUSH: return HandlePush(pParam);
		case QPOP: return HandlePop(pParam);
		case QSIZE: return HandleSize(pParam);
		case QCLEAR: return HandleClear(pParam);
		default: OutputDebugStringA("Unknown operation\n"); break;
	}
}

template<class T>
inline void CQueue<T>::HandlePush(Q_IOCP_PARAM* pParam)
{
	m_lstData.push_back(pParam->data);
	delete pParam;
}

template<class T>
inline void CQueue<T>::HandlePop(Q_IOCP_PARAM* pParam)
{
	if (!m_lstData.empty())
	{
		pParam->data = m_lstData.front();
		m_lstData.pop_front();
	}
	if (pParam->hEvent) SetEvent(pParam->hEvent);
}

template<class T>
inline void CQueue<T>::HandleSize(Q_IOCP_PARAM* pParam)
{
	pParam->nOpt = m_lstData.size();
	if (pParam->hEvent) SetEvent(pParam->hEvent);
}

template<class T>
inline void CQueue<T>::HandleClear(Q_IOCP_PARAM* pParam)
{
	m_lstData.clear();
	if (pParam->hEvent) SetEvent(pParam->hEvent);
}

template<class T>
inline BOOL CQueue<T>::IsValid()
{
	if (m_aStop.load()) return FALSE;
	if (!m_hCompletionPort || m_hCompletionPort == INVALID_HANDLE_VALUE) return FALSE;
	return TRUE;
}
