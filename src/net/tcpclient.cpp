#include "net/tcpclient.h"
#include "net/extendedsocket.h"
#include "net/socketshared.h"

#include "interface/net/iserverlistener.h"

#include "common/utils.h"
#include "common/logger.h"

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
	m_bConnected = false;

	FD_ZERO(&m_FdsRead);
	FD_ZERO(&m_FdsWrite);
	FD_ZERO(&m_FdsExcept);

#ifdef WIN32
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		Logger().Fatal("WSAStartup() failed with error: %d\n%s\n", m_nResult, WSAGetLastErrorString());
	}
#endif
}

/**
 * Destructor. Stop the server on destructor
 */
CTCPClient::~CTCPClient()
{
	Stop();

	if (m_pSocket)
		delete m_pSocket;
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

	addrinfo hints = {};
	addrinfo* result;
	if (getaddrinfo(ip.data(), port.data(), &hints, &result) != 0)
	{
		Logger().Fatal("getaddrinfo() failed with error: %ld\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		return false;
	}

	SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock == INVALID_SOCKET)
	{
		Logger().Fatal("socket() failed with error: %ld\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		return false;
	}

	// Set the mode of the socket to be nonblocking
	u_long iMode = 1;
	if (ioctlsocket(sock, FIONBIO, &iMode) == SOCKET_ERROR)
	{
		Logger().Fatal("ioctlsocket() failed with error: %d\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		closesocket(sock);
		return false;
	}

	if (connect(sock, result->ai_addr, result->ai_addrlen) == SOCKET_ERROR && GetNetworkError() != WSAEWOULDBLOCK)
	{
		Logger().Fatal("connect() failed with error: %d\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		closesocket(sock);
		return false;
	}

	m_pSocket = new CExtendedSocket(sock);
	m_pSocket->SetIP(ip);

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
		m_bIsRunning = false;

		m_ListenThread.Join();

		closesocket(m_pSocket->GetSocket());
	}
}

/**
 * Listen and wait for incoming data
 */
void CTCPClient::Listen()
{
	FD_ZERO(&m_FdsRead);
	FD_ZERO(&m_FdsWrite);
	FD_ZERO(&m_FdsExcept);

	FD_SET(m_pSocket->GetSocket(), &m_FdsRead);

	if (m_pSocket->GetPacketsToSend().size())
		FD_SET(m_pSocket->GetSocket(), &m_FdsWrite);

	FD_SET(m_pSocket->GetSocket(), &m_FdsExcept);

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	int activity = select(1, &m_FdsRead, &m_FdsWrite, &m_FdsExcept, &tv);
	if (activity == SOCKET_ERROR)
	{
		Logger().Fatal("select() failed with error: %d\n", GetNetworkError());
		return;
	}
	else if (!activity) // timeout
	{
		return;
	}

	if (m_pCriticalSection)
		m_pCriticalSection->Enter();

	if (FD_ISSET(m_pSocket->GetSocket(), &m_FdsRead))
	{
		if (!m_bConnected)
		{
			// read server connected message
			if (!m_pSocket->OnServerConnected())
			{
				// seems to be connected to the wrong server
				Stop();

				if (m_pListener)
					m_pListener->OnTCPError(0);
			}

			if (m_pListener)
				m_pListener->OnTCPServerConnected();

			m_bConnected = true;
		}
		else
		{
			CReceivePacket* msg = m_pSocket->Read();
			int readResult = m_pSocket->GetReadResult();
			if (readResult == 0)
			{
				// connection closed
				Stop();
			}
			else if (readResult == SOCKET_ERROR)
			{
				// error, close connection
				Stop();

				if (m_pListener)
					m_pListener->OnTCPError(0);
			}
			else if (!msg)
			{
				// packet is not valid or wrong sequence or decryption failed
				/// @fixme should we disconnect here?

				// exclude case when message is not fully read
				if (!m_pSocket->GetMsg())
					Stop();
			}
			else
			{
				// Call this to mark that socket is ready to receive a new message
				m_pSocket->SetMsg(NULL);

				// Important: responsibility for deleting the message is assumed by the listener
				/// @todo use shared_ptr for messages?
				if (m_pListener)
					m_pListener->OnTCPMessage(msg);
				else
					delete msg;
			}
		}
	}

	if (FD_ISSET(m_pSocket->GetSocket(), &m_FdsWrite)) // data to write
	{
 		if (m_pSocket->GetPacketsToSend().size())
		{
			// send the first packet from the queue
			CSendPacket* msg = m_pSocket->GetPacketsToSend()[0];
			if (m_pSocket->Send(msg, true) <= 0)
			{
				Logger().Fatal("An error occurred while sending packet from queue: WSAGetLastError: %d, queue.size: %d\n", GetNetworkError(), m_pSocket->GetPacketsToSend().size());

				Stop();
			}
			else
			{
				m_pSocket->GetPacketsToSend().erase(m_pSocket->GetPacketsToSend().begin());
			}
		}
	}

	if (FD_ISSET(m_pSocket->GetSocket(), &m_FdsExcept))
	{
		if (m_pListener)
			m_pListener->OnTCPServerConnectFailed();
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

/**
 * Gets extended socket object
 * @return m_pSocket Pointer to extended socket object
 */
CExtendedSocket* CTCPClient::GetSocket()
{
	return m_pSocket;
}