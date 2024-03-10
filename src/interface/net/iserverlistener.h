#pragma once

#include "common/buffer.h"

class CReceivePacket;
class IExtendedSocket;

class IServerListenerTCP
{
public:
	virtual void OnTCPConnectionCreated(IExtendedSocket* socket) = 0;
	virtual void OnTCPConnectionClosed(IExtendedSocket* socket) = 0;
	virtual void OnTCPMessage(IExtendedSocket* socket, CReceivePacket* msg) = 0;
	virtual void OnTCPError(int errorCode) = 0;
};

class IServerListenerUDP
{
public:
	virtual void OnUDPMessage(Buffer& buf, unsigned short port) = 0;
	virtual void OnUDPError(int errorCode) = 0;
};

class IClientListenerTCP
{
public:
	virtual void OnTCPServerConnected() = 0;
	virtual void OnTCPServerConnectFailed() = 0;
	virtual void OnTCPMessage(CReceivePacket* msg) = 0;
	virtual void OnTCPError(int errorCode) = 0;
};