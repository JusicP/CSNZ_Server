#include <doctest/doctest.h>

#include "net/tcpserver.h"
#include "net/tcpclient.h"
#include "net/sendpacket.h"
#include "net/receivepacket.h"
#include "net/extendedsocket.h"

#include "interface/net/iserverlistener.h"

using namespace std;

/*
 * Server class to test network send/receive functionality
 */
class CTCPServer_TestBasicFuncs : public IServerListenerTCP
{
public:
	CTCPServer_TestBasicFuncs(const string& port)
	{
		m_bFinished = false;
		m_bFailed = false;

		m_Server.SetListener(this);
		REQUIRE(m_Server.Start(port, 128) == true);
	}

	void OnTCPConnectionCreated(IExtendedSocket* socket)
	{
		// Send(Server -> Client)
		{
			// send message with id 1
			CSendPacket* msg = new CSendPacket(socket->GetSeq(), 1);
			msg->BuildHeader();
			msg->WriteString("Hello from server");

			size_t bufSize = msg->GetData().getBuffer().size();
			int sentBytes = socket->Send(msg);
			if (socket->GetPacketsToSend().empty())
				REQUIRE(sentBytes == bufSize);
		}
	}

	void OnTCPConnectionClosed(IExtendedSocket* socket)
	{
		REQUIRE(m_bFinished == true);
		if (!m_bFinished)
		{
			m_bFailed = true;
		}
	}

	void OnTCPMessage(IExtendedSocket* socket, CReceivePacket* msg)
	{
		// Receive(Client -> Server)
		{
			CHECK(msg->GetSequence() == 1);
			CHECK(msg->GetID() == 2);
			CHECK(msg->ReadString() == "Hello from client");
		}

		m_bFinished = true;
		delete msg;
	}

	void OnTCPError(int errorCode)
	{
		FAIL("Server error occurred");
		m_bFailed = true;
	}

	CTCPServer m_Server;
	bool m_bFailed;
	bool m_bFinished;
};

/*
 * Client class to test network send/receive functionality
 */
class CTCPClient_TestBasicFuncs : public IClientListenerTCP
{
public:
	CTCPClient_TestBasicFuncs(const string& ip, const string& port)
	{
		m_bConnected = false;

		m_Client.SetListener(this);
		REQUIRE(m_Client.Start(ip, port) == true);
	}

	void OnTCPServerConnected()
	{
		// server hello received
		m_bConnected = true;
	}

	void OnTCPServerConnectFailed()
	{
		FAIL("Server connect failed");
	}

	void OnTCPMessage(CReceivePacket* msg)
	{
		REQUIRE(m_bConnected == true);

		// Receive(Server -> Client)
		{
			CHECK(msg->GetSequence() == 1);
			CHECK(msg->GetID() == 1);
			CHECK(msg->ReadString() == "Hello from server");
		}

		// Send(Client -> Server)
		{
			// send message with id 2
			CSendPacket* sendMsg = new CSendPacket(m_Client.GetSocket()->GetSeq(), 2);
			sendMsg->BuildHeader();
			sendMsg->WriteString("Hello from client");

			size_t bufSize = sendMsg->GetData().getBuffer().size();
			int sentBytes = m_Client.GetSocket()->Send(sendMsg);
			if (m_Client.GetSocket()->GetPacketsToSend().empty())
				REQUIRE(sentBytes == bufSize);
		}

		delete msg;
	}

	void OnTCPError(int errorCode)
	{
		FAIL("Client error occurred");
	}

	CTCPClient m_Client;
	bool m_bConnected;
};
