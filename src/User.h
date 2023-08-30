#pragma once

#include "ExtendedSocket.h"
#include "UserDatabase.h"
#include "UserInventoryItem.h"
#include "Channel.h"
#include "Definitions.h"

class CUser
{
public:
	CUser(CExtendedSocket* socket, int userID, std::string userName);
	~CUser();

	void SetCurrentChannel(CChannel* channel);
	void SetLastChannelServer(CChannelServer* channelServer);
	void SetStatus(UserStatus newStatus);
	void SetCurrentRoom(CRoom* room);
	void SetRoomData(CRoomUser* roomUser);

	UserNetworkConfig_s GetNetworkConfig();
	CExtendedSocket* GetExtendedSocket();
	CChannel* GetCurrentChannel();
	CChannelServer* GetLastChannelServer();
	CRoom* GetCurrentRoom();
	CRoomUser* GetRoomData();
	UserStatus GetStatus();
	int GetUptime();
	int GetID();
	std::string GetUsername();
	const char* GetLogName();
	CUserData GetUser(int flag);
	CUserCharacter GetCharacter(int flag);
	CUserCharacterExtended GetCharacterExtended(int flag);

	int UpdateHolepunch(int portId, int localPort, int externalPort);
	void UpdateClientUserInfo(int flag, CUserCharacter character);
	void UpdateGameName(std::string gameName);
	int UpdatePoints(int64_t points);
	void UpdateCash(int64_t cash);
	void UpdateHonorPoints(int honorPoints);
	void UpdatePrefix(int prefixID);
	void UpdateStat(int battles, int win, int kills, int deaths);
	void UpdateLocation(int nation, int city, int town);
	void UpdateRank(int leagueID);
	void UpdateLevel(int level);
	void UpdateExp(int64_t exp);
	int UpdatePasswordBoxes(int passwordBoxes);
	void UpdateTitles(int slot, int titleID);
	void UpdateAchievementList(int titleID);
	void UpdateClan(int clanID);
	void UpdateTournament(int tournament);
	int UpdateBanList(std::string gameName, bool remove = false);
	void UpdateBanSettings(int settings);
	void UpdateNameplate(int nameplateID);
	void UpdateZbRespawnEffect(int zbRespawnEffect);
	void UpdateKillerMarkEffect(int killerMarkEffect);

	int CheckForLvlUp(int64_t exp);

	void OnTick();

	bool IsCharacterExists();
	bool CreateCharacter(std::string gameName);

private:
	CExtendedSocket* m_pSocket;

	// network data
	UserNetworkConfig_s m_NetworkData;

	CRoom* m_pCurrentRoom;
	CRoomUser* m_pRoomData;
	CChannel* m_pCurrentChannel;
	CChannelServer* m_pLastChannelServer;

	UserStatus m_Status;
	int m_nUptime;
	int m_nID;
	std::string m_UserName;
};