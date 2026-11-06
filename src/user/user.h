#pragma once

#include "interface/iuser.h"

#include "net/extendedsocket.h"
#include "channel/channel.h"
#include "definitions.h"

class CUser : public IUser
{
public:
	CUser(IExtendedSocket* socket, int userID, const std::string& userName);
	~CUser();

	void SetCurrentChannel(CChannel* channel);
	void SetLastChannelServer(CChannelServer* channelServer);
	void SetStatus(UserStatus newStatus);
	void SetCurrentRoom(IRoom* room);
	void SetRoomData(CRoomUser* roomUser);

	UserNetworkConfig_s GetNetworkConfig();
	IExtendedSocket* GetExtendedSocket();
	CChannel* GetCurrentChannel();
	CChannelServer* GetLastChannelServer();
	IRoom* GetCurrentRoom();
	CRoomUser* GetRoomData();
	UserStatus GetStatus();
	bool IsPlaying();
	int GetUptime();
	int GetID();
	std::string GetUsername();
	const char* GetLogName();
	CUserData GetUser(int flag);
	CUserCharacter GetCharacter(int lowFlag, int highFlag = 0);
	CUserCharacterExtended GetCharacterExtended(int flag);

	void UpdateHolepunch(unsigned char portType, unsigned long externalIP, unsigned short externalPort, unsigned long localIP, unsigned short localPort, unsigned long long currentTime);
	void UpdateUDPHeartbeat(unsigned char portType, unsigned long long currentTime);
	void UpdateClientUserInfo(CUserCharacter character);
	void UpdateGameName(const std::string& gameName);
	int UpdatePoints(int64_t points);
	void UpdateCash(int64_t cash);
	void UpdateHonorPoints(int honorPoints);
	void UpdatePrefix(int prefixID);
	void UpdateStat(int battles, int win, int kills, int deaths);
	void UpdateLocation(int nation, int city, int town);
	void UpdateChatColor(int chatColorID);
	void UpdateRank(int leagueID);
	void UpdateLevel(int level);
	void UpdateExp(int64_t exp);
	int UpdatePasswordBoxes(int passwordBoxes);
	void UpdateTitles(int slot, int titleID);
	void UpdateAchievementList(int titleID);
	void UpdateClan(int clanID);
	void UpdateTournament(int tournament);
	int UpdateBanList(const std::string& gameName, bool remove = false);
	void UpdateBanSettings(int settings);
	void UpdateNameplate(int nameplateID);
	void UpdateZbRespawnEffect(int zbRespawnEffect);
	void UpdateKillerMarkEffect(int killerMarkEffect);

	int CheckForLvlUp(int64_t exp);

	void OnTick();

	bool IsCharacterExists();
	bool CreateCharacter(const std::string& gameName);

private:
	IExtendedSocket* m_pSocket;

	// network data
	UserNetworkConfig_s m_NetworkData;

	IRoom* m_pCurrentRoom;
	CRoomUser* m_pRoomData;
	CChannel* m_pCurrentChannel;
	CChannelServer* m_pLastChannelServer;

	UserStatus m_Status;
	int m_nUptime;
	int m_nID;
	std::string m_UserName;
};