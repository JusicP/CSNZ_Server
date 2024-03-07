#pragma once

#include "interface/iserverinstance.h"
#include "interface/net/iserverlistener.h"
#include "csvtable.h"

#include "net/tcpserver.h"
#include "net/udpserver.h"

class CServerInstance : public IServerInstance, IServerListenerTCP, IServerListenerUDP
{
public:
	CServerInstance();
	~CServerInstance();

	bool Init();
	bool LoadConfigs();
	void UnloadConfigs();

	virtual void OnTCPConnectionCreated(IExtendedSocket* socket);
	virtual void OnTCPConnectionClosed(IExtendedSocket* socket);
	virtual void OnTCPMessage(IExtendedSocket* socket, CReceivePacket* msg);
	virtual void OnTCPError(int errorCode);

	virtual void OnUDPMessage(Buffer& buf, unsigned short port);
	virtual void OnUDPError(int errorCode);

	void SetServerActive(bool active);
	bool IsServerActive();
	void OnCommand(const std::string& command);
	void OnEvent();
	void OnPackets(IExtendedSocket* s, CReceivePacket* msg);
	void OnSecondTick();
	void OnMinuteTick();
	void OnFunction(std::function<void()>& func);
	void UpdateConsoleStatus();
	virtual time_t GetCurrentTime();
	virtual tm* GetCurrentLocalTime();
	virtual double GetMemoryInfo();
	virtual const char* GetMainInfo();
	virtual void DisconnectClient(IExtendedSocket* socket);
	virtual std::vector<IExtendedSocket*> GetClients();
	virtual IExtendedSocket* GetSocketByID(unsigned int id);

private:
	bool m_bIsServerActive;

	time_t m_CurrentTime;
	tm* m_pCurrentLocalTime;
	time_t m_nUptime;

	CTCPServer m_TCPServer;
	CUDPServer m_UDPServer;
};

extern CServerInstance* g_pServerInstance;

extern CCSVTable* g_pItemTable;
extern CCSVTable* g_pMapListTable;
extern CCSVTable* g_pGameModeListTable;

void* ReadConsoleThread(void*);
void* EventThread(void*);
