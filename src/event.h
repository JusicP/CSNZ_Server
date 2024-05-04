#pragma once

#include "interface/ievent.h"
#include "common/thread.h"

/**
 * Representation of event as a function
 */
class CEvent_Function : public IEvent
{
public:
	CEvent_Function(const std::function<void()>& func, EventFunctionType type = EventFunctionType::NotSpecified)
	{
		m_Func = func;
		m_Type = type;
	}

	virtual void Execute()
	{
		m_Func();
	}

	EventFunctionType GetType()
	{
		return m_Type;
	}

private:
	EventFunctionType m_Type;
	std::function<void()> m_Func;
};

class CReceiveMessage;

/**
 * Separate event for TCP packet.
 * We need an extended socket object to be able to remove events associated with it.
 */
class CEvent_TCPPacket : public CEvent_Function
{
public:
	CEvent_TCPPacket(IExtendedSocket* socket, const std::function<void()>& func) : CEvent_Function(func, EventFunctionType::TCPPacket)
	{
		m_pSocket = socket;
	}

	IExtendedSocket* GetSocket()
	{
		return m_pSocket;
	}

private:
	IExtendedSocket* m_pSocket;
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

	void AddEventTCPPacket(IExtendedSocket* socket, const std::function<void()>& func)
	{
		CEvent_TCPPacket* ev = new CEvent_TCPPacket(socket, func);
		AddEvent(ev);
	}

	/**
	 * Removes all events associated with socket obj 
	 */
	void RemoveEventsBySocket(IExtendedSocket* socket)
	{
		m_Events.erase(std::remove_if(m_Events.begin(), m_Events.end(),
			[socket](IEvent* ev)
			{
				if (ev->GetType() == EventFunctionType::TCPPacket)
				{
					CEvent_TCPPacket* evPacket = static_cast<CEvent_TCPPacket*>(ev);
					if (evPacket->GetSocket() == socket)
					{
						delete evPacket; /// @todo: use unique_ptr...
						return true;
					}
				}

				return false;
			}
		), m_Events.end());
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