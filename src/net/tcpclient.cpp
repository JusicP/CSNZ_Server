#include "net/tcpclient.h"
#include "net/extendedsocket.h"
#include "net/socketshared.h"

#include "interface/net/iserverlistener.h"

#include "common/utils.h"
#include "common/console.h"

using namespace std;

/**
 * Constructor.
 */
CTCPClient::CTCPClient() : m_ListenThread(ListenThread, this)
{
	m_pSocket = NULL;
	m_bIsRunning = false;
	m_pListener = NULL;
	m_pCriticalSection = NULL;
	m_nResult = 0;

	FD_ZERO(&m_FdsRead);
	FD_ZERO(&m_FdsWrite);
	m_nMaxFD = 0;

#ifdef WIN32
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 0), &wsaData);
	if (result != 0)
	{
		Console().FatalError("WSAStartup() failed with error: %d\n%s\n", m_nResult, WSAGetLastErrorString());
	}
#endif
}

/**
 * Destructor. Stop the server on destructor
 */
CTCPClient::~CTCPClient()
{
	Stop();
}

/**
 * Create socket and start listening.
 * @param port Server's port
 * @return True on success, false on error
 */
bool CTCPClient::Start(const string& ip, const string& port)
{
	if (IsRunning())
		return false;

	addrinfo hints;
	addrinfo* result;
	if (getaddrinfo(ip.data(), port.data(), &hints, &result) != 0)
	{
		Console().FatalError("getaddrinfo() failed with error: %ld\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		return false;
	}

	SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET)
	{
		Console().FatalError("socket() failed with error: %ld\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		return false;
	}

	// Set the mode of the socket to be nonblocking
	u_long iMode = 1;
	if (ioctlsocket(sock, FIONBIO, &iMode) == SOCKET_ERROR)
	{
		Console().FatalError("ioctlsocket() failed with error: %d\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		closesocket(sock);
		return false;
	}

	if (connect(sock, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR)
	{
		Console().FatalError("connect() failed with error: %d\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		closesocket(sock);
		return false;
	}

	freeaddrinfo(result);

	m_pSocket = new CExtendedSocket(sock);
	m_pSocket->SetIP(ip);

	m_nMaxFD = sock;

	m_bIsRunning = true;

	m_ListenThread.Start();

	return true;
}

/**
 * Stop the client
 */
void CTCPClient::Stop()
{
	if (IsRunning())
	{
		closesocket(m_pSocket->GetSocket());

		m_bIsRunning = false;

		m_ListenThread.Join();
	}
}

/**
 * Listen and wait for incoming data
 */
void CTCPClient::Listen()
{
	FD_ZERO(&m_FdsRead);
	FD_ZERO(&m_FdsWrite);

	FD_SET(m_pSocket->GetSocket(), &m_FdsRead);

	if (m_pSocket->GetPacketsToSend().size())
		FD_SET(m_pSocket->GetSocket(), &m_FdsWrite);

	FD_SET(m_pSocket->GetSocket(), &m_FdsRead);

	if ((int)m_pSocket->GetSocket() > m_nMaxFD)
		m_nMaxFD = m_pSocket->GetSocket();

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	int activity = select(m_nMaxFD + 1, &m_FdsRead, &m_FdsWrite, NULL, &tv);
	if (activity == SOCKET_ERROR)
	{
		Console().Error("select() failed with error: %d\n", GetNetworkError());
		return;
	}
	else if (!activity) // timeout
	{
		return;
	}

	if (m_pCriticalSection)
		m_pCriticalSection->Enter();

	for (int i = 0; i <= m_nMaxFD; i++)
	{
		if (FD_ISSET(i, &m_FdsRead))
		{
			if (m_pSocket->GetSocket() == i)
			{
				printf("accept\n");
			}
			else
			{
				printf("receive\n");

				CReceivePacket* msg = m_pSocket->Read();
				int readResult = m_pSocket->GetReadResult();

				//if (m_pListener)
					//m_pListener->OnTCPMessage(socket, socket->GetMsg());
			}
		}

		if (FD_ISSET(i, &m_FdsWrite)) // data to write
		{
			printf("write\n");
			if (m_pSocket->GetPacketsToSend().size())
			{
				// send the first packet from the queue
				CSendPacket* msg = m_pSocket->GetPacketsToSend()[0];
				if (m_pSocket->Send(msg, true) <= 0)
				{
					Console().Warn("An error occurred while sending packet from queue: WSAGetLastError: %d, queue.size: %d\n", GetNetworkError(), m_pSocket->GetPacketsToSend().size());

					*(int*)0 = NULL;
				}
				else
				{
					m_pSocket->GetPacketsToSend().erase(m_pSocket->GetPacketsToSend().begin());
				}
			}
		}
	}

	if (m_pCriticalSection)
		m_pCriticalSection->Leave();
}

/**
 * Checks if client is running
 * @return Running status
 */
bool CTCPClient::IsRunning()
{
	return m_bIsRunning;
}

/**
 * Sets listen object
 * @param listener Listener object
 */
void CTCPClient::SetListener(IClientListenerTCP* listener)
{
	m_pListener = listener;
}

/**
 * Sets critical section object. Critical section is used for interacting with the listener object or extended socket
 * @param criticalSection Pointer to critical section object
 */
void CTCPClient::SetCriticalSection(CCriticalSection* criticalSection)
{
	m_pCriticalSection = criticalSection;
}