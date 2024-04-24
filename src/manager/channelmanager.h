#pragma once

#include "interface/ichannelmanager.h"
#include "manager.h"

#include "channel/channelserver.h"
//#include "manager/usermanager.h"
#include "user/user.h"

class CChannelManager : public CBaseManager<IChannelManager>
{
public:
	CChannelManager();

	bool OnChannelListPacket(IExtendedSocket* socket);
	bool OnRoomRequest(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnRoomListPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnLobbyMessage(CReceivePacket* msg, IExtendedSocket* socket, IUser* user);
	bool OnWhisperMessage(CReceivePacket* msg, IUser* user);
	bool OnRoomUserMessage(CReceivePacket* msg, IUser* user);
	bool OnRoomTeamUserMessage(CReceivePacket* msg, IUser* user);
	bool OnServerYellMessage(CReceivePacket* msg, IUser* user);

	class CChannelServer* GetServerByIndex(int index);
	void JoinChannel(IUser* user, int channelServerID, int channelID, bool transfer);
	void EndAllGames();

	std::vector<CChannelServer*> channelServers;

private:
	bool OnCommandHandler(IExtendedSocket* socket, IUser* user, const std::string& message);

	bool OnNewRoomRequest(CReceivePacket* msg, IUser* user);
	bool OnJoinRoomRequest(CReceivePacket* msg, IUser* user);
	bool OnLeaveRoomRequest(IUser* user);
	bool OnToggleReadyRequest(IUser* user);
	bool OnConnectionFailure(IUser* user);
	bool OnGameStartRequest(IUser* user);
	bool OnCloseResultRequest(IUser* user);
	bool OnRoomUpdateSettings(CReceivePacket* msg, IUser* user);
	bool OnSetTeamRequest(CReceivePacket* msg, IUser* user);
	bool OnUserInviteRequest(CReceivePacket* msg, IUser* user);
	bool OnRoomSetZBAddonRequest(CReceivePacket* msg, IUser* user);
};

extern CChannelManager g_ChannelManager;