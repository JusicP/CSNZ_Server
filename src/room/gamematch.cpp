#include "gamematch.h"
#include "serverconfig.h"
#include "manager/questmanager.h"
#include "manager/packetmanager.h"
#include "manager/minigamemanager.h"
#include "manager/itemmanager.h"
#include "manager/hostmanager.h"
#include "manager/userdatabase.h"

#include "user/userinventoryitem.h"

using namespace std;

CGameMatchUserStat::CGameMatchUserStat()
{
	m_pUser = NULL;
	m_nScore = 0;
	m_nKills = 0;
	m_nDeaths = 0;
	m_nPointsEarned = 0;
	m_nBonusPointsEarned = 0;
	m_nExpEarned = 0;
	m_nBonusExpEarned = 0;
	m_nItemBonusPoints = 0;
	m_nItemBonusExp = 0;
	m_nClassBonusPoints = 0;
	m_nClassBonusExp = 0;
	m_nClanBonusPoints = 0;
	m_nClanBonusExp = 0;
	m_nEventBonusPoints = 0;
	m_nEventBonusExp = 0;
	m_nPlayerCoopBonusPoints = 0;
	m_nPlayerCoopBonusExp = 0;
	m_nClassItemID = 0;
	m_nIsLevelUp = false;
	m_nRank = 0;
}

void CGameMatchUserStat::IncrementScore(int score)
{
	m_nScore += score;
}

void CGameMatchUserStat::UpdateKillsCount(int kills)
{
	m_nKills = kills;
}

void CGameMatchUserStat::UpdateDeathsCount(int deaths)
{
	m_nDeaths = deaths;
}

void CGameMatchUserStat::UpdateClass(int classItemID)
{
	m_nClassItemID = classItemID;
}

void CGameMatchUserStat::UpdateZbsRank(int rankPoints)
{
	if (rankPoints < 10)
	{
		m_nRank = 0;
	}
	else if (rankPoints < 25)
	{
		m_nRank = 1;
	}
	else if (rankPoints < 40)
	{
		m_nRank = 2;
	}
	else if (rankPoints < 50)
	{
		m_nRank = 3;
	}
	else if (rankPoints >= 50)
	{
		m_nRank = 4;
	}
}

void CGameMatchUserStat::IncrementKillCount()
{
	m_nKills++;
}

UserQuestProgress& CGameMatchUserStat::GetTempQuestProgress(int questID)
{
	UserQuestProgress tempQuestProgress = {};
	auto tempQuestTaskProgressIt = find_if(m_TempQuestProgress.begin(), m_TempQuestProgress.end(),
		[questID](const UserQuestProgress& tempQuestProgress) { return tempQuestProgress.questID == questID; });

	if (tempQuestTaskProgressIt != m_TempQuestProgress.end())
	{
		return *tempQuestTaskProgressIt;
	}
	else
	{
		g_UserDatabase.GetQuestProgress(m_pUser->GetID(), questID, tempQuestProgress);
		m_TempQuestProgress.push_back(tempQuestProgress);
	}

	return m_TempQuestProgress[m_TempQuestProgress.size() - 1];
}

UserQuestTaskProgress& CGameMatchUserStat::GetTempQuestTaskProgress(int questID, int taskID)
{
	UserQuestTaskProgress tempQuestTaskProgress = {};
	UserQuestProgress& tempQuestProgress = GetTempQuestProgress(questID);

	auto tempTaskProgressIt = find_if(tempQuestProgress.tasks.begin(), tempQuestProgress.tasks.end(),
		[taskID](const UserQuestTaskProgress& taskProgress) { return taskProgress.taskID == taskID; });

	if (tempTaskProgressIt != tempQuestProgress.tasks.end())
	{
		return *tempTaskProgressIt;
	}
	else
	{
		g_UserDatabase.GetQuestTaskProgress(m_pUser->GetID(), questID, taskID, tempQuestTaskProgress);
		tempQuestProgress.tasks.push_back(tempQuestTaskProgress);
	}

	return tempQuestProgress.tasks[tempQuestProgress.tasks.size() - 1];
}

UserQuestProgress& CGameMatchUserStat::GetTempQuestEventProgress(int questID)
{
	UserQuestProgress tempQuestProgress = {};
	auto tempQuestTaskProgressIt = find_if(m_TempQuestEventProgress.begin(), m_TempQuestEventProgress.end(),
		[questID](const UserQuestProgress& tempQuestProgress) { return tempQuestProgress.questID == questID; });

	if (tempQuestTaskProgressIt != m_TempQuestEventProgress.end())
	{
		return *tempQuestTaskProgressIt;
	}
	else
	{
		g_UserDatabase.GetQuestEventProgress(m_pUser->GetID(), questID, tempQuestProgress);
		m_TempQuestEventProgress.push_back(tempQuestProgress);
	}

	return m_TempQuestEventProgress[m_TempQuestEventProgress.size() - 1];
}

UserQuestTaskProgress& CGameMatchUserStat::GetTempQuestEventTaskProgress(int questID, int taskID)
{
	UserQuestTaskProgress tempQuestTaskProgress = {};
	UserQuestProgress& tempQuestEventProgress = GetTempQuestEventProgress(questID);

	auto tempTaskProgressIt = find_if(tempQuestEventProgress.tasks.begin(), tempQuestEventProgress.tasks.end(),
		[taskID](const UserQuestTaskProgress& taskProgress) { return taskProgress.taskID == taskID; });

	if (tempTaskProgressIt != tempQuestEventProgress.tasks.end())
	{
		return *tempTaskProgressIt;
	}
	else
	{
		g_UserDatabase.GetQuestEventTaskProgress(m_pUser->GetID(), questID, taskID, tempQuestTaskProgress);
		tempQuestEventProgress.tasks.push_back(tempQuestTaskProgress);
	}

	return tempQuestEventProgress.tasks[tempQuestEventProgress.tasks.size() - 1];
}

CUserInventoryItem* CGameMatchUserStat::GetItem(int itemID)
{
	auto itemIt = find_if(m_Items.begin(), m_Items.begin(),
		[itemID](const CUserInventoryItem& item) { return item.m_nItemID == itemID; });

	if (itemIt != m_Items.end())
	{
		return &*itemIt;
	}

	return NULL;
}

CGameMatch::CGameMatch(IRoom* room, int gameMode, int mapID)
{
	m_pParentRoom = room;
	m_nGameMode = gameMode;
	m_nMapID = mapID;
	m_nCtWinCount = 0;
	m_nTerWinCount = 0;
	m_nFirstPlaceUserId = 0;
	m_nSecondCounter = 0;
}

CGameMatch::~CGameMatch()
{
	for (auto u : m_UserStats)
	{
		g_QuestManager.OnGameMatchLeave(u->m_pUser, u->m_TempQuestProgress, u->m_TempQuestEventProgress);
		delete u;
	}
}

void CGameMatch::Connect(IUser* user)
{
	if (GetGameUserStat(user) == NULL)
	{
		CGameMatchUserStat* userStat = new CGameMatchUserStat();
		userStat->m_pUser = user;

		m_UserStats.push_back(userStat);
	}
}

void CGameMatch::Disconnect(IUser* user)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (stat)
	{
		g_QuestManager.OnGameMatchLeave(user, stat->m_TempQuestProgress, stat->m_TempQuestEventProgress);

		delete stat;
		m_UserStats.erase(remove(begin(m_UserStats), end(m_UserStats), stat), end(m_UserStats));
	}
}

bool GameMatchUserStatCompare(CGameMatchUserStat* gameStat, IUser* user)
{
	return gameStat->m_pUser == user;
}

CGameMatchUserStat* CGameMatch::GetGameUserStat(IUser* user)
{
	auto it = find_if(m_UserStats.begin(), m_UserStats.end(), bind(GameMatchUserStatCompare, placeholders::_1, user));

	if (it != m_UserStats.end())
	{
		CGameMatchUserStat* stat = *it;
		if (stat)
		{
			return stat;
		}
		else
		{
			Logger().Warn("CGameMatch::GetGameUserStat: user stat does not exist\n");
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

void CGameMatch::OnUpdateScore(IUser* user, int score)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);

	if (stat)
	{
		stat->IncrementScore(score);
	}
}

void CGameMatch::OnUpdateKillCounter(IUser* user, int kills)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);

	if (stat)
	{
		stat->UpdateKillsCount(kills);
	}
}

void CGameMatch::OnUpdateDeathCounter(IUser* user, int deaths)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);

	if (stat)
	{
		stat->UpdateDeathsCount(deaths);
	}
}

void CGameMatch::OnKillEvent(IUser* user, GameMatch_KillEvent& killEvent)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;

	// don't count player suicide
	if (killEvent.killerUserID != killEvent.victimUserID)
		stat->IncrementKillCount();

	g_QuestManager.OnKillEvent(stat, this, killEvent);

	// random letters for weapon release event
	if (g_pServerConfig->activeMiniGamesFlag & kEventFlag_WeaponRelease)
	{
		if (yesOrNo(2.0f))
		{
			Randomer rand(g_pServerConfig->weaponRelease.characters.size() - 1);
			char character = g_pServerConfig->weaponRelease.characters[rand()];
			g_MiniGameManager.WeaponReleaseAddCharacter(user, character, 1);
			g_PacketManager.SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), va(OBFUSCATE("[Weapon Release] You have obtained '%s' character."), character == '~' ? OBFUSCATE("Joker") : va("%c", character)));
			g_PacketManager.SendMiniGameWeaponReleaseIGNotice(user->GetExtendedSocket(), character);
		}
	}

	// TODO: rewrite
	// random gachapon item for each 100 kills
	CUserCharacterExtended character = {};
	character.flag = EXT_UFLAG_KILLSTOGETGACHAPONITEM;
	g_UserDatabase.GetCharacterExtended(user->GetID(), character);

	character.killsToGetGachaponItem--;
	if (character.killsToGetGachaponItem <= 0)
	{
		character.killsToGetGachaponItem = 100;

		Randomer rand(1);
		int randomIndex = rand();
		int randomItemID = randomIndex ? 166 : 5101;

		g_PacketManager.SendItemGachapon(user->GetExtendedSocket(), randomIndex);

		g_ItemManager.AddItem(user->GetID(), user, randomItemID, 1, 0);
	}

	g_UserDatabase.UpdateCharacterExtended(user->GetID(), character);
}

void CGameMatch::OnUpdateWinCounter(int ctWinCount, int tWinCount)
{
	m_nCtWinCount = ctWinCount;
	m_nTerWinCount = tWinCount;

	//UpdateRoundWinCount(ctWinCount, tWinCount);
}

void CGameMatch::OnUpdateClass(IUser* user, int classItemID)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);

	if (stat)
	{
		stat->UpdateClass(classItemID);
	}
}

void CGameMatch::OnBombExplode(IUser* user)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;

	g_QuestManager.OnBombExplode(stat, this);
}

void CGameMatch::OnBombDefuse(IUser* user)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;

	g_QuestManager.OnBombDefuse(stat, this);
}

void CGameMatch::OnHostageEscape(IUser* user)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;

	g_QuestManager.OnHostageEscape(stat, this);
}

void CGameMatch::OnMonsterKill(IUser* user, int monsterType)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;	

	g_QuestManager.OnMonsterKill(stat, this, monsterType);
}

void CGameMatch::OnDropBoxPickup(IUser* user, int rewardID)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;

#ifndef PUBLIC_RELEASE
	if (m_nGameMode != 15 && m_nGameMode != 17 && m_nGameMode != 26 && m_nGameMode != 28)
	{
		Logger().Info("[SuspectNotice] detected suspect user '%d, %s', reason: %d, %s, %d, %d\n", user->GetID(), user->GetUsername().c_str(), 227, "DROPBOXABUSE", rewardID, m_nGameMode);
		return;
	}

	if (rewardID < 3000 || rewardID > 3023)
	{
		Logger().Info("[SuspectNotice] detected suspect user '%d, %s', reason: %d, %s, %d\n", user->GetID(), user->GetUsername().c_str(), 228, "DROPBOXABUSE", rewardID);
		return;
	}
#endif	
	RewardNotice notice = g_ItemManager.GiveReward(user->GetID(), user, rewardID, 0, true, 0);
	if (notice.status)
		g_PacketManager.SendUMsgSystemReply(user->GetExtendedSocket(), SystemReply_Red, OBFUSCATE("EVT_SCENARIO_ITEM_POINT_REWARD"), vector<string> {to_string(notice.points)});
}

void CGameMatch::OnMosquitoKill(IUser* user)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;

	g_QuestManager.OnMosquitoKill(stat, this);
}

void CGameMatch::OnKiteKill(IUser* user)
{
	CGameMatchUserStat* stat = GetGameUserStat(user);
	if (!stat)
		return;

	g_QuestManager.OnKiteKill(stat, this);
}

void CGameMatch::OnGameMatchEnd()
{
	CalculateGameResult();
	ApplyGameResult();

	Logger().Info("CGameMatch::OnGameMatchEnd: gamemode: %d, CT rounds win count: %d, T rounds win count: %d\n", m_nGameMode, m_nCtWinCount, m_nTerWinCount);
}

int CGameMatch::GetExpCoefficient()
{
	auto it = find_if(g_pServerConfig->gameMatch.gameModeCoefficients.begin(), g_pServerConfig->gameMatch.gameModeCoefficients.end(),
		[this](const GameMatchCoefficients_s& gameMatchCoef) { return gameMatchCoef.gameMode == this->m_nGameMode; });
	if (it != g_pServerConfig->gameMatch.gameModeCoefficients.end())
	{
		return it->exp;
	}
	else
	{
		auto it = find_if(g_pServerConfig->gameMatch.gameModeCoefficients.begin(), g_pServerConfig->gameMatch.gameModeCoefficients.end(),
			[](const GameMatchCoefficients_s& gameMatchCoef) { return gameMatchCoef.gameMode == 0; });
		if (it != g_pServerConfig->gameMatch.gameModeCoefficients.end())
		{
			return it->exp;
		}
		else
		{
			return 0;
		}
	}
}

int CGameMatch::GetPointsCoefficient()
{
	auto it = find_if(g_pServerConfig->gameMatch.gameModeCoefficients.begin(), g_pServerConfig->gameMatch.gameModeCoefficients.end(),
		[this](const GameMatchCoefficients_s& gameMatchCoef) { return gameMatchCoef.gameMode == this->m_nGameMode; });
	if (it != g_pServerConfig->gameMatch.gameModeCoefficients.end())
	{
		return it->points;
	}
	else
	{
		auto it = find_if(g_pServerConfig->gameMatch.gameModeCoefficients.begin(), g_pServerConfig->gameMatch.gameModeCoefficients.end(),
			[](const GameMatchCoefficients_s& gameMatchCoef) { return gameMatchCoef.gameMode == 0; });
		if (it != g_pServerConfig->gameMatch.gameModeCoefficients.end())
		{
			return it->points;
		}
		else
		{
			return 0;
		}
	}
}

bool CGameMatch::IsZombieMode()
{
	switch (m_nGameMode)
	{
	case 8:
	case 9:
	case 14:
	case 29:
		return true;
	};

	return false;
}
	

void CGameMatch::SetSaveData(const vector<unsigned char>& saveData)
{
	m_SaveData = saveData;
}

vector<unsigned char>& CGameMatch::GetSaveData()
{
	return m_SaveData;
}

void CGameMatch::OnHostChanged(IUser* newHost)
{
	for (auto userStat : m_UserStats)
	{
		g_HostManager.OnHostChanged(userStat->m_pUser, newHost, this);
	}
}

//const char* Чайник() { return "С функцией жопа"; }

bool GameMatchUserStatSortCompare(CGameMatchUserStat* a1, CGameMatchUserStat* a2)
{
	return a1->m_nKills > a2->m_nKills;
}

void CGameMatch::CalculateFirstPlace()
{
	sort(m_UserStats.begin(), m_UserStats.end(), GameMatchUserStatSortCompare);
}

void CGameMatch::CalculateGameResult()
{
	CalculateFirstPlace();

	int expCoef = GetExpCoefficient();
	int pointsCoef = GetPointsCoefficient();

	for (auto stat : m_UserStats)
	{
		IUser* user = stat->m_pUser;

		// calc exp
		stat->m_nExpEarned = stat->m_nKills > 0 ? expCoef * stat->m_nKills : expCoef;

		// calc points
		stat->m_nPointsEarned = stat->m_nKills > 0 ? pointsCoef * stat->m_nKills : pointsCoef;

		// calc bonus exp/points
		auto bonusPercentageClass = find_if(g_pServerConfig->gameMatch.bonusPercentageClasses.begin(), g_pServerConfig->gameMatch.bonusPercentageClasses.end(),
			[stat](const BonusPercentage_s& bonus) { return bonus.itemID == stat->m_nClassItemID; });
		if (bonusPercentageClass != g_pServerConfig->gameMatch.bonusPercentageClasses.end())
		{
			BonusPercentage_s bonus = *bonusPercentageClass;
			vector<CUserInventoryItem> items;
			g_UserDatabase.GetInventoryItemsByID(user->GetID(), bonus.itemID, items);
			if (items.size())
			{
				auto activeClassItem = find_if(items.begin(), items.end(),
					[](const CUserInventoryItem& item) { return item.m_nInUse && item.m_nStatus; });
				if (activeClassItem != items.end())
				{
					stat->m_nBonusExpEarned += stat->m_nExpEarned * bonus.exp / 100;
					stat->m_nBonusPointsEarned += stat->m_nPointsEarned * bonus.points / 100;
					stat->m_nClassBonusExp += bonus.exp;
					stat->m_nClassBonusPoints += bonus.points;
				}
			}
		}

		/*for (auto bonusItem : g_pServerConfig->bonusPercentageClasses)
		{
			if (bonusItem.itemID == stat->m_nClassItemID)
			{
				vector<CUserInventoryItem*> items = user->GetData()->inventory->GetItemsByItemId(bonusItem.itemID);
				for (auto item : items)
				{
					if (item->m_nInUse && item->m_nStatus)
					{
						stat->m_nBonusExpEarned += stat->m_nExpEarned * bonusItem.exp / 100;
						stat->m_nBonusPointsEarned += stat->m_nPointsEarned * bonusItem.points / 100;
						stat->m_nItemBonusExp += bonusItem.exp;
						stat->m_nItemBonusPoints += bonusItem.points;
						break;
					}
				}
			}
		}*/

		ServerConfigGameMatch_s gameMatch = g_pServerConfig->gameMatch;

		// bonus for players
		if (m_UserStats.size() >= 2)
		{
			auto bonusPlayerCoop = find_if(gameMatch.bonusPlayerCoop.begin(), gameMatch.bonusPlayerCoop.end(), [this](const BonusPercentage_s& bonus) { return bonus.itemID == this->m_nGameMode; });
			if (bonusPlayerCoop != gameMatch.bonusPlayerCoop.end())
			{
				BonusPercentage_s bonus = *bonusPlayerCoop;

				int percentageExp = bonus.exp * bonus.coef * m_UserStats.size();
				int percentagePoints = bonus.points * bonus.coef * m_UserStats.size();

				stat->m_nBonusExpEarned += stat->m_nExpEarned * percentageExp / 100;
				stat->m_nBonusPointsEarned += stat->m_nPointsEarned * percentagePoints / 100;

				stat->m_nPlayerCoopBonusExp += percentageExp;
				stat->m_nPlayerCoopBonusPoints += percentagePoints;
			}
		}

		if (stat->m_nExpEarned > 9999)
		{
			stat->m_nExpEarned = 9999;
		}
		if (stat->m_nPointsEarned > 9999)
		{
			stat->m_nPointsEarned = 9999;
		}
	}
}

void CGameMatch::ApplyGameResult()
{
	for (auto stat : m_UserStats)
	{
		IUser* user = stat->m_pUser;

		int totalExp = stat->m_nExpEarned + stat->m_nBonusExpEarned;
		user->UpdateExp(totalExp);

		int totalPoints = stat->m_nPointsEarned + stat->m_nBonusPointsEarned;
		user->UpdatePoints(totalPoints);

		if (m_nGameMode == 0 || m_nGameMode == 1 || m_nGameMode == 2 || m_nGameMode == 6 || m_nGameMode == 22)
			user->UpdateStat(0, 0, stat->m_nKills, stat->m_nDeaths);
	}
}

void CGameMatch::PrintGameResult()
{
	// use ostringstream to reduce cpu time when printing
	char msg[2048];

	ostringstream log;
	snprintf(msg, sizeof(msg), OBFUSCATE("\nRoom (%d) game result, host time: %s, GameModeID: %d, MapID: %d\n%-6s|%-32s|%-10s|%-10s|%-10s|%-10s|%-10s\n"), m_pParentRoom->GetID(), FormatSeconds(m_nSecondCounter), m_nGameMode, m_nMapID,
		(const char*)OBFUSCATE("UserID"), (const char*)OBFUSCATE("Username"), (const char*)OBFUSCATE("Kills"), (const char*)OBFUSCATE("Deaths"), (const char*)OBFUSCATE("Score"), (const char*)OBFUSCATE("Exp"), (const char*)OBFUSCATE("Points"));
	log << msg;

	for (auto stat : m_UserStats)
	{
		snprintf(msg, sizeof(msg), OBFUSCATE("%-6d|%-32s|%-10d|%-10d|%-10d|+%-10d|+%-10d\n"), stat->m_pUser ? stat->m_pUser->GetID() : 0, stat->m_pUser ? stat->m_pUser->GetUsername().c_str() : "NONE", stat->m_nKills, stat->m_nDeaths,
			stat->m_nScore, stat->m_nExpEarned, stat->m_nPointsEarned);
		log << msg;
	}

	Logger().Info(log.str().c_str());
}

// TODO: find another way
int GetZbsMapSeason(int mapID)
{
	switch (mapID)
	{
	case 74:
	case 75:
	case 76:
	case 77:
	case 85:
	case 98:
	case 101:
		return 1;
	case 106:
	case 112:
	case 113:
	case 117:
		return 2;
	case 121:
	case 123:
	case 126:
	case 128:
		return 3;
	case 131:
	case 133:
	case 134:
	case 139:
	case 143:
		return 4;
	case 146:
	case 147:
	case 148:
	case 152:
	case 156:
	case 157:
	case 158:
	case 163:
	case 164:
	case 165:
	case 172:
	case 173:
	case 174:
		return 5;
	}

	return 0;
}

void CGameMatch::OnZBSWin()
{
	for (auto stat : m_UserStats)
	{
		int rewardID = stat->m_nRank <= 3 ? 100 + stat->m_nRank : 100 + stat->m_nRank + GetZbsMapSeason(m_nMapID) - 1;

		Logger().Info(OBFUSCATE("CGameMatch::OnZBSWin: rank: %d, rewardID: %d\n"), stat->m_nRank, rewardID);

		g_ItemManager.GiveReward(stat->m_pUser->GetID(), stat->m_pUser, rewardID, 0, false, 0);
	}
}