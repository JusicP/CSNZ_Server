#pragma once

#include <string>
#include <vector>

enum UserStatus;
class IExtendedSocket;
class IRoom;
class CRoomUser;
struct UserNetworkConfig_s;
class CChannel;
class CChannelServer;
class CUserData;
class CUserCharacter;
class CUserCharacterExtended;

struct UserData
{
	int m_nID;
	std::string m_UserName;
	UserStatus m_Status;
	int m_nUptime;
	int m_nCurrentChannelID;
	int m_nCurrentRoomID;
};

class IUser
{
public:
	virtual ~IUser() {}

	virtual void SetCurrentChannel(CChannel* channel) = 0;
	virtual void SetLastChannelServer(CChannelServer* channelServer) = 0;
	virtual void SetStatus(UserStatus newStatus) = 0;
	virtual void SetCurrentRoom(IRoom* room) = 0;
	virtual void SetRoomData(CRoomUser* roomUser) = 0;

	virtual UserNetworkConfig_s GetNetworkConfig() = 0;
	virtual IExtendedSocket* GetExtendedSocket() = 0;
	virtual CChannel* GetCurrentChannel() = 0;
	virtual CChannelServer* GetLastChannelServer() = 0;
	virtual IRoom* GetCurrentRoom() = 0;
	virtual CRoomUser* GetRoomData() = 0;
	virtual UserStatus GetStatus() = 0;
	virtual int GetUptime() = 0;
	virtual int GetID() = 0;
	virtual std::string GetUsername() = 0;
	virtual const char* GetLogName() = 0;
	virtual CUserData GetUser(int flag) = 0;
	virtual CUserCharacter GetCharacter(int flag) = 0;
	virtual CUserCharacterExtended GetCharacterExtended(int flag) = 0;

	virtual int UpdateHolepunch(int portId, const std::string& localIpAddress, int localPort, int externalPort) = 0;
	virtual void UpdateClientUserInfo(int flag, CUserCharacter character) = 0;
	virtual void UpdateGameName(const std::string& gameName) = 0;
	virtual int UpdatePoints(int64_t points) = 0;
	virtual void UpdateCash(int64_t cash) = 0;
	virtual void UpdateHonorPoints(int honorPoints) = 0;
	virtual void UpdatePrefix(int prefixID) = 0;
	virtual void UpdateStat(int battles, int win, int kills, int deaths) = 0;
	virtual void UpdateLocation(int nation, int city, int town) = 0;
	virtual void UpdateRank(int leagueID) = 0;
	virtual void UpdateLevel(int level) = 0;
	virtual void UpdateExp(int64_t exp) = 0;
	virtual int UpdatePasswordBoxes(int passwordBoxes) = 0;
	virtual void UpdateTitles(int slot, int titleID) = 0;
	virtual void UpdateAchievementList(int titleID) = 0;
	virtual void UpdateClan(int clanID) = 0;
	virtual void UpdateTournament(int tournament) = 0;
	virtual int UpdateBanList(const std::string& gameName, bool remove = false) = 0;
	virtual void UpdateBanSettings(int settings) = 0;
	virtual void UpdateNameplate(int nameplateID) = 0;
	virtual void UpdateZbRespawnEffect(int zbRespawnEffect) = 0;
	virtual void UpdateKillerMarkEffect(int killerMarkEffect) = 0;

	virtual int CheckForLvlUp(int64_t exp) = 0;

	virtual void OnTick() = 0;

	virtual bool IsCharacterExists() = 0;
	virtual bool CreateCharacter(const std::string& gameName) = 0;
};