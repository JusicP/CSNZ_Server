#pragma once

#include <string>
#include <vector>

class IUser;
class CGameMatch;
class CDedicatedServer;
class CReceivePacket;
class CRoomSettings;
class CChannel;
class CRoomUser;
enum RoomStatus;
enum RoomTeamNum;
enum RoomReadyStatus;

class IRoom
{
public:
	virtual int GetNumOfPlayers() = 0;
	virtual int GetFreeSlots() = 0;
	virtual bool HasFreeSlots() = 0;
	virtual bool HasPassword() = 0;
	virtual bool HasUser(IUser* user) = 0;
	virtual void AddUser(IUser* user) = 0;
	virtual RoomTeamNum FindDesirableTeamNum() = 0;
	virtual RoomTeamNum GetUserTeam(IUser* user) = 0;
	virtual int GetNumOfReadyRealPlayers() = 0;
	virtual int GetNumOfRealCts() = 0;
	virtual int GetNumOfRealTerrorists() = 0;
	virtual int GetNumOfReadyPlayers() = 0;
	virtual RoomReadyStatus IsUserReady(IUser* user) = 0;
	virtual bool IsRoomReady() = 0;
	virtual bool IsUserIngame(IUser* user) = 0;
	virtual void SetUserIngame(IUser* user, bool inGame) = 0;
	virtual void RemoveUser(IUser* targetUser) = 0;
	//virtual void RemoveUserById(int userId) = 0;
	virtual void SetUserToTeam(IUser* user, RoomTeamNum newTeam) = 0;
	virtual RoomStatus GetStatus() = 0;
	virtual void SetStatus(RoomStatus newStatus) = 0;
	virtual RoomReadyStatus ToggleUserReadyStatus(IUser* user) = 0;
	virtual void ResetStatusIngameUsers() = 0;
	//virtual bool CanStartGame() = 0;
	virtual void OnUserRemoved(IUser* user) = 0;
	virtual void SendRemovedUser(IUser* deletedUser) = 0;
	virtual void UpdateHost(IUser* newHost) = 0;
	virtual void HostStartGame() = 0;
	virtual void UserGameJoin(IUser* user) = 0;
	virtual void EndGame(bool forcedEnd) = 0;
	virtual bool FindAndUpdateNewHost() = 0;
	virtual void UpdateSettings(CRoomSettings& newSettings) = 0;
	virtual void OnUserMessage(CReceivePacket* msg, IUser* user) = 0;
	virtual void OnUserTeamMessage(CReceivePacket* msg, IUser* user) = 0;
	virtual void OnGameStart() = 0;
	virtual void KickUser(IUser* user) = 0;
	virtual void VoteKick(IUser* user, bool kick) = 0;
	virtual void SendJoinNewRoom(IUser* user) = 0;
	virtual void SendRoomSettings(IUser* user) = 0;
	virtual void SendUpdateRoomSettings(IUser* user, CRoomSettings* settings, int lowFlag, int lowMidFlag, int highMidFlag, int highFlag) = 0;
	virtual void SendRoomUsersReadyStatus(IUser* user) = 0;
	virtual void SendReadyStatusToAll() = 0;
	virtual void SendReadyStatusToAll(IUser* user) = 0;
	virtual void SendNewUser(IUser* user, IUser* newUser) = 0;
	virtual void SendUserReadyStatus(IUser* user, IUser* player) = 0;
	virtual void SendConnectHost(IUser* user, IUser* host) = 0;
	virtual void SendGuestData(IUser* host, IUser* guest) = 0;
	virtual void SendStartMatch(IUser* host) = 0;
	virtual void SendCloseResultWindow(IUser* user) = 0;
	virtual void SendTeamChange(IUser* user, IUser* player, RoomTeamNum newTeamNum) = 0;
	virtual void SendGameEnd(IUser* user) = 0;
	virtual void SendRoomStatus(IUser* user) = 0;
	virtual void SendPlayerLeaveIngame(IUser* user) = 0;

	virtual int GetID() = 0;
	virtual IUser* GetHostUser() = 0;
	virtual std::vector<IUser*> GetUsers() = 0;
	virtual CRoomSettings* GetSettings() = 0;
	virtual CGameMatch* GetGameMatch() = 0;
	virtual CChannel* GetParentChannel() = 0;
	virtual bool IsUserKicked(int userID) = 0;

	virtual CDedicatedServer* GetServer() = 0;
	virtual void SetServer(CDedicatedServer* server) = 0;
	virtual void ChangeMap(int mapId) = 0;
};