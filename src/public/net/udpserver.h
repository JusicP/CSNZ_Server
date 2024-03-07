#pragma once

#include "socketshared.h"
#include "common/thread.h"
#include "common/buffer.h"

#include <string>

class IServerListenerUDP;

/**
 * Class that communicates with UDP clients
 */
class CUDPServer : public ISocketListenable
{
public:
	CUDPServer();
	~CUDPServer();

	bool Start(const std::string& port);
	void Stop();
	void Listen();
	void SendTo(const Buffer& buf);

	bool IsRunning();

	void SetListener(IServerListenerUDP* listener);
	void SetCriticalSection(CCriticalSection* criticalSection);

private:
	SOCKET m_Socket;
	bool m_bIsRunning;
	int m_nResult;
	CThread m_ListenThread;
	IServerListenerUDP* m_pListener;
	CCriticalSection* m_pCriticalSection;
	char m_Buffer[5000];

	fd_set m_FdsMaster;
	fd_set m_FdsRead;
	int m_nMaxFD;

	// last address got from recvfrom
	sockaddr_in m_LastAddr;
	int m_nLastAddrLen;
};