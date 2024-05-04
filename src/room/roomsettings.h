#pragma once

#include "common/buffer.h"

class IUser;

struct unk33_data
{
	int unk1;
	int unk2;
	int unk3;
	int unk4;
	int unk5;
	int unk6;
	int unk7;
	int unk8;
	int unk9;
};

struct mapPlaylist_data
{
	int unk1;
	int mapId;
};

class CRoomSettings
{
public:
	CRoomSettings();
	CRoomSettings(Buffer& inPacket);

	void Init();
	int GetGameModeDefaultSetting(int gameModeId, const std::string& setting);
	int GetMapSetting(int mapId, const std::string& setting);
	int GetGameModeDefaultWeaponLimit(int gameModeId);
	int GetMapDefaultWeaponRestrict(int mapId);
	int GetDefaultBuyTime(int gameModeId);
	int GetDefaultTeamBalance(int gameModeId);
	int GetDefaultFriendlyFire(int gameModeId);
	int GetDefaultViewFlag(int gameModeId);
	int GetDefaultFriendlyBots(int gameModeId);
	int GetDefaultEnemyBots(int gameModeId);
	int GetDefaultBotAdd(int gameModeId);
	int GetDefaultStartingCash(int gameModeId);
	int GetDefaultZbRespawn(int gameModeId);
	int GetDefaultZbBalance(int gameModeId);
	bool IsFunGameMode(int gameModeId);
	bool IsPlayroomGameMode(int gameModeId);
	bool IsVoxelGameMode(int gameModeId);
	std::string GetGameModeNameByID(int gameModeId);
	bool IsMapValid(int gameModeId, int mapId);
	bool IsMapPlaylistAllowed(int gameModeId);
	bool IsRandomMapAllowed(int gameModeId);
	void LoadDefaultSettings(int gameModeId, int mapId);
	void LoadZbCompetitiveSettings(int gameModeId);
	void LoadNewSettings(int gameModeId, int mapId, IUser* user);
	bool IsSettingValid(int gameModeId, const std::string& setting, int value);
	bool IsLeagueRuleWinLimitValid(int winLimit);
	bool IsBuyTimeValid(int gameModeId, int buyTime);
	bool IsStartingCashValid(int gameModeId, int startingCash);
	bool IsStartingCashValid(int startingCash);
	bool IsZombieItem(int itemId);
	bool IsZbLimitValid(const std::vector<int>& zbLimit);
	bool IsMutationRestrictValid(const std::vector<int>& mutationRestrictList);
	bool IsMapPlaylistValid(const std::vector<mapPlaylist_data>& mapPlaylist);
	bool IsMutationLimitValid(int mutationLimit);
	bool CanChangeTeamBalance(int gameModeId);
	bool CanChangeFriendlyFire(int gameModeId);
	bool CheckSettings(IUser* user);
	bool CheckNewSettings(IUser* user, CRoomSettings* roomSettings);

public:
	int lowFlag;
	int lowMidFlag;
	int highMidFlag;
	int highFlag;
	std::string roomName;
	int unk00;
	int unk01;
	int unk02;
	int unk03;
	int unk04;
	std::string password;
	int levelLimit;
	int unk07;
	int gameModeId;
	int mapId;
	int maxPlayers;
	int winLimit;
	int killLimit;
	int gameTime;
	int roundTime;
	int weaponLimit;
	int hostageKillLimit;
	int freezeTime;
	int buyTime;
	int displayNickname;
	int teamBalance;
	int unk21;
	int friendlyFire;
	int flashlight;
	int footsteps;
	int unk25;
	int tkPunish;
	int autoKick;
	int unk28;
	int unk29;
	int viewFlag;
	int voiceChat;
	int status; // isIngame
	int unk33;
	std::vector<unk33_data> unk33_vec;
	int unk34;
	std::string unk35;
	int unk36;
	int unk37;
	int unk38;
	int c4Timer;
	int botDifficulty;
	int friendlyBots;
	int enemyBots;
	int botBalance;
	int botAdd;
	int kdRule;
	int startingCash;
	int movingShot;
	int ballNumber;
	int statusSymbol;
	int randomMap;
	int mapPlaylistSize;
	std::vector<mapPlaylist_data> mapPlaylist;
	int mapPlaylistIndex;
	int enhanceRestrict;
	int sd;
	int zsDifficulty;
	int unk56;
	int unk57;
	int leagueRule;
	int mannerLimit;
	int mapId2;
	int zbLimitFlag;
	std::vector<int> zbLimit;
	int unk62;
	int unk63;
	std::vector<int> unk63_vec;
	int unk64;
	int teamSwitch;
	int zbRespawn;
	int zbBalance;
	int gameRule;
	int superRoom;
	int isZbCompetitive;
	int zbAutoHunting;
	int integratedTeam;
	int unk73;
	int fireBomb;
	int mutationRestrict;
	std::vector<int> mutationRestrictList;
	int mutationLimit;
	int unk77;
	int floatingDamageSkin;
	int playerOneTeam;
	int weaponRestrict;
};
