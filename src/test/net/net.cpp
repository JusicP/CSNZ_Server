#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "net/tcpserver.h"
#include "net/tcpclient.h"
#include "net/sendpacket.h"
#include "net/receivepacket.h"
#include "net/extendedsocket.h"

#include "interface/net/iserverlistener.h"

#define TEST_PORT "30002"

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
			REQUIRE(socket->Send(msg) == bufSize);
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
			REQUIRE(m_Client.GetSocket()->Send(sendMsg) == bufSize);
		}
	}

	void OnTCPError(int errorCode)
	{
		FAIL("Client error occurred");
	}

	CTCPClient m_Client;
	bool m_bConnected;
};

TEST_CASE("Network (TCP) - Test send/receive")
{
	CTCPServer_TestBasicFuncs server(TEST_PORT);
	CTCPClient_TestBasicFuncs client("127.0.0.1", TEST_PORT);

	while (!server.m_bFailed && !server.m_bFinished)
	{
		// wait for the test finish/error
	}
}

//TEST_CASE("Network (TCP) - Test packet sequence")
//{
//	CTCPServer_TestPacketSequence server(TEST_PORT);
//	CTCPClient_TestPacketSequence client("127.0.0.1", TEST_PORT);
//
//	while (!server.m_bFailed && !server.m_bFinished)
//	{
//		// wait for the test finish/error
//	}
//}