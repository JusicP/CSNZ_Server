#pragma once

#include "RoomSettings.h"
#include "ReceivePacket.h"
#include "Definitions.h"

class CGameMatch;
class CDedicatedServer; // TODO: fix includes

class CRoomUser
{
public:
	CRoomUser(CUser* user, RoomTeamNum team, RoomReadyStatus ready)
	{
		m_bIsIngame = false;
		m_pUser = user;
		m_Team = team;
		m_Ready = ready;
	}

	CUser* m_pUser;
	RoomTeamNum m_Team;
	RoomReadyStatus m_Ready;
	bool m_bIsIngame;
};

class CRoom
{
public:
	CRoom(int roomId, CUser* hostUser, class CChannel* channel, CRoomSettings* settings);
	~CRoom();

	int GetNumOfPlayers();
	int GetFreeSlots();
	bool HasFreeSlots();
	bool HasPassword();
	bool HasUser(CUser* user);
	void AddUser(CUser* user);
	enum RoomTeamNum FindDesirableTeamNum();
	enum RoomTeamNum GetUserTeam(CUser* user);
	int GetNumOfReadyRealPlayers();
	int GetNumOfRealCts();
	int GetNumOfRealTerrorists();
	int GetNumOfReadyPlayers();
	enum RoomReadyStatus GetUserReadyStatus(CUser* user);
	RoomReadyStatus IsUserReady(CUser* user);
	bool IsRoomReady();
	bool IsUserIngame(CUser* user);
	void SetUserIngame(CUser* user, bool inGame);
	void RemoveUser(CUser* targetUser);
	void RemoveUserById(int userId);
	void SetUserToTeam(CUser* user, RoomTeamNum newTeam);
	enum RoomStatus GetStatus();
	void SetStatus(RoomStatus newStatus);
	enum RoomReadyStatus ToggleUserReadyStatus(CUser* user);
	void ResetStatusIngameUsers();
	bool CanStartGame();
	void OnUserRemoved(CUser* user);
	void SendRemovedUser(CUser* deletedUser);
	void UpdateHost(CUser* newHost);
	void HostStartGame();
	void UserGameJoin(CUser* user);
	void EndGame();
	bool FindAndUpdateNewHost();
	void UpdateSettings(CRoomSettings newSettings);
	void OnUserMessage(CReceivePacket* msg, CUser* user);
	void OnUserTeamMessage(CReceivePacket* msg, CUser* user);
	void OnGameStart();
	void KickUser(CUser* user);
	void VoteKick(CUser* user, bool kick);
	void SendJoinNewRoom(CUser* user);
	void SendRoomSettings(CUser* user);
	void SendUpdateRoomSettings(CUser* user, CRoomSettings* settings, int lowFlag, int lowMidFlag, int highMidFlag, int highFlag);
	void SendRoomUsersReadyStatus(CUser* user);
	void SendReadyStatusToAll();
	void SendReadyStatusToAll(CUser* user);
	void SendNewUser(CUser* user, CUser* newUser);
	void SendUserReadyStatus(CUser* user, CUser* player);
	void SendConnectHost(CUser* user, CUser* host);
	void SendGuestData(CUser* host, CUser* guest);
	void SendStartMatch(CUser* host);
	void SendCloseResultWindow(CUser* user);
	void SendTeamChange(CUser* user, CUser* player, RoomTeamNum newTeamNum);
	void SendGameEnd(CUser* user);
	void SendUserMessage(std::string senderName, std::string msg, CUser* user);
	void SendRoomStatus(CUser* user);
	void SendPlayerLeaveIngame(CUser* user);

	int GetID();
	CUser* GetHostUser();
	std::vector<CUser*> GetUsers();
	CRoomSettings* GetSettings();
	CGameMatch* GetGameMatch();
	CChannel* GetParentChannel();
	bool IsUserKicked(int userID);

	CDedicatedServer* GetServer();
	void SetServer(CDedicatedServer* server);

private:
	CUser* m_pHostUser;
	class CChannel* m_pParentChannel;
	CRoomSettings* m_pSettings;
	std::vector<CUser*> m_Users;
	CGameMatch* m_pGameMatch;
	std::vector<int> m_KickedUsers;

	CDedicatedServer* m_pServer;

	int m_nID;
	RoomStatus m_Status;
};

typedef struct IRoomOptions_s
{
	std::string roomName;
	int gameModeId;
	int mapId;
	int winLimit;
	int killLimit;
	int startMoney;
	int forceCamera;
	int nextMapEnabled;
	int changeTeams;
	int enableBots;
	int difficulty;
	int respawnTime;
	int teamBalance;
	int weaponRestrictions;
	int hltvEnabled;
} IRoomOptions;
