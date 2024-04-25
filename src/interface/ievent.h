#pragma once

#include <string>
#include <vector>
#include <functional>

class IEvent
{
public:
	virtual void Execute() = 0;
};

class IExtendedSocket;
class CReceivePacket;

class IEvents
{
public:
	virtual void AddEvent(IEvent* ev) = 0;
	virtual void AddEventFunction(const std::function<void()>& func) = 0;
};
