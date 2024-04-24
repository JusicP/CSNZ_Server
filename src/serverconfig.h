#pragma once

#include "definitions.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

class CServerConfig
{
public:
	CServerConfig();
	~CServerConfig();

	bool Load();
	void LoadDefaultConfig(ordered_json& cfg);

	std::string hostName;
	std::string description;
	std::string tcpPort;
	std::string udpPort;
	int tcpSendBufferSize;
	int maxPlayers;
	std::string welcomeMessage;
	bool restartOnCrash;
	int inventorySlotMax;
	bool checkClientBuild;
	int allowedClientTimestamp;
	int allowedLauncherVersion;
	int maxRegistrationsPerIP;
	uint64_t metadataToSend;
	DefaultUser defUser;
	std::vector<Notice_s> notices;
	ServerConfigGameMatch_s gameMatch;
	ServerConfigRoom_s room;
	int activeMiniGamesFlag;
	int flockingFlyerType;
	ServerConfigBingo bingo;
	WeaponReleaseConfig weaponRelease;
	std::vector<std::string> nameBlacklist;
	std::vector<Survey> surveys;
	bool crypt;
	int mainMenuSkinEvent;
	int banListMaxSize;
};

extern class CServerConfig* g_pServerConfig;