#pragma once
#include "Queue.h"

template<class T>
class SendQueue : public CQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* CB)(T& data);
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
inline SendQueue<T>::SendQueue(ThreadFuncBase* obj, CB callback)
	: CQueue<T>()
{
	m_base = obj;
	m_callback = callback;
	m_thread.Start();
	m_thread.UpdateWorker(new ::ThreadWorker(this, (FUNC)&SendQueue<T>::ThreadTick));
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