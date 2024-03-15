#include <doctest/doctest.h>

#include "net/tcpserver.h"
#include "net/tcpclient.h"
#include "net/sendpacket.h"
#include "net/receivepacket.h"
#include "net/extendedsocket.h"
#include "common/net/netdefs.h"

#include "interface/net/iserverlistener.h"

using namespace std;

/*
 * Server class to test packet sequence reset
 */
class CTCPServer_TestPacketSequence : public IServerListenerTCP
{
public:
	CTCPServer_TestPacketSequence(const string& port)
	{
		m_bFinished = false;
		m_bFailed = false;

		m_Server.SetListener(this);
		REQUIRE(m_Server.Start(port, 128) == true);
	}

	void OnTCPConnectionCreated(IExtendedSocket* socket)
	{
		// send 255 + 1 messages
		for (int i = 0; i < MAX_SEQUENCE + 1; i++)
		{
			CSendPacket* msg = new CSendPacket(socket->GetSeq(), 1);
			msg->BuildHeader();
			msg->WriteUInt8(i);

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
		// finish test after receiving
		if (msg->ReadUInt8() == MAX_SEQUENCE)
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
 * Client class to test packet sequence reset
 */
class CTCPClient_TestPacketSequence : public IClientListenerTCP
{
public:
	CTCPClient_TestPacketSequence(const string& ip, const string& port)
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
		CSendPacket* sendMsg = new CSendPacket(m_Client.GetSocket()->GetSeq(), msg->GetID());
		sendMsg->BuildHeader();
		sendMsg->WriteUInt8(msg->ReadUInt8());

		size_t bufSize = sendMsg->GetData().getBuffer().size();
		int sentBytes = m_Client.GetSocket()->Send(sendMsg);
		if (m_Client.GetSocket()->GetPacketsToSend().empty())
			REQUIRE(sentBytes == bufSize);

		delete msg;
	}

	void OnTCPError(int errorCode)
	{
		FAIL("Client error occurred");
	}

	CTCPClient m_Client;
	bool m_bConnected;
};
