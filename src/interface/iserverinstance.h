#pragma once

#include <string>
#include <vector>

class CReceivePacket;
class IExtendedSocket;

class IServerInstance
{
public:
	virtual bool Reload() = 0;
	virtual time_t GetCurrentTime() = 0;
	virtual tm* GetCurrentLocalTime() = 0;
	virtual double GetMemoryInfo() = 0;
	virtual const char* GetMainInfo() = 0;
	virtual void DisconnectClient(IExtendedSocket* socket) = 0;
	virtual std::vector<IExtendedSocket*> GetClients() = 0;
	virtual IExtendedSocket* GetSocketByID(unsigned int id) = 0;

	virtual void OnCommand(const std::string& command) = 0;
};