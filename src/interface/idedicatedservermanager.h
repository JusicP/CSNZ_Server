#pragma once

#include "imanager.h"

class CDedicatedServer;
class IRoom;
class IDedicatedServerManager : public IBaseManager
{
public:
	virtual bool OnPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;

	virtual void AddServer(IExtendedSocket* socket, int ip, int port) = 0;

	virtual CDedicatedServer* GetAvailableServerFromPools(IRoom* room) = 0;
	virtual bool IsPoolAvailable() = 0;
	virtual CDedicatedServer* GetServerBySocket(IExtendedSocket* socket) = 0;
	virtual void RemoveServer(IExtendedSocket* socket) = 0;
	virtual void TransferServer(IExtendedSocket* socket, const std::string& ipAddress, int port) = 0;
};