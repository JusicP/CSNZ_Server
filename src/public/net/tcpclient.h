#pragma once

#include "socketshared.h"
#include "common/thread.h"

#include <string>

class CExtendedSocket;
class IClientListenerTCP;

/**
 * Class that connects to the server on a given IP and port
 */
class CTCPClient : public ISocketListenable
{
public:
	CTCPClient();
	~CTCPClient();

	bool Start(const std::string& ip, const std::string& port);
	void Stop();
	void Listen();

	bool IsRunning();

	void SetListener(IClientListenerTCP* listener);
	void SetCriticalSection(CCriticalSection* criticalSection);

	CExtendedSocket* GetSocket();

private:
	CExtendedSocket* m_pSocket;
	bool m_bIsRunning;
	bool m_nResult;
	CThread m_ListenThread;
	IClientListenerTCP* m_pListener;
	CCriticalSection* m_pCriticalSection;

	fd_set m_FdsRead;
	fd_set m_FdsWrite;
	fd_set m_FdsExcept;

	bool m_bConnected; // true when server connected msg received
};