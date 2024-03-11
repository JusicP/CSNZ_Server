#pragma once

#include "socketshared.h"
#include "common/thread.h"

#include <string>
#include <vector>

class IExtendedSocket;
class CExtendedSocket;
class IServerListenerTCP;

/**
 * Class that accepts TCP connection
 */
class CTCPServer : public ISocketListenable
{
public:
	CTCPServer();
	~CTCPServer();
	
	bool Start(const std::string& port, int tcpSendBufferSize);
	void Stop();
	void Listen();

	IExtendedSocket* Accept(unsigned int id);
	IExtendedSocket* GetExSocketBySocket(SOCKET socket);
	void DisconnectClient(IExtendedSocket* socket);
	std::vector<IExtendedSocket*>& GetClients();

	bool IsRunning();

	void SetListener(IServerListenerTCP* listener);
	void SetCriticalSection(CCriticalSection* criticalSection);

private:
	SOCKET m_Socket;
	bool m_bIsRunning;
	int m_nResult;
	CThread m_ListenThread;
	unsigned int m_nNextClientIndex;
	std::vector<IExtendedSocket*> m_Clients;
	IServerListenerTCP* m_pListener;
	CCriticalSection* m_pCriticalSection;

	fd_set m_FdsRead;
	fd_set m_FdsWrite;
	int m_nMaxFD;
};