#pragma once

#include <string>
#include <vector>
#include <functional>

enum class EventFunctionType
{
	NotSpecified = 0,
	TCPPacket = 1,
};

class IEvent
{
public:
	virtual void Execute() = 0;
	virtual EventFunctionType GetType() = 0;
};

class IExtendedSocket;
class CReceivePacket;

class IEvents
{
public:
	virtual void AddEvent(IEvent* ev) = 0;
	virtual void AddEventFunction(const std::function<void()>& func) = 0;
};
