#include "roomsettings.h"
#include "definitions.h"
#include "manager/userdatabase.h"

#include "user/user.h"
#include "user/userinventoryitem.h"

#include "serverconfig.h"

using namespace std;

CRoomSettings::CRoomSettings()
{
	Init();
}

CRoomSettings::CRoomSettings(Buffer& inPacket) // unfinished
{
	Init();

	// TODO: implement 128-bit bitwise operations...
	lowFlag = inPacket.readUInt32_LE();
	lowMidFlag = inPacket.readUInt32_LE();
	highMidFlag = inPacket.readUInt32_LE();
	highFlag = inPacket.readUInt32_LE();

	if (lowFlag & ROOM_LOW_ROOMNAME) {
		roomName = inPacket.readStr().c_str();
	}
	if (lowFlag & ROOM_LOW_UNK) {
		unk00 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_CLANBATTLE) {
		unk01 = inPacket.readUInt8();
		unk02 = inPacket.readUInt8();
		unk03 = inPacket.readUInt8();
		unk04 = inPacket.readUInt32_LE();
	}
	if (lowFlag & ROOM_LOW_PASSWORD) {
		password = inPacket.readStr().c_str();
	}
	if (lowFlag & ROOM_LOW_LEVELLIMIT) {
		levelLimit = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK7) {
		unk07 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_GAMEMODEID) {
		gameModeId = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_MAPID) {
		mapId = inPacket.readUInt16_LE();
	}
	if (lowFlag & ROOM_LOW_MAXPLAYERS) {
		maxPlayers = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_WINLIMIT) {
		winLimit = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_KILLLIMIT) {
		killLimit = inPacket.readUInt16_LE();
	}
	if (lowFlag & ROOM_LOW_GAMETIME) {
		gameTime = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_ROUNDTIME) {
		roundTime = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_ARMSRESTRICTION) {
		armsRestriction = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_HOSTAGEKILLLIMIT) {
		hostageKillLimit = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FREEZETIME) {
		freezeTime = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_BUYTIME) {
		buyTime = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_DISPLAYNICKNAME) {
		displayNickname = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_TEAMBALANCE) {
		teamBalance = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK21) {
		unk21 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FRIENDLYFIRE) {
		friendlyFire = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FLASHLIGHT) {
		flashlight = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_FOOTSTEPS) {
		footsteps = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK25) {
		unk25 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_TKPUNISH) {
		tkPunish = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_AUTOKICK) {
		autoKick = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK28) {
		unk28 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK29) {
		unk29 = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_VIEWFLAG) {
		viewFlag = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_VOICECHAT) {
		voiceChat = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_STATUS) {
		status = inPacket.readUInt8();
	}
	if (lowFlag & ROOM_LOW_UNK33) {
		unk33 = inPacket.readUInt8();
		for (int i = 0; i < 2; i++)
		{
			unk33_data dat;
			dat.unk1 = inPacket.readUInt32_LE();
			dat.unk2 = inPacket.readUInt32_LE();
			dat.unk3 = inPacket.readUInt8();
			dat.unk4 = inPacket.readUInt16_LE();
			dat.unk5 = inPacket.readUInt8();
			dat.unk6 = inPacket.readUInt8();
			dat.unk7 = inPacket.readUInt16_LE();
			dat.unk8 = inPacket.readUInt8();
			dat.unk9 = inPacket.readUInt8();

			unk33_vec.push_back(dat);
		}
	}

	if (lowMidFlag & ROOM_LOWMID_UNK34) {
		unk34 = inPacket.readUInt32_LE();
		unk35 = inPacket.readStr();
		unk36 = inPacket.readUInt8();
		unk37 = inPacket.readUInt8();
		unk38 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_C4TIMER) {
		c4Timer = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_BOT) {
		botDifficulty = inPacket.readUInt8();
		friendlyBots = inPacket.readUInt8();
		enemyBots = inPacket.readUInt8();
		botBalance = inPacket.readUInt8();
		botAdd = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_KDRULE) {
		kdRule = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_STARTINGCASH) {
		startingCash = inPacket.readUInt16_LE();
	}
	if (lowMidFlag & ROOM_LOWMID_MOVINGSHOT) {
		movingShot = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_BALLNUMBER) {
		ballNumber = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_STATUSSYMBOL) {
		statusSymbol = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_RANDOMMAP) {
		randomMap = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_MAPPLAYLIST) {
		mapPlaylistSize = inPacket.readUInt8();
		for (int i = 0; i < mapPlaylistSize; i++)
		{
			mapPlaylist_data dat;
			dat.unk1 = inPacket.readUInt8();
			dat.mapId = inPacket.readUInt16_LE();

			mapPlaylist.push_back(dat);
		}
	}
	if (lowMidFlag & ROOM_LOWMID_MAPPLAYLISTINDEX) {
		mapPlaylistIndex = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ENHANCERESTRICT) {
		enhanceRestrict = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_SD) {
		sd = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ZSDIFFICULTY) {
		zsDifficulty = inPacket.readUInt8();
		unk56 = inPacket.readUInt32_LE();
		unk57 = inPacket.readUInt32_LE();
	}
	if (lowMidFlag & ROOM_LOWMID_LEAGUERULE) {
		leagueRule = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_MANNERLIMIT) {
		mannerLimit = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_MAPID2) {
		mapId2 = inPacket.readUInt16_LE();
	}
	if (lowMidFlag & ROOM_LOWMID_ZBLIMIT) {
		zbLimitFlag = inPacket.readUInt8();
		for (int i = 0; i < 4; i++)
		{
			zbLimit.push_back(inPacket.readUInt32_LE());
		}
	}
	if (lowMidFlag & ROOM_LOWMID_UNK62) {
		unk62 = inPacket.readUInt32_LE(); // user char flag
		// shit related to studio mode should be here
		// if (unk62)
	}
	if (lowMidFlag & ROOM_LOWMID_UNK63) {
		unk63 = inPacket.readUInt8();
		for (int i = 0; i < unk63; i++)
		{
			unk63_vec.push_back(inPacket.readUInt16_LE());
		}
	}
	if (lowMidFlag & ROOM_LOWMID_UNK64) {
		unk64 = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_TEAMSWITCH) {
		teamSwitch = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ZBRESPAWN) {
		zbRespawn = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ZBBALANCE) {
		zbBalance = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_GAMERULE) {
		gameRule = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_SUPERROOM) {
		superRoom = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ISZBCOMPETITIVE) {
		isZbCompetitive = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_ZBAUTOHUNTING) {
		zbAutoHunting = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_INTEGRATEDTEAM) {
		integratedTeam = inPacket.readUInt8();
	}
	if (lowMidFlag & ROOM_LOWMID_UNK73) {
		unk73 = inPacket.readUInt8();
	}

	if (highMidFlag & ROOM_HIGHMID_FIREBOMB) {
		fireBomb = inPacket.readUInt8();
	}
	if (highMidFlag & ROOM_HIGHMID_MUTATIONRESTRICT) {
		mutationRestrictSize = inPacket.readUInt8();
		for (int i = 0; i < 4; i++)
		{
			mutationRestrict.push_back(inPacket.readUInt8());
		}
	}
	if (highMidFlag & ROOM_HIGHMID_MUTATIONLIMIT) {
		mutationLimit = inPacket.readUInt8();
	}
	if (highMidFlag & ROOM_HIGHMID_UNK78) {
		unk78 = inPacket.readUInt8();
	}
	if (highMidFlag & ROOM_HIGHMID_UNK79) {
		unk79 = inPacket.readUInt8();
	}
	if (highFlag & ROOM_HIGH_UNK77) {
		unk77 = inPacket.readUInt8();
	}

	if (highFlag)
	{
		Console().Warn("CRoomSettings: got high flag!\n");
	}
}

void CRoomSettings::Init()
{
	lowFlag = 0;
	lowMidFlag = 0;
	highMidFlag = 0;
	highFlag = 0;

	roomName = "";
	unk00 = 0;
	unk01 = 0;
	unk02 = 0;
	unk03 = 0;
	unk04 = 0;
	password = "";
	levelLimit = 0;
	unk07 = 0;
	gameModeId = 0;
	mapId = 0;
	maxPlayers = 0;
	winLimit = 0;
	killLimit = 0;
	gameTime = 0;
	roundTime = 0;
	armsRestriction = 0;
	hostageKillLimit = 0;
	freezeTime = 0;
	buyTime = 0;
	displayNickname = 0;
	teamBalance = 0;
	unk21 = 0;
	friendlyFire = 0;
	flashlight = 0;
	footsteps = 0;
	unk25 = 0;
	tkPunish = 0;
	autoKick = 0;
	unk28 = 0;
	unk29 = 0;
	viewFlag = 0;
	voiceChat = 0;
	status = 0;
	unk33 = 0;
	unk34 = 0;
	unk35 = "";
	unk36 = 0;
	unk37 = 0;
	unk38 = 0;
	c4Timer = 0;
	botDifficulty = 0;
	friendlyBots = 0;
	enemyBots = 0;
	botBalance = 0;
	botAdd = 0;
	kdRule = 0;
	startingCash = 0;
	movingShot = 0;
	ballNumber = 0;
	statusSymbol = 0;
	randomMap = 0;
	mapPlaylistSize = 0;
	mapPlaylistIndex = 0;
	enhanceRestrict = 0;
	sd = 0;
	zsDifficulty = 0;
	unk56 = 0;
	unk57 = 0;
	leagueRule = 0;
	mannerLimit = 0;
	mapId2 = 0;
	zbLimitFlag = 0;
	unk62 = 0;
	unk63 = 0;
	unk64 = 0;
	teamSwitch = 0;
	zbRespawn = 0;
	zbBalance = 0;
	gameRule = 0;
	superRoom = 0;
	isZbCompetitive = 0;
	zbAutoHunting = 0;
	integratedTeam = 0;
	unk73 = 0;
	fireBomb = 0;
	mutationRestrictSize = 0;
	mutationLimit = 0;
	unk77 = 0; // high flag
	unk78 = 0; // high mid flag
	unk79 = 0;
}

vector<int> split(const string& s, char delimiter)
{
	vector<int> splits;
	string split;
	istringstream ss(s);

	while (getline(ss, split, delimiter))
	{
		splits.push_back(atoi(split.c_str()));
	}

	return splits;
}

bool CRoomSettings::IsSettingValid(int gameModeId, const string& setting, int value)
{
	vector<int> validValues = split(g_pGameModeListTable->GetCell<string>(setting, to_string(gameModeId)), '|');

	if (validValues.empty())
		return true;

	for (int i = validValues[0]; i <= validValues[1]; i += validValues[3])
	{
		if (value == i)
			return true;

		if (!validValues[3])
			break;
	}

	return false;
}

bool CRoomSettings::IsLeagueRuleWinLimitValid(int winLimit)
{
	for (int i = 6; i <= 16; i += 2)
	{
		if (winLimit == i)
			return true;
	}

	return false;
}

bool CRoomSettings::IsBuyTimeValid(int gameModeId, int buyTime)
{
	if (gameModeId == 32)
		return (buyTime == 17 || buyTime == 20);
	else
	{
		for (int i = 10; i <= 180; i += 10)
		{
			if (buyTime == i)
				return true;
		}
	}

	return false;
}

bool CRoomSettings::IsStartingCashValid(int gameModeId, int startingCash)
{
	if (gameModeId == 17)
	{
		if (startingCash == 800 || startingCash == 7500)
			return true;
	}
	else if (startingCash == 800 || startingCash == 2400 || startingCash == 5000 || startingCash == 7500)
		return true;

	return false;
}

bool CRoomSettings::IsZombieItem(int itemId)
{
	switch (itemId)
	{
		case 163:
		case 175:
		case 200:
		case 216:
		case 305:
		case 358:
		case 388:
		case 390:
		case 391:
		case 459:
		case 460:
		case 8079:
		case 8080:
		case 8115:
		case 8138:
		case 8222:
			return true;
	}

	return false;
}

bool CRoomSettings::IsZbLimitValid(const vector<int>& zbLimit)
{
	bool empty = all_of(zbLimit.begin(), zbLimit.end(), [](int i) { return i == 0; });
	if (!empty)
	{
		for (int i = 0; i < 4; i++)
		{
			int current = zbLimit[i];
			if (current != 0)
			{
				if (!IsZombieItem(current))
					return false;

				for (int j = i + 1; j < 4; j++)
				{
					if (current == zbLimit[j])
						return false;
				}
			}
		}
	}

	return true;
}

bool CRoomSettings::IsMutationRestrictValid(const vector<int>& mutationRestrict)
{
	bool empty = all_of(mutationRestrict.begin(), mutationRestrict.end(), [](int i) { return i == -1; });
	if (!empty)
	{
		for (int i = 0; i < 4; i++)
		{
			int current = mutationRestrict[i];
			if (current != -1)
			{
				if (current > 71)
					return false;

				for (int j = i + 1; j < 4; j++)
				{
					if (current == mutationRestrict[j])
						return false;
				}
			}
		}
	}

	return true;
}

bool CRoomSettings::IsMapPlaylistValid(const vector<mapPlaylist_data>& mapPlaylist)
{
	for (size_t i = 0; i < mapPlaylist.size(); i++)
	{
		int current = mapPlaylist[i].mapId;
		for (size_t j = i + 1; j < mapPlaylist.size(); j++)
		{
			if (current == mapPlaylist[j].mapId)
				return false;
		}
	}

	return true;
}

bool CRoomSettings::IsMutationLimitValid(int mutationLimit)
{
	if (mutationLimit == 20 || mutationLimit == 25 || mutationLimit == 30 || mutationLimit == 35 || mutationLimit == 40)
		return true;

	return false;
}

bool CRoomSettings::CanChangeTeamBalance(int gameModeId)
{
	return (gameModeId == 0 || gameModeId == 3 || gameModeId == 23 || gameModeId == 32 || gameModeId == 57);
}

bool CRoomSettings::CanChangeFriendlyFire(int gameModeId)
{
	return (gameModeId == 0 || gameModeId == 3 || gameModeId == 8 || gameModeId == 9 || gameModeId == 14 || gameModeId == 19 || gameModeId == 30 || gameModeId == 32 || gameModeId == 37 || gameModeId == 40 || gameModeId == 45 || gameModeId == 49 || gameModeId == 51 || gameModeId == 53 || gameModeId == 54 || gameModeId == 57);
}

int CRoomSettings::GetGameModeDefaultSetting(int gameModeId, const string& setting)
{
	vector<int> validValues = split(g_pGameModeListTable->GetCell<string>(setting, to_string(gameModeId)), '|');

	return validValues.size() == 4 ? validValues[2] : 0;
}

int CRoomSettings::GetMapSetting(int mapId, const string& setting)
{
	return g_pMapListTable->GetCell<int>(setting, to_string(mapId));
}

int CRoomSettings::GetGameModeDefaultArmsRestriction(int gameModeId)
{
	return (gameModeId == 2 || gameModeId == 5 || gameModeId == 23) ? 9 : 0;
}

int CRoomSettings::GetMapDefaultArmsRestriction(int mapId)
{
	return g_pMapListTable->GetCell<int>("weapon_restrict_deault", to_string(mapId));
}

int CRoomSettings::GetDefaultBuyTime(int gameModeId)
{
	int buyTime = 90;

	switch (gameModeId)
	{
		case 32:
			buyTime = 17;
			break;
		case 0:
			buyTime = 20;
			break;
		case 45:
			buyTime = 30;
			break;
		case 50:
			buyTime = 40;
			break;
		case 1:
		case 4:
		case 19:
		case 22:
		case 27:
		case 40:
		case 57:
			buyTime = 60;
			break;
	}

	return buyTime;
}

int CRoomSettings::GetDefaultTeamBalance(int gameModeId)
{
	int teamBalance = 1;

	switch (gameModeId)
	{
		case 41:
		case 48:
		case 50:
		case 55:
		case 56:
			teamBalance = 0;
			break;
	}

	return teamBalance;
}

int CRoomSettings::GetDefaultFriendlyFire(int gameModeId)
{
	int friendlyFire = 0;

	switch (gameModeId)
	{
		case 1:
		case 4:
		case 38:
		case 39:
			friendlyFire = 1;
			break;
	}

	return friendlyFire;
}

int CRoomSettings::GetDefaultViewFlag(int gameModeId)
{
	int viewFlag = 0;

	switch (gameModeId)
	{
		case 15:
		case 17:
		case 28:
		case 33:
			viewFlag = (1<<5);
			break;
		case 0:
		case 32:
		case 37:
		case 50:
			viewFlag = ((1<<0) | (1<<7) | (1<<5));
			break;
	}

	return viewFlag;
}

int CRoomSettings::GetDefaultFriendlyBots(int gameModeId)
{
	int friendlyBots = 0;

	switch (gameModeId)
	{
		case 3:
		case 4:
		case 5:
			friendlyBots = 4;
			break;
		case 24:
		case 57:
			friendlyBots = 5;
			break;
		case 22:
			friendlyBots = 7;
			break;
		case 14:
		case 45:
		case 54:
			friendlyBots = 8;
			break;
	}

	return friendlyBots;
}

int CRoomSettings::GetDefaultEnemyBots(int gameModeId)
{
	int enemyBots = 0;

	switch (gameModeId)
	{
		case 3:
		case 4:
		case 5:
			enemyBots = 5;
			break;
		case 24:
		case 57:
			enemyBots = 6;
			break;
		case 14:
		case 22:
		case 45:
		case 54:
			enemyBots = 8;
			break;
	}

	return enemyBots;
}

int CRoomSettings::GetDefaultBotAdd(int gameModeId)
{
	int botAdd = 0;

	switch (gameModeId)
	{
		case 3:
		case 4:
		case 5:
		case 14:
		case 22:
		case 24:
		case 45:
		case 54:
		case 57:
			botAdd = 1;
			break;
	}

	return botAdd;
}

int CRoomSettings::GetDefaultStartingCash(int gameModeId)
{
	int startingCash = 0;

	switch (gameModeId)
	{
		case 0:
		case 3:
		case 28:
		case 52:
			startingCash = 800;
			break;
		case 50:
			startingCash = 5000;
			break;
		case 15:
		case 17:
			startingCash = 7500;
			break;
		case 45:
			startingCash = 8000;
			break;
	}

	return startingCash;
}

int CRoomSettings::GetDefaultZbRespawn(int gameModeId)
{
	int zbRespawn = 0;

	switch (gameModeId)
	{
		case 8:
		case 9:
		case 14:
		case 20:
		case 24:
		case 29:
		case 32:
		case 45:
		case 51:
		case 56:
			zbRespawn = 1;
			break;
	}

	return zbRespawn;
}

int CRoomSettings::GetDefaultZbBalance(int gameModeId)
{
	int zbBalance = 0;

	switch (gameModeId)
	{
		case 8:
		case 9:
		case 14:
		case 20:
		case 24:
		case 29:
		case 32:
		case 45:
		case 51:
			zbBalance = 1;
			break;
	}

	return zbBalance;
}

bool CRoomSettings::IsFunGameMode(int gameModeId)
{
	return (gameModeId == 10 || gameModeId == 12 || gameModeId == 16 || gameModeId == 18 || gameModeId == 19 || gameModeId == 21 || gameModeId == 25 || gameModeId == 27 || gameModeId == 31 || gameModeId == 34 || gameModeId == 37);
}

bool CRoomSettings::IsPlayroomGameMode(int gameModeId)
{
	return (gameModeId == 41 || gameModeId == 48 || gameModeId == 55);
}

bool CRoomSettings::IsVoxelGameMode(int gameModeId)
{
	return (gameModeId == 38 || gameModeId == 39 || gameModeId == 49 || gameModeId == 52 || gameModeId == 53);
}

string CRoomSettings::GetGameModeNameByID(int gameModeId)
{
	if (IsFunGameMode(gameModeId))
		return "map_FunMode";

	if (IsPlayroomGameMode(gameModeId))
		return "playroom";

	switch (gameModeId)
	{
		case 0: return " map_original";
		case 1: return " map_deathmode";
		case 2: return " map_teamdeathmode";
		case 3: return " map_bot_original";
		case 4: return " map_bot_deathmode";
		case 5: return " map_bot_teamdeathmod";
		case 6: return "official";
		case 7: return "official_tiebreak";
		case 8: return "map_zombi";
		case 9: return "map_zombi_expand";
		case 11: return "map_zombi_team";
		case 13: return "map_zombi_team_ann";
		case 14: return "map_zombi_3";
		case 15: return "map_zombie_survival";
		case 17: return "map_human_scenario";
		case 20: return "map_ZombieEscape";
		case 22: return "map_GDM";
		case 23: return "map_Basic";
		case 24: return "map_BZM";
		case 26: return "map_ZombieShelter";
		case 28: return "map_shelterteam";
		case 29: return "map_Zombie4";
		case 30: return "map_ZombieGiant";
		case 32: return "map_Zombie_Exterminate";
		case 33: return "map_standalone";
		case 35: return "Zombieofficial";
		case 36: return "Zombieofficial_tiebreak";
		case 40: return "allstar";
		case 42: return "season original";
		case 43: return "season zombie ex";
		case 44: return "season zombie hero";
		case 45: return "map_zombi_3z";
		case 46: return "map_zombietouchdown";
		case 47: return "season touchdown";
		case 49: return "prop_hunt";
		case 50: return "map_partner";
		case 51: return "map_zhe";
		case 52: return "vxlzshelter";
		case 53: return "scenariotx";
		case 54: return "zombi_5";
		case 56: return "zb_teamcontrol";
		case 57: return "map_tdm_supersoldier";
		default: return "";
	}
}

bool CRoomSettings::IsMapValid(int gameModeId, int mapId)
{
	string gameModeName = GetGameModeNameByID(gameModeId);

	if (gameModeName.empty())
		return false;

	int id = g_pMapListTable->GetCell<int>(gameModeName, to_string(mapId));

	if (!id)
		return false;

	if (gameModeName == "map_FunMode" && g_pMapListTable->GetCell<int>("Fun_Subtype(축구 16, 챌린지 12, 아이템전 19, 바주카전 21, 메탈아레나 18, 파이트야드 31, 비스트 27, 서든데스 34, 술래잡기 37)", to_string(mapId)) != gameModeId)
		return false;

	if (gameModeName == "playroom" && g_pMapListTable->GetCell<int>("playroom_modeID", to_string(mapId)) != gameModeId)
		return false;

	return true;
}

bool CRoomSettings::IsMapPlaylistAllowed(int gameModeId)
{
	return !(IsFunGameMode(gameModeId) || IsPlayroomGameMode(gameModeId) || IsVoxelGameMode(gameModeId) || gameModeId == 26 || gameModeId == 28 || gameModeId == 33);
}

bool CRoomSettings::IsRandomMapAllowed(int gameModeId)
{
	return !(IsPlayroomGameMode(gameModeId) || IsVoxelGameMode(gameModeId) || gameModeId == 15 || gameModeId == 17 || gameModeId == 30 || gameModeId == 33 || gameModeId == 50 || gameModeId == 51);
}

void CRoomSettings::LoadDefaultSettings(int gameModeId, int mapId)
{
	lowFlag = ROOM_LOW_ALL;
	lowMidFlag = ROOM_LOWMID_ALL & ~ROOM_LOWMID_UNK62;
	highMidFlag = ROOM_HIGHMID_ALL;
	highFlag = ROOM_HIGH_ALL;

	unk00 = 0;
	unk01 = 0;
	unk02 = 0;
	unk03 = 0;
	unk04 = 0;
	levelLimit = 0;
	unk07 = 0;
	gameTime = GetGameModeDefaultSetting(gameModeId, "mode_time_limit_id");
	roundTime = GetGameModeDefaultSetting(gameModeId, "mode_roundtime_id");
	armsRestriction = GetMapDefaultArmsRestriction(mapId);
	if (!armsRestriction) armsRestriction = GetGameModeDefaultArmsRestriction(gameModeId);
	hostageKillLimit = 0;
	freezeTime = 0;
	buyTime = GetDefaultBuyTime(gameModeId);
	displayNickname = 1;
	teamBalance = GetDefaultTeamBalance(gameModeId);
	unk21 = 0;
	friendlyFire = GetDefaultFriendlyFire(gameModeId);
	flashlight = 1;
	footsteps = 1;
	unk25 = 0;
	tkPunish = 0;
	autoKick = 0;
	unk28 = 0;
	unk29 = 0;
	viewFlag = GetDefaultViewFlag(gameModeId);
	voiceChat = 1;
	status = 0;
	unk33 = 0;
	unk33_vec.clear();
	unk34 = 0;
	unk35 = "";
	unk36 = 0;
	unk37 = 0;
	unk38 = 0;
	c4Timer = 0;
	botDifficulty = 0;
	friendlyBots = GetDefaultFriendlyBots(gameModeId);
	enemyBots = GetDefaultEnemyBots(gameModeId);
	botBalance = GetDefaultBotAdd(gameModeId);
	botAdd = GetDefaultBotAdd(gameModeId);
	kdRule = 0;
	startingCash = GetDefaultStartingCash(gameModeId);
	movingShot = 0;
	ballNumber = GetMapSetting(mapId, "ball_default");
	statusSymbol = 0;
	mapPlaylistIndex = mapPlaylistSize ? 1 : 0;
	enhanceRestrict = (gameModeId == 22 || gameModeId == 32) ? 1 : 0;
	if (gameModeId != 15)
	{
		zsDifficulty = 0;
		unk56 = 0;
		unk57 = 0;
	}
	leagueRule = 0;
	mannerLimit = 0;
	mapId2 = mapId;
	zbLimitFlag = 0;
	zbLimit.clear();
	unk63 = 0;
	unk63_vec.clear();
	unk64 = 0;
	teamSwitch = 0;
	zbRespawn = GetDefaultZbRespawn(gameModeId);
	zbBalance = GetDefaultZbBalance(gameModeId);
	gameRule = gameModeId == 40 ? 1 : 0;
	isZbCompetitive = mapId == 282 ? 1 : 0;
	zbAutoHunting = 0;
	integratedTeam = 0;
	unk73 = 0;
	fireBomb = 1;
	mutationRestrictSize = 0;
	mutationRestrict.clear();
	mutationLimit = gameModeId == 45 ? 40 : 0;
	unk77 = 0;

	if (!winLimit)
		winLimit = GetGameModeDefaultSetting(gameModeId, "mode_win_limit_id");

	if (!killLimit)
		killLimit = GetGameModeDefaultSetting(gameModeId, "mode_kill_limit_id");

	if (isZbCompetitive)
	{
		maxPlayers = 32;
		winLimit = gameModeId == 14 ? 7 : 13;
		roundTime = 3;
	}
}

void CRoomSettings::LoadZbCompetitiveSettings(int gameModeId)
{
	lowFlag |= ROOM_LOW_WINLIMIT;
	winLimit = gameModeId == 14 ? 7 : 13;

	lowFlag |= ROOM_LOW_GAMETIME;
	gameTime = 150;

	lowFlag |= ROOM_LOW_ROUNDTIME;
	roundTime = 3;

	lowFlag |= ROOM_LOW_HOSTAGEKILLLIMIT;
	hostageKillLimit = 0;

	lowFlag |= ROOM_LOW_FREEZETIME;
	freezeTime = 0;

	lowFlag |= ROOM_LOW_BUYTIME;
	buyTime = 90;

	lowFlag |= ROOM_LOW_DISPLAYNICKNAME;
	displayNickname = 1;

	lowFlag |= ROOM_LOW_TEAMBALANCE;
	teamBalance = 1;

	lowFlag |= ROOM_LOW_FRIENDLYFIRE;
	friendlyFire = 0;

	lowFlag |= ROOM_LOW_FLASHLIGHT;
	flashlight = 1;

	lowFlag |= ROOM_LOW_VIEWFLAG;
	viewFlag = 0;

	lowFlag |= ROOM_LOW_VOICECHAT;
	voiceChat = 1;

	lowMidFlag |= ROOM_LOWMID_BOT;
	botDifficulty = 0;
	friendlyBots = GetDefaultFriendlyBots(gameModeId);
	enemyBots = GetDefaultEnemyBots(gameModeId);
	botBalance = GetDefaultBotAdd(gameModeId);
	botAdd = GetDefaultBotAdd(gameModeId);

	lowMidFlag |= ROOM_LOWMID_KDRULE;
	kdRule = 0;

	lowMidFlag |= ROOM_LOWMID_STARTINGCASH;
	startingCash = GetDefaultStartingCash(gameModeId);

	lowMidFlag |= ROOM_LOWMID_ENHANCERESTRICT;
	enhanceRestrict = 0;

	lowMidFlag |= ROOM_LOWMID_MANNERLIMIT;
	mannerLimit = 0;

	lowMidFlag |= ROOM_LOWMID_ZBLIMIT;
	zbLimitFlag = 0;
	zbLimit.clear();

	lowMidFlag |= ROOM_LOWMID_ZBRESPAWN;
	zbRespawn = 1;

	lowMidFlag |= ROOM_LOWMID_ZBBALANCE;
	zbBalance = 1;

	highMidFlag |= ROOM_HIGHMID_FIREBOMB;
	fireBomb = 1;

	highMidFlag |= ROOM_HIGHMID_MUTATIONRESTRICT;
	mutationRestrictSize = 0;
	mutationRestrict.clear();

	highMidFlag |= ROOM_HIGHMID_MUTATIONLIMIT;
	mutationLimit = gameModeId == 45 ? 40 : 0;
}

void CRoomSettings::LoadNewSettings(int gameModeId, int mapId, IUser* user, int changeFlag)
{
	if (lowFlag & ROOM_LOW_UNK)
		lowFlag &= ~ROOM_LOW_UNK;

	if (lowFlag & ROOM_LOW_CLANBATTLE)
		lowFlag &= ~ROOM_LOW_CLANBATTLE;

	if (lowFlag & ROOM_LOW_LEVELLIMIT)
		lowFlag &= ~ROOM_LOW_LEVELLIMIT;

	if (lowFlag & ROOM_LOW_UNK7)
		lowFlag &= ~ROOM_LOW_UNK7;

	if (lowFlag & ROOM_LOW_HOSTAGEKILLLIMIT)
	{
		if (hostageKillLimit > 10)
			hostageKillLimit = 10;
	}

	if (lowFlag & ROOM_LOW_FREEZETIME)
	{
		if (freezeTime > 10)
			freezeTime = 10;
	}

	if (lowFlag & ROOM_LOW_DISPLAYNICKNAME)
	{
		if (displayNickname > 2)
			displayNickname = 2;
	}

	if (lowFlag & ROOM_LOW_UNK21)
		lowFlag &= ~ROOM_LOW_UNK21;

	if (lowFlag & ROOM_LOW_FLASHLIGHT)
		flashlight = flashlight ? 1 : 0;

	if (lowFlag & ROOM_LOW_FOOTSTEPS)
		lowFlag &= ~ROOM_LOW_FOOTSTEPS;

	if (lowFlag & ROOM_LOW_UNK25)
		lowFlag &= ~ROOM_LOW_UNK25;

	if (lowFlag & ROOM_LOW_TKPUNISH)
		lowFlag &= ~ROOM_LOW_TKPUNISH;

	if (lowFlag & ROOM_LOW_AUTOKICK)
		lowFlag &= ~ROOM_LOW_AUTOKICK;

	if (lowFlag & ROOM_LOW_UNK28)
		lowFlag &= ~ROOM_LOW_UNK28;

	if (lowFlag & ROOM_LOW_UNK29)
		lowFlag &= ~ROOM_LOW_UNK29;

	if (lowFlag & ROOM_LOW_VOICECHAT)
		voiceChat = voiceChat ? 1 : 0;

	if (lowFlag & ROOM_LOW_STATUS)
		lowFlag &= ~ROOM_LOW_STATUS;

	if (lowFlag & ROOM_LOW_UNK33)
		lowFlag &= ~ROOM_LOW_UNK33;

	if (lowMidFlag & ROOM_LOWMID_UNK34)
		lowMidFlag &= ~ROOM_LOWMID_UNK34;

	if (lowMidFlag & ROOM_LOWMID_KDRULE)
		kdRule = kdRule ? 1 : 0;

	if (lowMidFlag & ROOM_LOWMID_MOVINGSHOT)
		lowMidFlag &= ~ROOM_LOWMID_MOVINGSHOT;

	if (lowMidFlag & ROOM_LOWMID_STATUSSYMBOL)
		lowMidFlag &= ~ROOM_LOWMID_STATUSSYMBOL;

	if (lowMidFlag & ROOM_LOWMID_MAPPLAYLISTINDEX)
		lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLISTINDEX;

	if (lowMidFlag & ROOM_LOWMID_SD)
		lowMidFlag &= ~ROOM_LOWMID_SD;

	if (lowMidFlag & ROOM_LOWMID_MANNERLIMIT)
	{
		if (mannerLimit > 2)
			mannerLimit = 2;
	}

	if (lowMidFlag & ROOM_LOWMID_MAPID2)
		lowMidFlag &= ~ROOM_LOWMID_MAPID2;

	if (lowMidFlag & ROOM_LOWMID_UNK62)
		lowMidFlag &= ~ROOM_LOWMID_UNK62;

	if (lowMidFlag & ROOM_LOWMID_UNK63)
		lowMidFlag &= ~ROOM_LOWMID_UNK63;

	if (lowMidFlag & ROOM_LOWMID_UNK64)
		lowMidFlag &= ~ROOM_LOWMID_UNK64;

	if (lowMidFlag & ROOM_LOWMID_SUPERROOM)
		lowMidFlag &= ~ROOM_LOWMID_SUPERROOM;

	if (lowMidFlag & ROOM_LOWMID_ISZBCOMPETITIVE)
		lowMidFlag &= ~ROOM_LOWMID_ISZBCOMPETITIVE;

	if (lowMidFlag & ROOM_LOWMID_UNK73)
		lowMidFlag &= ~ROOM_LOWMID_UNK73;

	if (highFlag & ROOM_HIGH_UNK77)
		highFlag &= ~ROOM_HIGH_UNK77;

	if (changeFlag & ROOM_LOW_GAMEMODEID)
	{
		lowFlag |= ROOM_LOW_MAXPLAYERS;
		maxPlayers = g_pGameModeListTable->GetCell<int>("mode_maxplayer", to_string(gameModeId));

		lowFlag |= ROOM_LOW_WINLIMIT;
		winLimit = GetGameModeDefaultSetting(gameModeId, "mode_win_limit_id");

		lowFlag |= ROOM_LOW_KILLLIMIT;
		killLimit = GetGameModeDefaultSetting(gameModeId, "mode_kill_limit_id");

		lowFlag |= ROOM_LOW_GAMETIME;
		gameTime = GetGameModeDefaultSetting(gameModeId, "mode_time_limit_id");

		lowFlag |= ROOM_LOW_ROUNDTIME;
		roundTime = GetGameModeDefaultSetting(gameModeId, "mode_roundtime_id");

		lowFlag |= ROOM_LOW_ARMSRESTRICTION;
		armsRestriction = GetGameModeDefaultArmsRestriction(gameModeId);

		lowFlag |= ROOM_LOW_HOSTAGEKILLLIMIT;
		hostageKillLimit = 0;

		lowFlag |= ROOM_LOW_FREEZETIME;
		freezeTime = 0;

		lowFlag |= ROOM_LOW_BUYTIME;
		buyTime = GetDefaultBuyTime(gameModeId);

		lowFlag |= ROOM_LOW_DISPLAYNICKNAME;
		displayNickname = 1;

		lowFlag |= ROOM_LOW_TEAMBALANCE;
		teamBalance = GetDefaultTeamBalance(gameModeId);

		lowFlag |= ROOM_LOW_FRIENDLYFIRE;
		friendlyFire = GetDefaultFriendlyFire(gameModeId);

		lowFlag |= ROOM_LOW_FLASHLIGHT;
		flashlight = 1;

		lowFlag |= ROOM_LOW_VIEWFLAG;
		viewFlag = GetDefaultViewFlag(gameModeId);

		lowFlag |= ROOM_LOW_VOICECHAT;
		voiceChat = 1;

		lowMidFlag |= ROOM_LOWMID_C4TIMER;
		c4Timer = 0;

		lowMidFlag |= ROOM_LOWMID_BOT;
		botDifficulty = 0;
		friendlyBots = GetDefaultFriendlyBots(gameModeId);
		enemyBots = GetDefaultEnemyBots(gameModeId);
		botBalance = GetDefaultBotAdd(gameModeId);
		botAdd = GetDefaultBotAdd(gameModeId);

		lowMidFlag |= ROOM_LOWMID_KDRULE;
		kdRule = 0;

		lowMidFlag |= ROOM_LOWMID_STARTINGCASH;
		startingCash = GetDefaultStartingCash(gameModeId);

		lowMidFlag |= ROOM_LOWMID_ENHANCERESTRICT;
		enhanceRestrict = (gameModeId == 22 || gameModeId == 32) ? 1 : 0;

		lowMidFlag |= ROOM_LOWMID_SD;
		if (gameModeId == 3 || gameModeId == 4 || gameModeId == 5 || gameModeId == 15 || gameModeId == 24)
		{
			CUserInventoryItem item;
			sd = g_pUserDatabase->GetFirstActiveItemByItemID(user->GetID(), 439 /* BigHeadEvent */, item);
		}
		else
			sd = 0;

		lowMidFlag |= ROOM_LOWMID_ZSDIFFICULTY;
		zsDifficulty = 0;
		unk56 = 0;
		unk57 = 0;

		lowMidFlag |= ROOM_LOWMID_LEAGUERULE;
		leagueRule = 0;

		lowMidFlag |= ROOM_LOWMID_MANNERLIMIT;
		mannerLimit = 0;

		lowMidFlag |= ROOM_LOWMID_ZBLIMIT;
		zbLimitFlag = 0;
		zbLimit.clear();

		lowMidFlag |= ROOM_LOWMID_TEAMSWITCH;
		teamSwitch = 0;

		lowMidFlag |= ROOM_LOWMID_ZBRESPAWN;
		zbRespawn = GetDefaultZbRespawn(gameModeId);

		lowMidFlag |= ROOM_LOWMID_ZBBALANCE;
		zbBalance = GetDefaultZbBalance(gameModeId);

		lowMidFlag |= ROOM_LOWMID_GAMERULE;
		gameRule = gameModeId == 40 ? 1 : 0;

		lowMidFlag |= ROOM_LOWMID_ZBAUTOHUNTING;
		zbAutoHunting = 0;

		lowMidFlag |= ROOM_LOWMID_INTEGRATEDTEAM;
		integratedTeam = 0;

		highMidFlag |= ROOM_HIGHMID_FIREBOMB;
		fireBomb = 1;

		highMidFlag |= ROOM_HIGHMID_MUTATIONRESTRICT;
		mutationRestrictSize = 0;
		mutationRestrict.clear();

		highMidFlag |= ROOM_HIGHMID_MUTATIONLIMIT;
		mutationLimit = gameModeId == 45 ? 40 : 0;
	}
	else
	{
		if (lowFlag & ROOM_LOW_MAXPLAYERS)
		{
			int gameModeMinPlayers = g_pGameModeListTable->GetCell<int>("mode_minplayer", to_string(gameModeId));
			if (maxPlayers < gameModeMinPlayers)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with maxPlayers < gameModeMinPlayers: %d, maxPlayers: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeMinPlayers, maxPlayers);
				lowFlag &= ~ROOM_LOW_MAXPLAYERS;
			}
			else
			{
				int gameModeMaxPlayers = g_pGameModeListTable->GetCell<int>("mode_maxplayer", to_string(gameModeId));
				if (maxPlayers > gameModeMaxPlayers)
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with maxPlayers > gameModeMaxPlayers: %d, maxPlayers: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeMaxPlayers, maxPlayers);
					lowFlag &= ~ROOM_LOW_MAXPLAYERS;
				}
			}
		}

		if (lowFlag & ROOM_LOW_WINLIMIT)
		{
			if (gameModeId != 0 && gameModeId != 3 && !IsSettingValid(gameModeId, "mode_win_limit_id", winLimit))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid winLimit: %d\n", user->GetID(), user->GetUsername().c_str(), winLimit);
				lowFlag &= ~ROOM_LOW_WINLIMIT;
			}
		}

		if (lowFlag & ROOM_LOW_KILLLIMIT)
		{
			if (!IsSettingValid(gameModeId, "mode_kill_limit_id", killLimit))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid killLimit: %d\n", user->GetID(), user->GetUsername().c_str(), killLimit);
				lowFlag &= ~ROOM_LOW_KILLLIMIT;
			}
		}

		if (lowFlag & ROOM_LOW_GAMETIME)
		{
			if (!IsSettingValid(gameModeId, "mode_time_limit_id", gameTime))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid gameTime: %d\n", user->GetID(), user->GetUsername().c_str(), gameTime);
				lowFlag &= ~ROOM_LOW_GAMETIME;
			}
		}

		if (lowFlag & ROOM_LOW_ROUNDTIME)
		{
			if (!IsSettingValid(gameModeId, "mode_roundtime_id", roundTime))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid roundTime: %d\n", user->GetID(), user->GetUsername().c_str(), roundTime);
				lowFlag &= ~ROOM_LOW_ROUNDTIME;
			}
		}

		if (lowFlag & ROOM_LOW_ARMSRESTRICTION)
		{
			if (armsRestriction > 17)
				armsRestriction = 17;
		}

		if (lowFlag & ROOM_LOW_BUYTIME)
		{
			if (!IsBuyTimeValid(gameModeId, buyTime))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid buyTime: %d\n", user->GetID(), user->GetUsername().c_str(), buyTime);
				lowFlag &= ~ROOM_LOW_BUYTIME;
			}
		}

		if (lowFlag & ROOM_LOW_TEAMBALANCE)
		{
			if (!CanChangeTeamBalance(gameModeId) && teamBalance != GetDefaultTeamBalance(gameModeId))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't allow teamBalance to be changed, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowFlag &= ~ROOM_LOW_TEAMBALANCE;
			}
			else if (teamBalance > 1)
				teamBalance = 1;
		}

		if (lowFlag & ROOM_LOW_FRIENDLYFIRE)
		{
			if (!CanChangeFriendlyFire(gameModeId) && friendlyFire != GetDefaultFriendlyFire(gameModeId))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't allow friendlyFire to be changed, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowFlag &= ~ROOM_LOW_FRIENDLYFIRE;
			}
			else if (friendlyFire > 1)
				friendlyFire = 1;
		}

		if (lowFlag & ROOM_LOW_VIEWFLAG)
		{
			if (viewFlag & (1 << 1))
				viewFlag &= ~(1 << 1);

			if (viewFlag & (1 << 2))
				viewFlag &= ~(1 << 2);

			if (viewFlag & (1 << 3))
				viewFlag &= ~(1 << 3);

			if (viewFlag & (1 << 4))
				viewFlag &= ~(1 << 4);
		}

		if (lowMidFlag & ROOM_LOWMID_C4TIMER)
		{
			if (c4Timer)
			{
				if (gameModeId == 0 || gameModeId == 3 || gameModeId == 23)
				{
					CUserInventoryItem item;
					c4Timer = g_pUserDatabase->GetFirstActiveItemByItemID(user->GetID(), 112 /* C4Sound */, item);
				}
				else
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use c4Timer, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
					lowMidFlag &= ~ROOM_LOWMID_C4TIMER;
				}
			}
		}

		if (lowMidFlag & ROOM_LOWMID_BOT)
		{
			if (!GetDefaultBotAdd(gameModeId))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't allow bots, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_BOT;
			}
			else if (gameModeId == 3 || gameModeId == 4 || gameModeId == 5 || gameModeId == 24)
			{
				if (botDifficulty > 7)
					botDifficulty = 7;

				if (friendlyBots > 15)
					friendlyBots = 15;

				if (enemyBots < 1)
					enemyBots = 1;

				if (enemyBots > 16)
					enemyBots = 16;
			}
			else
			{
				if (friendlyBots != GetDefaultFriendlyBots(gameModeId))
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use friendlyBots, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
					lowMidFlag &= ~ROOM_LOWMID_BOT;
				}
				else if (enemyBots != GetDefaultEnemyBots(gameModeId))
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use enemyBots, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
					lowMidFlag &= ~ROOM_LOWMID_BOT;
				}
				else if (botBalance != GetDefaultBotAdd(gameModeId))
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use botBalance, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
					lowMidFlag &= ~ROOM_LOWMID_BOT;
				}
				else if (botDifficulty > 2)
					botDifficulty = 2;
			}
		}

		if (lowMidFlag & ROOM_LOWMID_STARTINGCASH)
		{
			if (!(gameModeId == 0 || gameModeId == 3 || gameModeId == 17 || gameModeId == 50))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use startingCash, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_STARTINGCASH;
			}
			else if (!IsStartingCashValid(gameModeId, startingCash))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid startingCash: %d\n", user->GetID(), user->GetUsername().c_str(), startingCash);
				lowMidFlag &= ~ROOM_LOWMID_STARTINGCASH;
			}
		}

		if (lowMidFlag & ROOM_LOWMID_ENHANCERESTRICT)
		{
			if (!(gameModeId == 0 || gameModeId == 22 || gameModeId == 32) && enhanceRestrict != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use enhanceRestrict, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ENHANCERESTRICT;
			}
			else if ((gameModeId == 22 || gameModeId == 32) && enhanceRestrict != 1)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't allow enhanceRestrict to be changed, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ENHANCERESTRICT;
			}
			else if (enhanceRestrict > 1)
				enhanceRestrict = 1;
		}

		if (lowMidFlag & ROOM_LOWMID_SD)
			lowMidFlag &= ~ROOM_LOWMID_SD;

		if (lowMidFlag & ROOM_LOWMID_ZSDIFFICULTY)
		{
			if (gameModeId != 15 && zsDifficulty != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use zsDifficulty, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ZSDIFFICULTY;
			}
			else
			{
				int mapMaxZsDifficulty = g_pMapListTable->GetCell<int>("ZSmaxDifficulty", to_string(mapId));
				if (zsDifficulty > mapMaxZsDifficulty)
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with zsDifficulty > mapMaxZsDifficulty: %d, zsDifficulty: %d, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapMaxZsDifficulty, zsDifficulty, mapId);
					lowMidFlag &= ~ROOM_LOWMID_ZSDIFFICULTY;
				}
			}
		}

		if (lowMidFlag & ROOM_LOWMID_LEAGUERULE)
		{
			if (!(gameModeId == 0 || gameModeId == 32) && leagueRule != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use leagueRule, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_LEAGUERULE;
			}
			else if (leagueRule > 1)
				leagueRule = 1;
		}

		if (lowMidFlag & ROOM_LOWMID_ZBLIMIT)
		{
			if (!(gameModeId == 9 || gameModeId == 14 || gameModeId == 45) && (zbLimitFlag != 0 || !zbLimit.empty()))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use zbLimit, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ZBLIMIT;
			}
			else if (!IsZbLimitValid(zbLimit))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid zbLimit\n", user->GetID(), user->GetUsername().c_str());
				lowMidFlag &= ~ROOM_LOWMID_ZBLIMIT;
			}
			else if (zbLimitFlag == 2 || zbLimitFlag > 3)
				zbLimitFlag = 3;
		}

		if (lowMidFlag & ROOM_LOWMID_TEAMSWITCH)
		{
			if (!(gameModeId == 0 || gameModeId == 3) && teamSwitch != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use teamSwitch, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_TEAMSWITCH;
			}
			else if (teamSwitch > 1)
				teamSwitch = 1;
		}

		if (lowMidFlag & ROOM_LOWMID_ZBRESPAWN)
		{
			if (!GetDefaultZbRespawn(gameModeId) && zbRespawn != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use zbRespawn, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ZBRESPAWN;
			}
			else if (gameModeId == 56 && zbRespawn != 1)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't allow zbRespawn to be changed, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ZBRESPAWN;
			}
			else if (zbRespawn > 1)
				zbRespawn = 1;
		}

		if (lowMidFlag & ROOM_LOWMID_ZBBALANCE)
		{
			if (!GetDefaultZbBalance(gameModeId) && gameModeId != 56 && zbBalance != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use zbBalance, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ZBBALANCE;
			}
			else if (zbBalance > 1)
				zbBalance = 1;
		}

		if (lowMidFlag & ROOM_LOWMID_GAMERULE)
		{
			if (gameModeId != 40 && gameRule != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use gameRule, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_GAMERULE;
			}
			else if (gameRule > 1)
				gameRule = 1;
		}

		if (lowMidFlag & ROOM_LOWMID_ZBAUTOHUNTING)
		{
			if (!(gameModeId == 14 || gameModeId == 45) && zbAutoHunting != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use zbAutoHunting, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_ZBAUTOHUNTING;
			}
			else if (zbAutoHunting > 1)
				zbAutoHunting = 1;
		}

		if (lowMidFlag & ROOM_LOWMID_INTEGRATEDTEAM)
		{
			if (!(gameModeId == 2 || gameModeId == 5 || gameModeId == 56 || gameModeId == 57) && integratedTeam != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use integratedTeam, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				lowMidFlag &= ~ROOM_LOWMID_INTEGRATEDTEAM;
			}
			else if (integratedTeam > 1)
				integratedTeam = 1;
		}

		if (highMidFlag & ROOM_HIGHMID_FIREBOMB)
		{
			if (!(gameModeId == 14 || gameModeId == 45) && fireBomb != 1)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use fireBomb, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				highMidFlag &= ~ROOM_HIGHMID_FIREBOMB;
			}
			else if (fireBomb > 1)
				fireBomb = 1;
		}

		if (highMidFlag & ROOM_HIGHMID_MUTATIONRESTRICT)
		{
			if (gameModeId != 45 && (mutationRestrictSize != 0 || !mutationRestrict.empty()))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use mutationRestrict, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				highMidFlag &= ~ROOM_HIGHMID_MUTATIONRESTRICT;
			}
			else if (mutationRestrictSize != 4)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with mutationRestrictSize != 4, mutationRestrictSize: %d\n", user->GetID(), user->GetUsername().c_str(), mutationRestrictSize);
				highMidFlag &= ~ROOM_HIGHMID_MUTATIONRESTRICT;
			}
			else if (!IsMutationRestrictValid(mutationRestrict))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid mutationRestrict\n", user->GetID(), user->GetUsername().c_str());
				highMidFlag &= ~ROOM_HIGHMID_MUTATIONRESTRICT;
			}
		}

		if (highMidFlag & ROOM_HIGHMID_MUTATIONLIMIT)
		{
			if (gameModeId != 45 && mutationLimit != 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use mutationLimit, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				highMidFlag &= ~ROOM_HIGHMID_MUTATIONLIMIT;
			}
			else if (!IsMutationLimitValid(mutationLimit))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid mutationLimit: %d\n", user->GetID(), user->GetUsername().c_str(), mutationLimit);
				highMidFlag &= ~ROOM_HIGHMID_MUTATIONLIMIT;
			}
		}
	}

	if (changeFlag & ROOM_LOW_MAPID)
	{
		lowFlag |= ROOM_LOW_MAXPLAYERS;
		maxPlayers = g_pMapListTable->GetCell<int>("max_player", to_string(mapId));

		lowFlag |= ROOM_LOW_ARMSRESTRICTION;
		int restriction = GetMapDefaultArmsRestriction(mapId);
		if (!armsRestriction) armsRestriction = restriction;

		lowMidFlag |= ROOM_LOWMID_BALLNUMBER;
		ballNumber = GetMapSetting(mapId, "ball_default");

		lowMidFlag |= ROOM_LOWMID_ISZBCOMPETITIVE;
		isZbCompetitive = mapId == 282 ? 1 : 0;

		if (isZbCompetitive)
			LoadZbCompetitiveSettings(gameModeId);
	}
	else
	{
		if (lowFlag & ROOM_LOW_MAXPLAYERS)
		{
			int mapMaxPlayers = g_pMapListTable->GetCell<int>("max_player", to_string(mapId));
			if (maxPlayers > mapMaxPlayers)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with maxPlayers > mapMaxPlayers: %d, maxPlayers: %d\n", user->GetID(), user->GetUsername().c_str(), mapMaxPlayers, maxPlayers);
				lowFlag &= ~ROOM_LOW_MAXPLAYERS;
			}
		}

		if (lowFlag & ROOM_LOW_ARMSRESTRICTION)
		{
			int mapArmsRestriction = GetMapDefaultArmsRestriction(mapId);
			if (mapArmsRestriction && armsRestriction != mapArmsRestriction)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with armsRestriction != mapArmsRestriction: %d, armsRestriction: %d\n", user->GetID(), user->GetUsername().c_str(), mapArmsRestriction, armsRestriction);
				lowFlag &= ~ROOM_LOW_ARMSRESTRICTION;
			}
		}

		if (lowMidFlag & ROOM_LOWMID_BALLNUMBER)
		{
			if (gameModeId != 16 && ballNumber != 0)
			{
				if (ballNumber != 1) // client sends this on some modes, weird
					Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId that doesn't use ballNumber, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);

				lowMidFlag &= ~ROOM_LOWMID_BALLNUMBER;
			}
			else
			{
				int mapMaxBallNumber = GetMapSetting(mapId, "ball_max");
				if (ballNumber > mapMaxBallNumber)
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with ballNumber > mapMaxBallNumber: %d, ballNumber: %d\n", user->GetID(), user->GetUsername().c_str(), mapMaxBallNumber, ballNumber);
					lowMidFlag &= ~ROOM_LOWMID_BALLNUMBER;
				}
			}
		}
	}

	if (lowMidFlag & ROOM_LOWMID_RANDOMMAP)
	{
		if (!IsRandomMapAllowed(gameModeId) && randomMap != 0)
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId which doesn't allow randomMap, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
			lowMidFlag &= ~ROOM_LOWMID_RANDOMMAP;
		}
		else if (randomMap > 2)
			randomMap = 2;
	}

	if (lowMidFlag & ROOM_LOWMID_MAPPLAYLIST)
	{
		if (mapPlaylistSize < 2)
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with mapPlaylistSize < 2, mapPlaylistSize: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylistSize);
			lowFlag &= ~ROOM_LOW_MAPID;
			lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
		}
		else if (!IsMapPlaylistAllowed(gameModeId))
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with gameModeId which doesn't allow mapPlaylist, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
			lowFlag &= ~ROOM_LOW_MAPID;
			lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
		}
		else if (mapPlaylistSize > 5)
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with mapPlaylistSize > 5, mapPlaylistSize: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylistSize);
			lowFlag &= ~ROOM_LOW_MAPID;
			lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
		}
		else if (!IsMapPlaylistValid(mapPlaylist))
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with duplicate mapIds in mapPlaylist\n", user->GetID(), user->GetUsername().c_str());
			lowFlag &= ~ROOM_LOW_MAPID;
			lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
		}
		else if (mapId != mapPlaylist[0].mapId)
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with mapId which isn't the first in mapPlaylist, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId);
			lowFlag &= ~ROOM_LOW_MAPID;
			lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
		}
		else
		{
			for (int i = 0; i < mapPlaylistSize; i++)
			{
				if (g_pMapListTable->GetRowIdx(to_string(mapPlaylist[i].mapId)) < 0)
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with an invalid mapId in mapPlaylist, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylist[i].mapId);
					lowFlag &= ~ROOM_LOW_MAPID;
					lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
					break;
				}

				if (!IsMapValid(gameModeId, mapPlaylist[i].mapId))
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with an invalid mapId in mapPlaylist, mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylist[i].mapId, gameModeId);
					lowFlag &= ~ROOM_LOW_MAPID;
					lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
					break;
				}

				if (GetMapDefaultArmsRestriction(mapPlaylist[i].mapId))
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with mapId which isn't allowed in mapPlaylist, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylist[i].mapId);
					lowFlag &= ~ROOM_LOW_MAPID;
					lowMidFlag &= ~ROOM_LOWMID_MAPPLAYLIST;
					break;
				}
			}

			lowMidFlag |= ROOM_LOWMID_MAPPLAYLISTINDEX;
			mapPlaylistIndex = 1;
		}
	}

	if (lowFlag & ROOM_LOW_MAPID)
	{
		lowMidFlag |= ROOM_LOWMID_MAPPLAYLIST;

		if (changeFlag & ROOM_LOW_MAPID)
		{
			lowMidFlag |= ROOM_LOWMID_MAPID2;
			mapId2 = mapId;
		}
	}
}

bool CRoomSettings::CheckSettings(IUser* user)
{
	if (g_pServerConfig->room.validateSettings)
	{
		if (!(lowFlag & ROOM_LOW_ROOMNAME && lowFlag & ROOM_LOW_PASSWORD && lowFlag & ROOM_LOW_GAMEMODEID && lowFlag & ROOM_LOW_MAPID && lowFlag & ROOM_LOW_MAXPLAYERS && lowFlag & ROOM_LOW_WINLIMIT && lowFlag & ROOM_LOW_KILLLIMIT))
		{
			Console().Warn("User '%d, %s' tried to create a new room without necessary settings\n", user->GetID(), user->GetUsername().c_str());
			return false;
		}

		if (g_pGameModeListTable->GetRowIdx(to_string(gameModeId)) < 0)
		{
			Console().Warn("User '%d, %s' tried to create a new room with invalid gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
			return false;
		}

		if (gameModeId != 39 && !IsFunGameMode(gameModeId) && !g_pGameModeListTable->GetCell<int>("mode_select_ui_order", to_string(gameModeId)))
		{
			Console().Warn("User '%d, %s' tried to create a new room with invalid gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
			return false;
		}

		if (g_pMapListTable->GetRowIdx(to_string(mapId)) < 0)
		{
			Console().Warn("User '%d, %s' tried to create a new room with invalid mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId);
			return false;
		}

		if (!IsMapValid(gameModeId, mapId))
		{
			Console().Warn("User '%d, %s' tried to create a new room with invalid mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId, gameModeId);
			return false;
		}

		int mapMaxPlayers = g_pMapListTable->GetCell<int>("max_player", to_string(mapId));
		if (maxPlayers > mapMaxPlayers)
		{
			Console().Warn("User '%d, %s' tried to create a new room with maxPlayers > mapMaxPlayers: %d, maxPlayers: %d, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapMaxPlayers, maxPlayers, mapId);
			return false;
		}

		int gameModeMinPlayers = g_pGameModeListTable->GetCell<int>("mode_minplayer", to_string(gameModeId));
		if (maxPlayers < gameModeMinPlayers)
		{
			Console().Warn("User '%d, %s' tried to create a new room with maxPlayers < gameModeMinPlayers: %d, maxPlayers: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeMinPlayers, maxPlayers, gameModeId);
			return false;
		}

		int gameModeMaxPlayers = g_pGameModeListTable->GetCell<int>("mode_maxplayer", to_string(gameModeId));
		if (maxPlayers > gameModeMaxPlayers)
		{
			Console().Warn("User '%d, %s' tried to create a new room with maxPlayers > gameModeMaxPlayers: %d, maxPlayers: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeMaxPlayers, maxPlayers, gameModeId);
			return false;
		}

		if (!winLimit)
			winLimit = GetGameModeDefaultSetting(gameModeId, "mode_win_limit_id");

		if (!IsSettingValid(gameModeId, "mode_win_limit_id", winLimit))
		{
			Console().Warn("User '%d, %s' tried to create a new room with invalid winLimit: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), winLimit, gameModeId);
			return false;
		}

		if (!killLimit)
			killLimit = GetGameModeDefaultSetting(gameModeId, "mode_kill_limit_id");

		if (!IsSettingValid(gameModeId, "mode_kill_limit_id", killLimit))
		{
			Console().Warn("User '%d, %s' tried to create a new room with invalid killLimit: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), killLimit, gameModeId);
			return false;
		}

		if (lowMidFlag & ROOM_LOWMID_RANDOMMAP)
		{
			if (!IsRandomMapAllowed(gameModeId) && randomMap != 0)
			{
				Console().Warn("User '%d, %s' tried to create a new room with gameModeId which doesn't allow randomMap, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				return false;
			}

			if (randomMap > 2)
				randomMap = 2;
		}

		if (lowMidFlag & ROOM_LOWMID_MAPPLAYLIST)
		{
			if (mapPlaylistSize < 2)
			{
				Console().Warn("User '%d, %s' tried to create a new room with mapPlaylistSize < 2, mapPlaylistSize: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylistSize);
				return false;
			}

			if (!IsMapPlaylistAllowed(gameModeId))
			{
				Console().Warn("User '%d, %s' tried to create a new room with gameModeId which doesn't allow mapPlaylist, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
				return false;
			}

			if (mapPlaylistSize > 5)
			{
				Console().Warn("User '%d, %s' tried to create a new room with mapPlaylistSize > 5, mapPlaylistSize: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylistSize);
				return false;
			}

			if (!IsMapPlaylistValid(mapPlaylist))
			{
				Console().Warn("User '%d, %s' tried to create a new room with duplicate mapIds in mapPlaylist\n", user->GetID(), user->GetUsername().c_str());
				return false;
			}

			if (mapId != mapPlaylist[0].mapId)
			{
				Console().Warn("User '%d, %s' tried to create a new room with mapId which isn't the first in mapPlaylist, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId);
				return false;
			}

			for (int i = 0; i < mapPlaylistSize; i++)
			{
				if (g_pMapListTable->GetRowIdx(to_string(mapPlaylist[i].mapId)) < 0)
				{
					Console().Warn("User '%d, %s' tried to create a new room with an invalid mapId in mapPlaylist, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylist[i].mapId);
					return false;
				}

				if (!IsMapValid(gameModeId, mapPlaylist[i].mapId))
				{
					Console().Warn("User '%d, %s' tried to create a new room with an invalid mapId in mapPlaylist, mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylist[i].mapId, gameModeId);
					return false;
				}

				if (GetMapDefaultArmsRestriction(mapPlaylist[i].mapId) != 0)
				{
					Console().Warn("User '%d, %s' tried to create a new room with mapId which isn't allowed in mapPlaylist, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapPlaylist[i].mapId);
					return false;
				}
			}
		}

		if (gameModeId == 15)
		{
			if (lowMidFlag & ROOM_LOWMID_ZSDIFFICULTY)
			{
				int mapMaxZsDifficulty = g_pMapListTable->GetCell<int>("ZSmaxDifficulty", to_string(mapId));
				if (zsDifficulty > mapMaxZsDifficulty)
				{
					Console().Warn("User '%d, %s' tried to create a new room with zsDifficulty > mapMaxZsDifficulty: %d, zsDifficulty: %d, mapId: %d\n", user->GetID(), user->GetUsername().c_str(), mapMaxZsDifficulty, zsDifficulty, mapId);
					return false;
				}
			}
			else
			{
				Console().Warn("User '%d, %s' tried to create a new Zombie Scenario room without necessary settings\n", user->GetID(), user->GetUsername().c_str());
				return false;
			}
		}
	}

	LoadDefaultSettings(gameModeId, mapId);

	return true;
}

bool CRoomSettings::CheckNewSettings(IUser* user, CRoomSettings* roomSettings)
{
	if (lowFlag & ROOM_LOW_GAMEMODEID && gameModeId != roomSettings->gameModeId)
	{
		if (g_pGameModeListTable->GetRowIdx(to_string(gameModeId)) < 0)
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
			return false;
		}

		if (gameModeId != 39 && !IsFunGameMode(gameModeId) && !g_pGameModeListTable->GetCell<int>("mode_select_ui_order", to_string(gameModeId)))
		{
			Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), gameModeId);
			return false;
		}

		if (lowFlag & ROOM_LOW_MAPID && mapId != roomSettings->mapId)
		{
			if (g_pMapListTable->GetRowIdx(to_string(mapId)) < 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId, gameModeId);
				return false;
			}

			if (!IsMapValid(gameModeId, mapId))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId, gameModeId);
				return false;
			}

			LoadNewSettings(gameModeId, mapId, user, ROOM_LOW_GAMEMODEID | ROOM_LOW_MAPID);
		}
		else
		{
			if (!IsMapValid(gameModeId, roomSettings->mapId))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), roomSettings->mapId, gameModeId);
				return false;
			}

			LoadNewSettings(gameModeId, roomSettings->mapId, user, ROOM_LOW_GAMEMODEID);
		}
	}
	else 
	{
		if (lowFlag & ROOM_LOW_MAPID && mapId != roomSettings->mapId)
		{
			if (g_pMapListTable->GetRowIdx(to_string(mapId)) < 0)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId, roomSettings->gameModeId);
				return false;
			}

			if (!IsMapValid(roomSettings->gameModeId, mapId))
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid mapId: %d, gameModeId: %d\n", user->GetID(), user->GetUsername().c_str(), mapId, roomSettings->gameModeId);
				return false;
			}

			LoadNewSettings(roomSettings->gameModeId, mapId, user, ROOM_LOW_MAPID);
		}
		else
			LoadNewSettings(roomSettings->gameModeId, roomSettings->mapId, user);

		if (roomSettings->gameModeId == 0 || roomSettings->gameModeId == 3)
		{
			if ((lowMidFlag & ROOM_LOWMID_LEAGUERULE && leagueRule != roomSettings->leagueRule && !leagueRule) ||
				(lowMidFlag & ROOM_LOWMID_TEAMSWITCH && teamSwitch != roomSettings->teamSwitch && !teamSwitch)) // requesting league rule or team switch to be deactivated
			{
				lowFlag |= ROOM_LOW_WINLIMIT;
				winLimit = GetGameModeDefaultSetting(roomSettings->gameModeId, "mode_win_limit_id");
			}
			else if (lowFlag & ROOM_LOW_WINLIMIT)
			{
				if (roomSettings->leagueRule || roomSettings->teamSwitch) // changing winLimit with league rule or team switch
				{
					if (!IsLeagueRuleWinLimitValid(winLimit))
					{
						Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid winLimit: %d\n", user->GetID(), user->GetUsername().c_str(), winLimit);
						lowFlag &= ~ROOM_LOW_WINLIMIT;
					}
				}
				else // changing winLimit without league rule and team switch
				{
					if (!IsSettingValid(roomSettings->gameModeId, "mode_win_limit_id", winLimit))
					{
						if (winLimit != 6) // turning on league rule or team switch and then creating a new room makes the client sends this incorrectly, so let's just mute this
							Console().Warn("User '%d, %s' tried to update a room\'s settings with invalid winLimit: %d\n", user->GetID(), user->GetUsername().c_str(), winLimit);

						lowFlag &= ~ROOM_LOW_WINLIMIT;
					}
				}
			}
		}

		if (roomSettings->gameModeId != 9)
		{
			if (lowMidFlag & ROOM_LOWMID_ZBLIMIT)
			{
				if (zbLimitFlag == 1 && !roomSettings->fireBomb)
				{
					Console().Warn("User '%d, %s' tried to update a room\'s settings with zbLimitFlag == 1 when roomSettings->fireBomb == 0\n", user->GetID(), user->GetUsername().c_str());
					lowMidFlag &= ~ROOM_LOWMID_ZBLIMIT;
				}
			}
		}

		if (highMidFlag & ROOM_HIGHMID_FIREBOMB)
		{
			if (!fireBomb && roomSettings->zbLimitFlag == 1)
			{
				Console().Warn("User '%d, %s' tried to update a room\'s settings with fireBomb == 0 when roomSettings->zbLimitFlag == 1\n", user->GetID(), user->GetUsername().c_str());
				highMidFlag &= ~ROOM_HIGHMID_FIREBOMB;
			}
		}
	}

	return true;
}