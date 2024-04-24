#pragma once

#include "imanager.h"

class IUser;

class IChannelManager : public IBaseManager
{
public:
	virtual bool OnChannelListPacket(IExtendedSocket* socket) = 0;
	virtual bool OnRoomRequest(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnRoomListPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnLobbyMessage(CReceivePacket* msg, IExtendedSocket* socket, IUser* user) = 0;
	virtual bool OnWhisperMessage(CReceivePacket* msg, IUser* user) = 0;
	virtual bool OnRoomUserMessage(CReceivePacket* msg, IUser* user) = 0;
	virtual bool OnRoomTeamUserMessage(CReceivePacket* msg, IUser* user) = 0;
	virtual bool OnServerYellMessage(CReceivePacket* msg, IUser* user) = 0;

	virtual class CChannelServer* GetServerByIndex(int index) = 0;
	virtual void JoinChannel(IUser* user, int channelServerID, int channelID, bool transfer) = 0;
};