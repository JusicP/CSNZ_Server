#pragma once

#include "interface/iroom.h"
#include "roomsettings.h"
#include "net/receivepacket.h"
#include "definitions.h"

class CGameMatch;
class CDedicatedServer; // TODO: fix includes

class CRoomUser
{
public:
	CRoomUser(IUser* user, RoomTeamNum team, RoomReadyStatus ready)
	{
		m_bIsIngame = false;
		m_pUser = user;
		m_Team = team;
		m_Ready = ready;
	}

	IUser* m_pUser;
	RoomTeamNum m_Team;
	RoomReadyStatus m_Ready;
	bool m_bIsIngame;
};

class CRoom : public IRoom
{
public:
	CRoom(int roomId, IUser* hostUser, class CChannel* channel, CRoomSettings* settings);
	~CRoom();

	int GetNumOfPlayers();
	int GetFreeSlots();
	bool HasFreeSlots();
	bool HasPassword();
	bool HasUser(IUser* user);
	void AddUser(IUser* user);
	enum RoomTeamNum FindDesirableTeamNum();
	enum RoomTeamNum GetUserTeam(IUser* user);
	int GetNumOfReadyRealPlayers();
	int GetNumOfRealCts();
	int GetNumOfRealTerrorists();
	int GetNumOfReadyPlayers();
	RoomReadyStatus IsUserReady(IUser* user);
	bool IsRoomReady();
	bool IsUserIngame(IUser* user);
	void SetUserIngame(IUser* user, bool inGame);
	void RemoveUser(IUser* targetUser);
	//void RemoveUserById(int userId);
	void SetUserToTeam(IUser* user, RoomTeamNum newTeam);
	enum RoomStatus GetStatus();
	void SetStatus(RoomStatus newStatus);
	enum RoomReadyStatus ToggleUserReadyStatus(IUser* user);
	void ResetStatusIngameUsers();
	//bool CanStartGame();
	void OnUserRemoved(IUser* user);
	void SendRemovedUser(IUser* deletedUser);
	void UpdateHost(IUser* newHost);
	void HostStartGame();
	void UserGameJoin(IUser* user);
	void EndGame(bool forcedEnd);
	bool FindAndUpdateNewHost();
	void UpdateSettings(CRoomSettings& newSettings);
	void OnUserMessage(CReceivePacket* msg, IUser* user);
	void OnUserTeamMessage(CReceivePacket* msg, IUser* user);
	void OnGameStart();
	void KickUser(IUser* user);
	void VoteKick(IUser* user, bool kick);
	void SendJoinNewRoom(IUser* user);
	void SendRoomSettings(IUser* user);
	void SendUpdateRoomSettings(IUser* user, CRoomSettings* settings, int lowFlag, int lowMidFlag, int highMidFlag, int highFlag);
	void SendRoomUsersReadyStatus(IUser* user);
	void SendReadyStatusToAll();
	void SendReadyStatusToAll(IUser* user);
	void SendNewUser(IUser* user, IUser* newUser);
	void SendUserReadyStatus(IUser* user, IUser* player);
	void SendConnectHost(IUser* user, IUser* host);
	void SendGuestData(IUser* host, IUser* guest);
	void SendStartMatch(IUser* host);
	void SendCloseResultWindow(IUser* user);
	void SendTeamChange(IUser* user, IUser* player, RoomTeamNum newTeamNum);
	void SendGameEnd(IUser* user);
	void SendRoomStatus(IUser* user);
	void SendPlayerLeaveIngame(IUser* user);

	void CheckForHostItems();

	int GetID();
	IUser* GetHostUser();
	std::vector<IUser*> GetUsers();
	CRoomSettings* GetSettings();
	CGameMatch* GetGameMatch();
	CChannel* GetParentChannel();
	bool IsUserKicked(int userID);

	CDedicatedServer* GetServer();
	void SetServer(CDedicatedServer* server);
	void ChangeMap(int mapId);

private:
	IUser* m_pHostUser;
	class CChannel* m_pParentChannel;
	CRoomSettings* m_pSettings;
	std::vector<IUser*> m_Users;
	CGameMatch* m_pGameMatch;
	std::vector<int> m_KickedUsers;

	CDedicatedServer* m_pServer;

	int m_nID;
	RoomStatus m_Status;
};