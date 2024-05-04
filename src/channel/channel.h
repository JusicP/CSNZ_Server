#pragma once

#include "room/room.h"

class CChannelServer;

class CChannel
{
public:
	CChannel(CChannelServer* server, int id, const std::string& channelName, int maxPlayers, const std::string& loginMsg);
	~CChannel();

	bool UserJoin(IUser* user, bool unhide = false);
	void UserLeft(IUser* user, bool hide = false);
	void SendFullUpdateRoomList();
	void SendFullUpdateRoomList(IUser* user);
	void SendUpdateRoomList(IRoom* room);
	void SendAddRoomToRoomList(IRoom* room);
	void SendRemoveFromRoomList(int roomId);
	void SendUserMessageToAllUser(int type, int senderUserID, const std::string& senderName, const std::string& msg);
	void UpdateUserInfo(IUser* user, const CUserCharacter& character);
	IRoom* CreateRoom(IUser* host, CRoomSettings* settings);
	IRoom* GetRoomById(int id);
	IUser* GetUserById(int userID);
	void RemoveRoom(IRoom* room);
	bool RemoveUser(IUser* user);

	int GetID();
	std::string GetName();
	std::vector<IRoom*> GetRooms();
	std::vector<IUser*> GetUsers();
	std::vector<IUser*> GetOutsideUsers();

	CChannelServer* GetParentChannelServer();

private:
	CChannelServer* m_pParentChannelServer;

	int m_nID;
	int m_nNextRoomID;
	int m_nMaxPlayers;
	std::string m_szName;
	std::string m_LoginMsg;

	std::vector<IRoom*> m_Rooms;
	std::vector<IUser*> m_Users;
};