#include "net/udpserver.h"
#include "interface/net/iserverlistener.h"

#include "common/utils.h"
#include "common/console.h"

#include <vector>

using namespace std;

/**
 * Constructor.
 */
CUDPServer::CUDPServer() : m_ListenThread(ListenThread, this)
{
	m_Socket = INVALID_SOCKET;
	m_bIsRunning = false;
	m_pListener = NULL;
	m_pCriticalSection = NULL;

	FD_ZERO(&m_FdsMaster);
	FD_ZERO(&m_FdsRead);
	m_nMaxFD = 0;

	m_nLastAddrLen = sizeof(m_LastAddr);

#ifdef WIN32
	WSADATA wsaData;
	m_nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (m_nResult != 0)
	{
		Console().FatalError("WSAStartup() failed with error: %d\n%s\n", m_nResult, WSAGetLastErrorString());
	}
#else
	m_nResult = 0;
#endif
}

/**
 * Destructor. Stop the server on destructor
 */
CUDPServer::~CUDPServer()
{
	Stop();
}

/**
 * Create socket and start listening.
 * @param port Server's port
 * @return True on success, false on error
 */
bool CUDPServer::Start(const string& port)
{
	if (m_bIsRunning)
		return false;

	int fromlen = sizeof(sockaddr_in);
	struct addrinfo* result = NULL;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	m_nResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
	if (m_nResult != 0)
	{
		Console().FatalError("getaddrinfo() failed with error: %d\n%s\n", m_nResult, WSAGetLastErrorString());
		return false;
	}

	m_Socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (m_Socket == INVALID_SOCKET)
	{
		Console().FatalError("socket() failed with error: %ld\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		return false;
	}

	u_long iMode = 1;
	m_nResult = ioctlsocket(m_Socket, FIONBIO, &iMode);
	if (m_nResult == SOCKET_ERROR)
	{
		Console().FatalError("ioctlsocket() failed with error: %d\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		closesocket(m_Socket);
		return false;
	}

	m_nResult = ::bind(m_Socket, result->ai_addr, (int)result->ai_addrlen);
	if (m_nResult == SOCKET_ERROR)
	{
		Console().FatalError("bind failed with error: %d\n%s\n", GetNetworkError(), WSAGetLastErrorString());
		freeaddrinfo(result);
		closesocket(m_Socket);
		return false;
	}

	freeaddrinfo(result);

	FD_ZERO(&m_FdsMaster);
	FD_ZERO(&m_FdsRead);

	FD_SET(m_Socket, &m_FdsMaster);

	m_nMaxFD = m_Socket;

	m_bIsRunning = true;

	m_ListenThread.Start();

	return true;
}

/**
 * Stop the server
 */
void CUDPServer::Stop()
{
	if (m_bIsRunning)
	{
		m_bIsRunning = false;

		m_ListenThread.Join();

		closesocket(m_Socket);
	}
}

/**
 * Listen and wait for incoming data
 */
void CUDPServer::Listen()
{
	m_FdsRead = m_FdsMaster; // reset

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	int activity = select(m_nMaxFD + 1, &m_FdsRead, NULL, NULL, &tv);
	if (activity == SOCKET_ERROR)
	{
		Console().Error("select(udp) failed with error: %d.\n", GetNetworkError());
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
			// got message
			int datalen = recvfrom(m_Socket, m_Buffer, 5000, 0, (sockaddr*)&m_LastAddr, &m_nLastAddrLen);

			Buffer buf(vector<unsigned char>(m_Buffer, m_Buffer + datalen));

			if (m_pListener)
				m_pListener->OnUDPMessage(buf, m_LastAddr.sin_port);
		}
	}

	if (m_pCriticalSection)
		m_pCriticalSection->Leave();
}

/**
 * Sends message to client
 * @param buf Send buffer
 */
void CUDPServer::SendTo(const Buffer& buf)
{
	const vector<unsigned char>& buffer = buf.getBuffer();
	sendto(m_Socket, reinterpret_cast<const char*>(&buffer[0]), buffer.size(), 0, (sockaddr*)&m_LastAddr, m_nLastAddrLen);
}

/**
 * Checks if server is running
 * @return Running status
 */
bool CUDPServer::IsRunning()
{
	return m_bIsRunning;
}

/**
 * Sets listen object
 * @param listener Listener object
 */
void CUDPServer::SetListener(IServerListenerUDP* listener)
{
	m_pListener = listener;
}

/**
 * Sets critical section object. Critical section is used for interacting with the listener object 
 * @param criticalSection Pointer to critical section object
 */
void CUDPServer::SetCriticalSection(CCriticalSection* criticalSection)
{
	m_pCriticalSection = criticalSection;
}