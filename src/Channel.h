#pragma once

#include "Room.h"

class CChannel
{
public:
	CChannel(CChannelServer* server, int idx, std::string channelName, int maxPlayers, std::string loginMsg);

	bool UserJoin(CUser* user, bool unhide = false);
	void UserLeft(CUser* user, bool hide = false);
	void SendFullUpdateRoomList();
	void SendFullUpdateRoomList(CUser* user);
	void SendUpdateRoomList(CRoom* room);
	void SendAddRoomToRoomList(CRoom* room);
	void SendRemoveFromRoomList(int roomId);
	void OnEmptyRoomCallback(CRoom* room);
	void SendLobbyMessageToAllUser(std::string senderName, std::string msg);
	void UpdateUserInfo(CUser* user, const CUserCharacter& character);
	CRoom* CreateRoom(CUser* host, CRoomSettings* settings);
	CRoom* GetRoomById(int id);
	CUser* GetUserById(int userID);
	bool RemoveRoomById(int roomID);
	bool RemoveUserById(int userID);

	CChannelServer* GetParentChannelServer();

	int m_nIndex;
	std::string m_szName;

	std::vector<CRoom*> m_Rooms;
	std::vector<CUser*> m_Users;

private:
	CChannelServer* m_pParentChannelServer;

	int m_nNextRoomID;
	int m_nMaxPlayers;
	std::string m_LoginMsg;
};