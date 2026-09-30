#include <wolfssl/options.h>
#include <wolfssl/openssl/evp.h>

#include "net/extendedsocket.h"
#include "net/sendpacket.h"
#include "net/receivepacket.h"

#include "common/net/netdefs.h"
#include "common/logger.h"
#include "common/utils.h"

using namespace std;

/**
 * Constructor.
 * @param id
 */
CExtendedSocket::CExtendedSocket(SOCKET socket, unsigned int id)
{
	memset(&m_GuestData, 0, sizeof(GuestData_s));
	m_nID = id;
	m_Socket = socket;
	m_nSequence = 0;
	m_nBytesReceived = 0;
	m_nBytesSent = 0;
	m_nPacketReceivedSize = 0;
	m_nPacketSentSize = 0;
	m_nReadResult = 0;
	m_nNextExpectedSeq = 1;
	m_pMsg = NULL;
	m_pEncEVPCTX = NULL;
	m_pDecEVPCTX = NULL;
	m_bCryptInput = false;
	m_bCryptOutput = false;
	memset(m_pCryptKey, 0, 64);
	memset(m_pCryptIV, 0, 64);
	m_pSSL = NULL;
}

/**
 * Destructor.
 * Deletes unsent and unreceived packets
 */
CExtendedSocket::~CExtendedSocket()
{
	if (m_pMsg)
		delete m_pMsg;

	for (auto msg : m_SendPackets)
		delete msg;

	m_SendPackets.clear();

	if (m_pDecEVPCTX)
	{
		EVP_CIPHER_CTX_cleanup(m_pDecEVPCTX);
		EVP_CIPHER_CTX_free(m_pDecEVPCTX);
	}

	if (m_pEncEVPCTX)
	{
		EVP_CIPHER_CTX_cleanup(m_pEncEVPCTX);
		EVP_CIPHER_CTX_free(m_pEncEVPCTX);
	}

	if (m_pSSL)
	{
		// Notify the client that the connection is ending
		wolfSSL_shutdown(m_pSSL);

		// Cleanup after this connection
		wolfSSL_free(m_pSSL);
	}

	closesocket(m_Socket);
}

/**
 * Sets up packet cipher and create key
 * @return True on success
 */
bool CExtendedSocket::SetupCrypt()
{
	int keyLen = EVP_BytesToKey(EVP_aes_128_cbc(), EVP_md5(), NULL, m_HWID.data(), m_HWID.size(), 1, m_pCryptKey, m_pCryptIV);
	if (!keyLen)
	{
		return false;
	}

	m_pEncEVPCTX = EVP_CIPHER_CTX_new();
	m_pDecEVPCTX = EVP_CIPHER_CTX_new();

	EVP_CipherInit(m_pEncEVPCTX, EVP_rc4(), NULL, NULL, 1);
	EVP_CipherInit(m_pDecEVPCTX, EVP_rc4(), NULL, NULL, 0);

	// useless?
	//EVP_CIPHER_CTX_set_padding(m_pEncEVPCTX, 0);
	//EVP_CIPHER_CTX_set_padding(m_pDecEVPCTX, 0);

	if (EVP_CipherInit(m_pEncEVPCTX, EVP_rc4(), (const unsigned char*)m_pCryptKey, (const unsigned char*)m_pCryptIV, 1) != 1)
	{
		EVP_CIPHER_CTX_cleanup(m_pDecEVPCTX);
		EVP_CIPHER_CTX_cleanup(m_pEncEVPCTX);

		EVP_CIPHER_CTX_free(m_pDecEVPCTX);
		EVP_CIPHER_CTX_free(m_pEncEVPCTX);

		m_pDecEVPCTX = NULL;
		m_pEncEVPCTX = NULL;

		return false;
	}

	if (EVP_CipherInit(m_pDecEVPCTX, EVP_rc4(), (const unsigned char*)m_pCryptKey, (const unsigned char*)m_pCryptIV, 0) != 1)
	{
		EVP_CIPHER_CTX_cleanup(m_pDecEVPCTX);
		EVP_CIPHER_CTX_cleanup(m_pEncEVPCTX);

		EVP_CIPHER_CTX_free(m_pDecEVPCTX);
		EVP_CIPHER_CTX_free(m_pEncEVPCTX);

		m_pDecEVPCTX = NULL;
		m_pEncEVPCTX = NULL;

		return false;
	}

	return true;
}

/**
 * Increments current sequence and return it
 * @return Sequence number used for sending 
 */
int CExtendedSocket::GetSeq()
{
	if (m_nSequence == MAX_SEQUENCE)
	{
		ResetSeq();
	}

	return ++m_nSequence;
}

/**
 * Gets current sequence without incrementing
 * @return Current packet sequence
 */
int CExtendedSocket::LoggerGetSeq()
{
	return m_nSequence;
}

/**
 * Resets sequence counter
 */
void CExtendedSocket::ResetSeq()
{
	m_nSequence = -1;
}

/**
 * Reads data on a socket
 * @param buf Pointer to received data
 * @param len Data length to be received
 * @return recv result
 */
int CExtendedSocket::Read(char* buf, int len)
{
	int recvResult = 0;

	if (m_pSSL && !m_bCryptInput)
		recvResult = wolfSSL_recv(m_pSSL, buf, len, 0);
	else
		recvResult = recv(m_Socket, buf, len, 0);

	m_nReadResult += recvResult;
	m_nBytesReceived += recvResult;
	if (m_nBytesReceived < 0)
		m_nBytesReceived = 0;

	return recvResult;
}

/**
 * Receives incoming packet or packets
 * @return Pointer to received message if entire message has been read, NULL on error or if waiting for the rest of message
 */
CReceivePacket* CExtendedSocket::Read()
{
	m_nReadResult = 0;
	vector<unsigned char> packetDataBuf;
	int recvResult = 0;

	if (!m_pMsg)
	{
		packetDataBuf.resize(PACKET_HEADER_SIZE);

		// first of all read the packet header to know is received packet is valid
		recvResult = Read((char*)packetDataBuf.data(), packetDataBuf.size());
		if (recvResult < PACKET_HEADER_SIZE)
		{
			if (recvResult > 0)
				Logger().Error("CExtendedSocket::Read(%s): result < PACKET_HEADER_SIZE, %d\n", GetIP().c_str(), GetNetworkError());

			return NULL;
		}

		// decrypt header if encrypted
		if (m_bCryptInput)
		{
			int outLen = 0;
			if (EVP_DecryptUpdate(m_pDecEVPCTX, packetDataBuf.data(), &outLen, packetDataBuf.data(), recvResult) != 1)
			{
				Logger().Error("CExtendedSocket::Read(%s): EVP_DecryptUpdate failed\n", GetIP().c_str());
				return NULL;
			}

			int finalLen = 0;
			if (EVP_DecryptFinal_ex(m_pDecEVPCTX, packetDataBuf.data() + outLen, &finalLen) != 1)
			{
				Logger().Error("CExtendedSocket::Read(%s): EVP_DecryptUpdate failed\n", GetIP().c_str());
				return NULL;
			}
		}

		// when a people may incorrect once packet data, might spammed this message forever....
		m_pMsg = new CReceivePacket(Buffer(packetDataBuf));
		if (!m_pMsg->IsValid())
		{
			Logger().Error("CExtendedSocket::Read(%s): received invalid packet\n", GetIP().c_str());
			delete m_pMsg;
			m_pMsg = NULL;
			return NULL;
		}

		if (m_pMsg->GetSequence() != m_nNextExpectedSeq)
		{
			Logger().Error("CExtendedSocket::Read(%s): sequence mismatch, got: %d, expected: %d\n", GetIP().c_str(), m_pMsg->GetSequence(), m_nNextExpectedSeq);
			delete m_pMsg;
			m_pMsg = NULL;
			return NULL;
		}

		// Reset m_nNextExpectedSeq
		if (m_nNextExpectedSeq == MAX_SEQUENCE)
			m_nNextExpectedSeq = -1;

		m_nNextExpectedSeq++;

		m_nPacketReceivedSize = 0;
	}

	// if there is data to read
	if (m_pMsg->GetLength() > 0)
	{
		packetDataBuf.resize(m_pMsg->GetLength() - m_nPacketReceivedSize);

		recvResult = Read((char*)packetDataBuf.data(), packetDataBuf.size());
		if (recvResult <= 0) // error or peer disconnected
		{
			if (recvResult < 0)
				Logger().Error("CExtendedSocket::Read(%s): result < 0, %d\n", GetIP().c_str(), GetNetworkError());

			delete m_pMsg;
			m_pMsg = NULL;
			return NULL;
		}

		if (recvResult < packetDataBuf.size())
			packetDataBuf.resize(recvResult);

		m_nPacketReceivedSize += recvResult;

		// decrypt the rest part of packet
		if (m_bCryptInput)
		{
			int outLen = 0;
			if (EVP_DecryptUpdate(m_pDecEVPCTX, packetDataBuf.data(), &outLen, packetDataBuf.data(), recvResult) != 1)
			{
				Logger().Error("CExtendedSocket::Read(%s): EVP_DecryptUpdate failed\n", GetIP().c_str());
				delete m_pMsg;
				m_pMsg = NULL;
				return NULL;
			}

			int finalLen = 0;
			if (EVP_DecryptFinal_ex(m_pDecEVPCTX, packetDataBuf.data() + outLen, &finalLen) != 1)
			{
				Logger().Error("CExtendedSocket::Read(%s): EVP_DecryptUpdate failed\n", GetIP().c_str());
				delete m_pMsg;
				m_pMsg = NULL;
				return NULL;
			}
		}

		// append read data to packet buffer
		Buffer& buf = m_pMsg->GetData();
		/// @todo: rewrite
		vector<unsigned char> vecBuf = buf.getBuffer();
		vecBuf.insert(vecBuf.end(), packetDataBuf.begin(), packetDataBuf.end());
		buf.setBuffer(vecBuf);

		// if not full message read
		if (m_nPacketReceivedSize < m_pMsg->GetLength())
		{
			// wait for rest of message
			return NULL;
		}
	}

#if 0
	Logger().Info("CExtendedSocket::Read(%s): recvResult: %d, packetDataBuf.size: %d, m_nPacketReceivedSize: %d, m_pMsg->GetLength: %d, m_pMsg->GetSequence: %d, m_nPacketToReceiveFullSize: %d\n", GetIP().c_str(), recvResult, packetDataBuf.size(), m_nPacketReceivedSize, m_pMsg->GetLength(), m_pMsg->GetSequence(), m_nPacketToReceiveFullSize);
#endif

	m_pMsg->GetData().setReadOffset(0);
	m_pMsg->ParseHeader();

	if (m_pMsg->GetID() == 12 && !m_bCryptInput)
		SetCryptInput(true);

	return m_pMsg;
}

/**
 * Sends packet (internal)
 * @param buffer Data to be sent
 * @return Number of bytes sent, SOCKET_ERROR on error
 */
int CExtendedSocket::Send(vector<unsigned char>& buffer, bool serverHelloMsg)
{
	if (!serverHelloMsg && buffer.size() > PACKET_MAX_SIZE)
	{
		Logger().Error("CExtendedSocket::Send(%s) buffer.size(): %d > PACKET_MAX_SIZE!!!, ID: %d, seq: %d. Packet not sent.\n", GetIP().c_str(), buffer.size(), buffer[4], buffer[1]);
		return 0;
	}

#ifdef _DEBUG
	auto rawBuffer = buffer;
#endif

	if (m_bCryptOutput)
	{
		int encLen = 0;
		if (EVP_EncryptUpdate(m_pEncEVPCTX, buffer.data(), &encLen, buffer.data(), buffer.size()) != 1)
		{
			Logger().Info("CExtendedSocket::Send(%s): EVP_EncryptUpdate failed\n", GetIP().c_str());
			return 0;
		}

		int finalLen = 0;
		if (EVP_EncryptFinal_ex(m_pEncEVPCTX, buffer.data() + encLen, &finalLen) != 1)
		{
			Logger().Info("CExtendedSocket::Send(%s): EVP_EncryptUpdate failed\n", GetIP().c_str());
			return 0;
		}

		if (encLen != buffer.size())
		{
			Logger().Info("CExtendedSocket::Send(%s): encLen != buffer.size()\n", GetIP().c_str());
			return 0;
		}
	}

	m_nPacketSentSize = 0;

	int bytesSent = 0;

	do
	{
		if (m_pSSL && !m_bCryptOutput)
		{
			int err;
			do
			{
				bytesSent = wolfSSL_send(m_pSSL, (const char*)&buffer[m_nPacketSentSize], buffer.size() - m_nPacketSentSize, 0);
				err = wolfSSL_get_error(m_pSSL, bytesSent);
			} while (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE);
		}
		else
			bytesSent = send(m_Socket, (const char*)&buffer[m_nPacketSentSize], buffer.size() - m_nPacketSentSize, 0);
		if (bytesSent <= 0)
			return bytesSent;

		m_nPacketSentSize += bytesSent;

		m_nBytesSent += bytesSent;
		if (m_nBytesSent < 0)
			m_nBytesSent = 0;
	} while (m_nPacketSentSize != buffer.size());

#ifdef _DEBUG
	if (!serverHelloMsg)
		Logger().Debug("CExtendedSocket::Send(%s) seq: %d, buffer.size(): %d, id: %d\n", GetIP().c_str(), rawBuffer[1], rawBuffer.size(), rawBuffer[4]);
#endif

	return m_nPacketSentSize;
}

/**
 * Sends packet
 * @param msg
 * @param ignoreQueue Used to send a packet ignoring the queue of them (used but need to make sure that the data is ready to be sent)
 * @return Number of bytes sent, SOCKET_ERROR on error
 */
int CExtendedSocket::Send(CSendPacket* msg, bool ignoreQueue)
{
	int result = 1;

	if (!ignoreQueue)
	{
		// add to the send queue
		m_SendPackets.push_back(msg);
	}
	else
	{
		auto data = msg->SetPacketLength();
		result = Send(data);

		if (result > 0 && msg->m_nPacketID == 7 && !m_bCryptOutput)
			SetCryptOutput(true);

		if (GetNetworkError() != WSAEWOULDBLOCK)
			delete msg;
	}

	return result;
}

/**
 * Reads TCP_CONNECTED_MESSAGE. Called when tcp client connected to the server.
 * @return false if received not TCP_CONNECTED_MESSAGE, true otherwise
 */
bool CExtendedSocket::OnServerConnected()
{
	vector<unsigned char> data;
	data.resize(sizeof(TCP_CONNECTED_MESSAGE));

	if (Read((char*)data.data(), data.size()) != data.size() - 1)
	{
		// doesn't look like TCP_CONNECTED_MESSAGE
		return false;
	}

	Buffer buf(data);
	if (buf.readStr() != TCP_CONNECTED_MESSAGE)
	{
		// not TCP_CONNECTED_MESSAGE
 		return false;
	}

	return true;
}

/**
 * Gets ID of ExtendedSocket
 * @return Number of bytes sent
 */
unsigned int CExtendedSocket::GetID()
{
	return m_nID;
}

/**
 * Gets socket object
 */
SOCKET CExtendedSocket::GetSocket()
{
	return m_Socket;
}

/**
 * Gets received message
 * @return Pointer to received message
 */
CReceivePacket* CExtendedSocket::GetMsg()
{
	return m_pMsg;
}

/**
 * Sets received message
 * @param msg
 */
void CExtendedSocket::SetMsg(CReceivePacket* msg)
{
	m_pMsg = msg;
}

/**
 * Gets read result
 * @return Last Read() result
 */
int CExtendedSocket::GetReadResult()
{
	return m_nReadResult;
}

/**
 * Gets number of bytes received by client
 * @return Bytes received
 */
int CExtendedSocket::GetBytesReceived()
{
	return m_nBytesReceived;
}

/**
 * Gets number of bytes sent by client
 * @return Bytes sent
 */
int CExtendedSocket::GetBytesSent()
{
	return m_nBytesSent;
}

/**
 * Gets packets that are in the queue for sending
 * @return Vector of packets to send
 */
vector<CSendPacket*>& CExtendedSocket::GetPacketsToSend()
{
	return m_SendPackets;
}

/**
 * Gets client guest data (some additional info about the client)
 * @return Guest data
 */
GuestData_s& CExtendedSocket::GetGuestData()
{
	return m_GuestData;
}