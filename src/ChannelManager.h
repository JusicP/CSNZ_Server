#pragma once

#include "ChannelServer.h"
#include "UserManager.h"
#include "User.h"

class CChannelManager
{
public:
	CChannelManager();

	bool OnChannelListPacket(CExtendedSocket* socket);
	bool OnRoomRequest(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnRoomListPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnLobbyMessage(CReceivePacket* msg, CExtendedSocket* socket, CUser* user);
	bool OnWhisperMessage(CReceivePacket* msg, CUser* user);
	bool OnRoomUserMessage(CReceivePacket* msg, CUser* user);
	bool OnRoomTeamUserMessage(CReceivePacket* msg, CUser* user);

	class CChannelServer* GetServerByIndex(int index);
	void JoinChannel(CUser* user, int channelServerID, int channelID, bool transfer);

	std::vector<CChannelServer*> channelServers;

private:
	bool OnCommandHandler(CExtendedSocket* socket, CUser* user, std::string message);

	bool OnNewRoomRequest(CReceivePacket* msg, CUser* user);
	bool OnJoinRoomRequest(CReceivePacket* msg, CUser* user);
	bool OnLeaveRoomRequest(CUser* user);
	bool OnToggleReadyRequest(CUser* user);
	bool OnConnectionFailure(CUser* user);
	bool OnGameStartRequest(CUser* user);
	bool OnCloseResultRequest(CUser* user);
	bool OnRoomUpdateSettings(CReceivePacket* msg, CUser* user);
	bool OnSetTeamRequest(CReceivePacket* msg, CUser* user);
	bool OnUserInviteRequest(CReceivePacket* msg, CUser* user);
	bool OnRoomSetZBAddonRequest(CReceivePacket* msg, CUser* user);
};