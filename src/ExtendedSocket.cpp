#include "ExtendedSocket.h"
#include "SendPacket.h"

using namespace std;
#define PACKET_MAX_SIZE 0x10000
#define PACKET_HEADER_SIZE 4

CExtendedSocket::CExtendedSocket()
{
	memset(&data, 0, sizeof(GuestData_s));
	m_Socket = 0;
	m_dwTimer = 0;
	m_nSequence = 0;
	m_nBytesReceived = 0;
	m_nBytesSent = 0;
	m_nPacketReceivedSize = 0;
	m_nPacketToReceiveFullSize = 0;
	m_nReadResult = 0;
	m_nNextExpectedSeq = 1;
	m_pMsg = NULL;
}

int CExtendedSocket::GetSeq()
{
	if (m_nSequence > MAX_SEQUENCE)
	{
		ResetSeq();
	}

	return m_nSequence++;
}

int CExtendedSocket::LoggerGetSeq()
{
	return m_nSequence;
}

void CExtendedSocket::ResetSeq()
{
	m_nSequence = 0;
}

CReceivePacket* CExtendedSocket::Read()
{
	m_nReadResult = 0;
	vector<unsigned char> packetDataBuf;
	packetDataBuf.resize(PACKET_HEADER_SIZE);
	int recvResult = 0;

	if (!m_pMsg)
	{
		// first of all read the packet header to know is received packet is valid
		recvResult = g_pNetwork->ReceiveMessage(m_Socket, (char*)packetDataBuf.data(), PACKET_HEADER_SIZE);
		if (recvResult < 4)
		{
			g_pConsole->Warn("CExtendedSocket::Read(%s): result < PACKET_HEADER_SIZE, %d\n", GetIP().c_str(), WSAGetLastError());
			return NULL;
		}

		// update bytes counters
		m_nReadResult += recvResult; 
		m_nBytesReceived += recvResult;
		if (m_nBytesReceived < 0)
			m_nBytesReceived = 0;

		// when a people may incorrect once packet data, might spammed this message forever....
		m_pMsg = new CReceivePacket(Buffer(packetDataBuf));
		if (!m_pMsg->IsValid())
		{
			g_pConsole->Error("CExtendedSocket::Read(%s): received invalid packet\n", GetIP().c_str());
			delete m_pMsg;
			m_pMsg = NULL;
			return NULL;
		}

		if (m_pMsg->GetSequence() != m_nNextExpectedSeq)
		{
			g_pConsole->Warn("CExtendedSocket::Read(%s): sequence mismatch, got: %d, expected: %d\n", GetIP().c_str(), m_pMsg->GetSequence(), m_nNextExpectedSeq);
			//return NULL;
		}

		// well i think set to -1 afterwards, so fixed shits
		if (m_nNextExpectedSeq == 255)
			m_nNextExpectedSeq = -1;

		// here causes seq never 0 lol
		m_nNextExpectedSeq++;

		m_nPacketToReceiveFullSize = m_pMsg->GetLength();
		m_nPacketToReceiveFullSize += PACKET_HEADER_SIZE;

		m_nPacketReceivedSize = 0;
	}

	packetDataBuf.resize(m_pMsg->GetLength());
	recvResult = g_pNetwork->ReceiveMessage(m_Socket, (char*)packetDataBuf.data(), m_nPacketReceivedSize ? m_pMsg->GetLength() + PACKET_HEADER_SIZE - m_nPacketReceivedSize : m_pMsg->GetLength());
	if (recvResult <= 0)
	{
		g_pConsole->Warn("CExtendedSocket::Read(%s): result <= 0\n", GetIP().c_str(), WSAGetLastError());
		delete m_pMsg;
		m_pMsg = NULL;
		return NULL;
	}

	packetDataBuf.resize(recvResult);

	m_nReadResult += recvResult;
	m_nBytesReceived += recvResult;
	if (m_nBytesReceived < 0)
		m_nBytesReceived = 0;

	Buffer& buf = m_pMsg->GetData();
	// todo: rewrite
	vector<unsigned char> vecBuf = buf.getBuffer();
	vecBuf.insert(vecBuf.end(), packetDataBuf.begin(), packetDataBuf.end());
	buf.setBuffer(vecBuf);

#if 0
	g_pConsole->Server->Log("CExtendedSocket::Read: recvResult: %d, packetDataBuf.size: %d, m_nPacketReceivedSize: %d, m_pMsg->GetLength: %d, m_pMsg->GetSequence: %d, m_nPacketToReceiveFullSize: %d\n", recvResult, packetDataBuf.size(), m_nPacketReceivedSize, m_pMsg->GetLength(), m_pMsg->GetSequence(), m_nPacketToReceiveFullSize);
#endif

	// if not received the full packet
	if (m_nPacketToReceiveFullSize != buf.getBuffer().size())
	{
		m_nPacketReceivedSize = buf.getBuffer().size();
		return NULL;
	}

	buf.setReadOffset(0);
	m_pMsg->ParseHeader();

	return m_pMsg;
}

int CExtendedSocket::Send(const vector<unsigned char>& buffer)
{
	if (buffer.size() > PACKET_MAX_SIZE)
	{
		g_pConsole->Error("CExtendedSocket::Send() buffer.size(): %d > PACKET_MAX_SIZE!!!, ID: %d, seq: %d. Packet not sent.\n", buffer.size(), buffer[4], buffer[1]);
		m_nSequence--;
		return 0;
	}

	const char* bufData = reinterpret_cast<const char*>(&buffer[0]);

	int bytesSent = g_pNetwork->SendMessage(m_Socket, bufData, buffer.size());
	if (bytesSent != buffer.size())
		return bytesSent;

	m_nBytesSent += bytesSent;
	if (m_nBytesSent < 0)
		m_nBytesSent = 0;

#ifdef _DEBUG
	g_pConsole->Log("CExtendedSocket::Send() seq: %d, buffer.size(): %d, bytesSent: %d, id: %d\n", buffer[1], buffer.size(), bytesSent, buffer[4]);
#endif

	return bytesSent;
}

// complicated shit
int CExtendedSocket::Send(CSendPacket* msg, bool forceSend)
{
	int result = 1;

	if (!forceSend && m_SendPackets.size())
	{
		m_SendPackets.push_back(msg);
	}
	else
	{
		result = Send(msg->SetPacketLength());
		int error = WSAGetLastError();
		if (result != msg->GetData().getBuffer().size() && error)
		{
			if (error == WSAEWOULDBLOCK)
			{
				m_SendPackets.push_back(msg);
			}
			else
			{
				g_pConsole->Error("CExtendedSocket::Send() WSAGetLastError: %d\n", error);
			}
		}
		else
		{
			delete msg;
		}
	}

	return result;
}