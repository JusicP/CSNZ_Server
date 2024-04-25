#pragma once

#include "interface/ievent.h"
#include "common/thread.h"

/**
 * Representation of event as a function
 */
class CEvent_Function : public IEvent
{
public:
	CEvent_Function(const std::function<void()>& func)
	{
		m_Func = func;
	}

	virtual void Execute()
	{
		m_Func();
	}

private:
	std::function<void()> m_Func;
};

/**
 * Class that implements a queue of server events such as incoming message, second tick, console command
 * To process events, use WaitForSignal() method which waits until event is added
 */
class CEvents : public IEvents
{
public:
	CEvents()
	{
		m_pCurrentEvent = NULL;
	}

	~CEvents()
	{
		if (m_pCurrentEvent)
			delete m_pCurrentEvent;
	}

	/**
	 * Adds event and signals about it
	 */
	virtual void AddEvent(IEvent* ev)
	{
		m_Mutex.Enter();

		if (m_Events.size() >= 50)
			printf("m_Events.size() >= 50\n");

		m_Events.push_back(ev);
		m_Object.Signal();

		m_Mutex.Leave();
	}

	virtual void AddEventFunction(const std::function<void()>& func)
	{
		CEvent_Function* ev = new CEvent_Function(func);
		AddEvent(ev);
	}

	/**
	 * Get next event and delete previous one
	 */
	IEvent* GetNextEvent()
	{
		if (m_pCurrentEvent)
		{
			delete m_pCurrentEvent;
			m_pCurrentEvent = NULL;
		}

		m_Mutex.Enter();
		if (!m_Events.empty())
		{
			// get first event and remove it from vector
			m_pCurrentEvent = m_Events.front();
			m_Events.erase(m_Events.begin());
		}

		m_Mutex.Leave();

		return m_pCurrentEvent;
	}

	/**
	 * Waits until event is added
	 */
	void WaitForSignal()
	{
		m_Object.WaitForSignal();
	}

	/**
	 * Signals that event is added
	 */
	void Signal()
	{
		m_Object.Signal();
	}

private:
	std::vector<IEvent*> m_Events;
	CObjectSync m_Object;
	CCriticalSection m_Mutex;
	IEvent* m_pCurrentEvent;
};