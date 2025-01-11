#pragma once
#include "Queue.h"

template<class T>
class SendQueue : public CQueue<T>
{
public:
	typedef int (ThreadFuncBase::* CB)(T& data /* = CPacket */);
	SendQueue(ThreadFuncBase* obj /* = *client */, CB callback /* = Client::SendData*/);
	virtual ~SendQueue();
protected:
	// using timer to auto pop dispatch message instead of parent popfront
	bool PopFront();
	// prohibit this popfront function.
	//virtual bool PopFront(T& data) { return FALSE; }
	virtual void HandleOpt(typename CQueue<T>::Q_IOCP_PARAM* pParam);
	virtual void HandlePop(typename CQueue<T>::Q_IOCP_PARAM* pParam);
private:
	ThreadFuncBase* m_base;
	CB m_callback;
	CThread m_thread;
	ThreadWorker m_worker;
};

template<class T>
inline SendQueue<T>::SendQueue(ThreadFuncBase* obj, CB callback)
	: CQueue<T>()
{
	m_base = obj;
	m_callback = callback;
	//m_thread.Start();
	//m_worker = ThreadWorker(this, (FUNC)&SendQueue<T>::ThreadTick);
	//m_thread.UpdateWorker(m_worker);
}

template<class T>
inline SendQueue<T>::~SendQueue()
{
	if (CQueue<T>::m_aStop.load()) return;
	CQueue<T>::Clear();
	m_base = NULL;
	m_callback = NULL;
	//m_thread.Stop(); you don't need to call stop, it will be called at deconstructor.
}

template<class T>
inline bool SendQueue<T>::PopFront()
{
	if (CQueue<T>::m_aStop.load())
	{
		return FALSE;
	}
	auto pParam = typename CQueue<T>::Q_IOCP_PARAM(CQueue<T>::QPOP, T());
	if (!PostQueuedCompletionStatus(CQueue<T>::m_hCompletionPort,
		sizeof(CQueue<T>::Q_IOCP_PARAM), (ULONG_PTR)&pParam, NULL))
	{
		//delete pParam;
		return FALSE;
	}
	// pop one packet,
	// send one packet,

	return TRUE;
}

template<class T>
inline void SendQueue<T>::HandleOpt(typename CQueue<T>::Q_IOCP_PARAM* pParam)
{
	switch (pParam->nOpt)
	{
	case CQueue<T>::QPUSH: return CQueue<T>::HandlePush(pParam);
	case CQueue<T>::QPOP: return HandlePop(pParam);
	case CQueue<T>::QSIZE: return CQueue<T>::HandleSize(pParam);
	case CQueue<T>::QCLEAR: return CQueue<T>::HandleClear(pParam);
	default: OutputDebugStringA("Unknown operation\n"); break;
	}
}

template<class T>
inline void SendQueue<T>::HandlePop(typename CQueue<T>::Q_IOCP_PARAM* pParam)
{
	if (!CQueue<T>::m_lstData.empty())
	{
		pParam->data = CQueue<T>::m_lstData.front();
		// async handle m_callback: Client::SendData(CPacket& packet)
		if ((m_base->*m_callback)(pParam->data) == 0)
		{
			CQueue<T>::m_lstData.pop_front();
		}
	}
	//delete pParam;
}

//template<class T>
//inline int SendQueue<T>::ThreadTick()
//{
//	// thread is is running
//	bool workerThreadRunning = WaitForSingleObject(CQueue<T>::m_hThread, 0) == WAIT_TIMEOUT;
//	if (!workerThreadRunning) return -1;
//	while (TRUE)
//	{
//		PopFront(); // post message to iocp
//		Sleep(1);
//	}
//	return 0;
//}