#pragma once

#include <mutex>
#include "manager.h"
#include "interface/idedicatedservermanager.h"

class IRoom;
class IExtendedSocket;
class CReceivePacket;

class CDedicatedServer
{
public:
	CDedicatedServer(IExtendedSocket* s, int ip, int port);

	void SetRoom(IRoom* room);
	void SetMemoryUsage(int memShift);

	IExtendedSocket* GetSocket();
	IRoom* GetRoom();
	int GetMemoryUsage();
	int GetIP();
	int GetPort();

private:
	IExtendedSocket* m_pSocket;
	IRoom* m_pRoom;

	int m_iLastMemory;
	int m_iIP;
	int m_iPort;
};

class CDedicatedServerManager : public CBaseManager<IDedicatedServerManager>
{
public:
	CDedicatedServerManager();
	~CDedicatedServerManager();

	virtual void Shutdown();

	bool OnPacket(CReceivePacket* msg, IExtendedSocket* socket);

	void AddServer(CDedicatedServer* server);

	CDedicatedServer* GetAvailableServerFromPools(IRoom* room);
	bool IsPoolAvailable();
	CDedicatedServer* GetServerBySocket(IExtendedSocket* socket);
	void RemoveServer(IExtendedSocket* socket);
	void TransferServer(IExtendedSocket* socket, const std::string& ipAddress, int port);

private:
	std::mutex hMutex;
	std::vector<CDedicatedServer*> vServerPools;
};

extern CDedicatedServerManager g_DedicatedServerManager;