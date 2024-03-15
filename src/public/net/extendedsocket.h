#pragma once

#include "interface/net/iextendedsocket.h"
#include "common/buffer.h"

struct GuestData_s
{
	bool isGuest;

	class CChannelServer* currentServer;
	class CChannel* currentChannel;

	int launcherVersion;
};

class CSendPacket;
class CReceivePacket;
struct WOLFSSL_EVP_CIPHER_CTX;

/**
 * Class that extends client sockets and sockets returned by accept() to store additional information
 * such as IP, some statistics, etc and to manage packets
 */
class CExtendedSocket : public IExtendedSocket
{
public:
	CExtendedSocket(SOCKET socket, unsigned int id = 0);
	~CExtendedSocket();

	bool SetupCrypt();
	void SetIP(const std::string& addr) { m_IP = addr; }
	void SetHWID(const std::vector<unsigned char>& hwid) { m_HWID = hwid; }
	const std::string& GetIP() { return m_IP; }
	const std::vector<unsigned char>& GetHWID() { return m_HWID; }
	void SetCryptInput(bool val) { m_bCryptInput = val; }
	void SetCryptOutput(bool val) { m_bCryptOutput = val; }
	unsigned char* GetCryptKey() { return m_pCryptKey; }
	unsigned char* GetCryptIV() { return m_pCryptIV; }
	int GetSeq();
	int LoggerGetSeq();
	void ResetSeq();
	int Read(char* buf, int len);
	CReceivePacket* Read();
	int Send(std::vector<unsigned char>& buffer, bool serverHelloMsg = false);
	int Send(CSendPacket* msg, bool ignoreQueue = false);

	// tcp client method
	bool OnServerConnected();

	unsigned int GetID();
	SOCKET GetSocket();
	void SetMsg(CReceivePacket* msg);
	CReceivePacket* GetMsg();
	int GetReadResult();
	int GetBytesReceived();
	int GetBytesSent();
	std::vector<CSendPacket*>& GetPacketsToSend();
	GuestData_s& GetGuestData();

private:
	unsigned int m_nID;
	SOCKET m_Socket;
	int m_nSequence;
	int m_nBytesReceived;
	int m_nBytesSent;

	GuestData_s m_GuestData;

	CReceivePacket* m_pMsg;
	
	int m_nPacketReceivedSize;
	
	int m_nReadResult;
	int m_nNextExpectedSeq; // TODO: we need it?

	std::string m_IP;
	std::vector<unsigned char> m_HWID;
	std::vector<CSendPacket*> m_SendPackets;

	// crypt things
	WOLFSSL_EVP_CIPHER_CTX* m_pDecEVPCTX;
	WOLFSSL_EVP_CIPHER_CTX* m_pEncEVPCTX;
	bool m_bCryptInput;
	bool m_bCryptOutput;
	unsigned char m_pCryptKey[64];
	unsigned char m_pCryptIV[64];
};
