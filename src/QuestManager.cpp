#include "QuestManager.h"

using namespace std;

CQuestManager::CQuestManager()
{
	Init();
}

CQuestManager::~CQuestManager()
{
	Shutdown();
}

void CQuestManager::Init()
{
	Shutdown();

	LoadQuests();
	LoadEventQuests();
	LoadClanQuests();
}

void CQuestManager::Shutdown()
{
	for (auto quest : m_Quests)
	{
		delete quest;
	}
	m_Quests.clear();

	for (auto quest : m_EventQuests)
	{
		delete quest;
	}
	m_EventQuests.clear();

	for (auto quest : m_ClanQuests)
	{
		delete quest;
	}
	m_ClanQuests.clear();
}

void CQuestManager::LoadQuests()
{
}

void CQuestManager::LoadEventQuests()
{
}

void CQuestManager::LoadClanQuests()
{
}

vector<CQuest*>& CQuestManager::GetQuests()
{
	return m_Quests;
}

void CQuestManager::OnPacket(CReceivePacket* msg, CExtendedSocket* socket)
{
	LOG_PACKET;

	CUser* user = g_pUserManager->GetUserBySocket(socket);
	if (user == NULL)
		return;

	int requestID = msg->ReadUInt8();
	switch (requestID)
	{
	case QuestPacketType::ReceiveReward:
		//OnReceiveReward(user, msg->ReadUInt8(), msg->ReadUInt32());
		break;
	case QuestPacketType::SpecialMissionRequest:
		//OnSpecialMissionRequest(user, msg);
		break;
	case QuestPacketType::SetFavouriteQuest:
		//OnSetFavouriteRequest(user, msg);
		break;
	default:
		g_pConsole->Warn(OBFUSCATE("[User '%s'] Packet_Quest type %d is not implemented\n"), user->GetLogName(), requestID);
		break;
	};
}

void CQuestManager::OnTitlePacket(CReceivePacket* msg, CExtendedSocket* socket)
{
	LOG_PACKET;

	CUser* user = g_pUserManager->GetUserBySocket(socket);
	if (user == NULL)
		return;

	int requestID = msg->ReadUInt8();
	switch (requestID)
	{
	case 0:
		OnTitleSetPrefixRequest(user, msg);
		break;
	case 1:
		OnTitleListSetRequest(user, msg);
		break;
	case 2:
		OnTitleListRemoveRequest(user, msg);
		break;
	default:
		g_pConsole->Warn(OBFUSCATE("[User '%s'] Packet_Title type %d is not implemented\n"), user->GetLogName(), requestID);
		break;
	};
}

void CQuestManager::OnMatchMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto quest : m_Quests)
	{
		quest->OnMinuteTick(userStat, gameMatch);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnMinuteTick(userStat, gameMatch);
	}
}

void CQuestManager::OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent& killEvent)
{
	for (auto quest : m_Quests)
	{
		quest->OnKillEvent(userStat, gameMatch, killEvent);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnKillEvent(userStat, gameMatch, killEvent);
	}
}

void CQuestManager::OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto quest : m_Quests)
	{
		quest->OnBombExplode(userStat, gameMatch);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnBombExplode(userStat, gameMatch);
	}
}

void CQuestManager::OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto quest : m_Quests)
	{
		quest->OnBombDefuse(userStat, gameMatch);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnBombDefuse(userStat, gameMatch);
	}
}

void CQuestManager::OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto quest : m_Quests)
	{
		quest->OnHostageEscape(userStat, gameMatch);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnHostageEscape(userStat, gameMatch);
	}
}

void CQuestManager::OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType)
{
	for (auto quest : m_Quests)
	{
		quest->OnMonsterKill(userStat, gameMatch, monsterType);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnMonsterKill(userStat, gameMatch, monsterType);
	}
}

void CQuestManager::OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto quest : m_Quests)
	{
		quest->OnMosquitoKill(userStat, gameMatch);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnMosquitoKill(userStat, gameMatch);
	}
}

void CQuestManager::OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto quest : m_Quests)
	{
		quest->OnKiteKill(userStat, gameMatch);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnKiteKill(userStat, gameMatch);
	}
}

void CQuestManager::OnLevelUpEvent(CUser* user, int level, int newLevel)
{
	for (auto quest : m_Quests)
	{
		quest->OnLevelUpEvent(user, level, newLevel);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnLevelUpEvent(user, level, newLevel);
	}
}

void CQuestManager::OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam)
{
	for (auto quest : m_Quests)
	{
		quest->OnMatchEndEvent(userStat, gameMatch, userTeam);
	}

	for (auto quest : m_EventQuests)
	{
		quest->OnMatchEndEvent(userStat, gameMatch, userTeam);
	}
}

void CQuestManager::OnGameMatchLeave(CUser* user, vector<UserQuestProgress>& questsProgress, vector<UserQuestProgress>& questsEventsProgress)
{
	for (auto questProgress : questsProgress)
	{
		CQuest* quest = GetQuest(questProgress.questID);
		if (quest)
		{
			quest->ApplyProgress(user, questProgress);
		}
	}

	for (auto questProgress : questsEventsProgress)
	{
		CQuestEvent* quest = GetEventQuest(questProgress.questID);
		if (quest)
		{
			quest->ApplyProgress(user, questProgress);
		}
	}
}

void CQuestManager::OnUserLogin(CUser* user)
{
	for (auto quest : m_EventQuests)
	{
		quest->OnUserLogin(user);
	}
}

void CQuestManager::OnQuestTaskFinished(CUser* user, UserQuestTaskProgress& taskProgress, CQuestTask* task, CQuest* quest)
{
	if (!quest->IsAllTaskFinished(user))
		return;

	UserQuestProgress questProgress = {};
	if (g_pUserDatabase->GetQuestProgress(user->GetID(), quest->GetID(), questProgress) <= 0)
		return;

	questProgress.status = UserQuestStatus::QUEST_DONE;

	OnQuestFinished(user, quest, questProgress);
}

void CQuestManager::OnQuestEventTaskFinished(CUser* user, UserQuestTaskProgress& taskProgress, CQuestEventTask* task, CQuestEvent* quest)
{
	g_pItemManager->GiveReward(user->GetID(), user, task->GetRewardID());

	if (!quest->IsAllTaskFinished(user))
		return;

	UserQuestProgress questProgress = {};
	if (g_pUserDatabase->GetQuestEventProgress(user->GetID(), quest->GetID(), questProgress) <= 0)
		return;

	questProgress.status = UserQuestStatus::QUEST_DONE;
}

void CQuestManager::OnQuestFinished(CUser* user, CQuest* quest, UserQuestProgress& questProgress)
{
	if (quest->GetType() == QuestType::QUEST_HONOR)
	{
		int titleID = quest->GetID() - 2000;

		user->UpdateAchievementList(titleID);

		g_pPacketManager->SendTitle(user->GetExtendedSocket(), titleID);
	}
	else if (quest->GetType() == QuestType::QUEST_DAILY)
	{
		UserQuestStat stat = {};
		g_pUserDatabase->GetQuestStat(user->GetID(), 0x8 | 0x20, stat);
		stat.dailyMissionsCompletedToday++;

		// move it to new func?
		int dailyQuestsCount = 0;
		for (auto quest : m_Quests)
		{
			if (quest->GetType() == QuestType::QUEST_DAILY)
				dailyQuestsCount++;
		}

		if (stat.dailyMissionsCompletedToday == dailyQuestsCount)
			stat.dailyMissionsCleared++;

		g_pUserDatabase->UpdateQuestStat(user->GetID(), 0x8 | 0x20, stat);
		g_pPacketManager->SendQuestUpdateQuestStat(user->GetExtendedSocket(), 0x8 | 0x20, 0, stat);
	}

	g_pPacketManager->SendQuestUpdateMainInfo(user->GetExtendedSocket(), 0xFFFF, quest, questProgress);
	g_pUserDatabase->UpdateQuestProgress(user->GetID(), questProgress);

	g_pConsole->Log(OBFUSCATE("[User '%s'] completed quest %d, %s, room id: %d\n"), user->GetLogName(), quest->GetID(), quest->GetName().c_str(), user->GetCurrentRoom() ? user->GetCurrentRoom()->GetID() : 0);
}

void CQuestManager::OnReceiveReward(CUser* user, int rewardID, int questID)
{
	CQuest* quest = GetQuest(questID);
	if (!quest)
		return;

	// there can be only 1 reward for honor quests. Client sends rewardID = 0, so force rewardID to 1
	if (quest->GetType() == QuestType::QUEST_HONOR)
		rewardID = 1;

	UserQuestProgress questProgress = GetUserQuestProgress(quest, user->GetID());
	if (questProgress.status != UserQuestStatus::QUEST_DONE)
		return;

	QuestReward_s reward = GetQuestReward(quest, rewardID);
	if (reward.rewardID)
		g_pItemManager->GiveReward(user->GetID(), user, reward.rewardID);

	questProgress.status = UserQuestStatus::QUEST_DONE_AND_REWARD_RECEIVED;
	g_pUserDatabase->UpdateQuestProgress(user->GetID(), questProgress);

	g_pPacketManager->SendQuestUpdateMainInfo(user->GetExtendedSocket(), 0xFFFF, quest, questProgress);

	CUserCharacter character = user->GetCharacter(UFLAG_ACHIEVEMENT);
	UserQuestStat stat = {};
	g_pPacketManager->SendQuestUpdateQuestStat(user->GetExtendedSocket(), 1, character.honorPoints, stat); // update honor stat
}

void CQuestManager::OnSpecialMissionRequest(CUser* user, CReceivePacket* msg)
{
	int questID = msg->ReadUInt16();
	int unk = msg->ReadUInt16();

	CQuest* quest = GetQuest(questID);
	if (!quest || quest->GetType() != QuestType::QUEST_SPECIAL)
		return;

	UserQuestProgress questProgress = GetUserQuestProgress(quest, user->GetID());
	if (questProgress.status != UserQuestStatus::QUEST_WAIT_FOR_OPEN)
		return;

	questProgress.status = UserQuestStatus::QUEST_IN_PROGRESS;
	g_pUserDatabase->UpdateQuestProgress(user->GetID(), questProgress);

	g_pPacketManager->SendQuestUpdateMainInfo(user->GetExtendedSocket(), 0xFFFF, quest, questProgress);

	g_pConsole->Log(OBFUSCATE("[User '%s'] CQuestManager::OnSpecialMissionRequest: %d\n"), user->GetLogName(), questID);
}

void CQuestManager::OnSetFavouriteRequest(CUser* user, CReceivePacket* msg)
{
	int questID = msg->ReadUInt32();
	bool favourite = msg->ReadUInt8();

	CQuest* quest = GetQuest(questID);
	if (!quest)
		return;

	UserQuestProgress questProgress = GetUserQuestProgress(quest, user->GetID());
	questProgress.favourite = favourite;

	g_pUserDatabase->UpdateQuestProgress(user->GetID(), questProgress);
	g_pPacketManager->SendQuestUpdateMainInfo(user->GetExtendedSocket(), 0xFFFF, quest, questProgress);
}

void CQuestManager::OnTitleSetPrefixRequest(CUser* user, CReceivePacket* msg)
{
	int titleID = msg->ReadInt16();
	if (titleID != 0)
	{
		int honorQuestID = titleID + 2000;
		CQuest* quest = GetQuest(honorQuestID);
		if (!quest)
			return;

		UserQuestProgress questProgress = GetUserQuestProgress(quest, user->GetID());
		if (questProgress.status == UserQuestStatus::QUEST_IN_PROGRESS)
			return;
	}

	user->UpdatePrefix(titleID);
}

void CQuestManager::OnTitleListSetRequest(CUser* user, CReceivePacket* msg)
{
	int slot = msg->ReadInt8();
	int titleID = msg->ReadInt16();

	int honorQuestID = titleID + 2000;
	CQuest* quest = GetQuest(honorQuestID);
	if (!quest)
		return;

	UserQuestProgress questProgress = GetUserQuestProgress(quest, user->GetID());
	if (questProgress.status == UserQuestStatus::QUEST_IN_PROGRESS)
		return;

	user->UpdateTitles(slot, titleID);
}

void CQuestManager::OnTitleListRemoveRequest(CUser* user, CReceivePacket* msg)
{
	int slot = msg->ReadInt8();
	if (slot >= 5)
	{
		return;
	}

	/*int titleID = msg->ReadInt16();

	int honorQuestID = titleID + 2000;
	Quest_s quest = GetQuest(honorQuestID);
	if (!quest.id)
	{
	return;
	}

	UserQuestProgress questProgress = GetUserQuestProgress(quest, user->GetData());
	if (questProgress.status != UserQuestStatus::QUEST_DONE)
	{
	return;
	}*/

	user->UpdateTitles(slot, 0);
}

CQuest* CQuestManager::GetQuest(int questID)
{
	vector<CQuest*> quests = m_Quests;
	vector<CQuest*>::iterator questIt = find_if(quests.begin(), quests.end(),
		[questID](CQuest* quest) { return questID == quest->GetID(); });

	if (questIt != quests.end())
		return *questIt;

	return NULL;
}

CQuestEvent* CQuestManager::GetEventQuest(int questID)
{
	vector<CQuestEvent*>::iterator questIt = find_if(m_EventQuests.begin(), m_EventQuests.end(),
		[questID](CQuestEvent* quest) { return questID == quest->GetID(); });

	if (questIt != m_EventQuests.end())
		return *questIt;

	return NULL;
}

QuestReward_s CQuestManager::GetQuestReward(CQuest* quest, int rewardID)
{
	QuestReward_s nullQuest = {};

	vector<QuestReward_s> rewards = quest->GetRewards();
	vector<QuestReward_s>::iterator questIt = find_if(rewards.begin(), rewards.end(),
		[rewardID](QuestReward_s questReward) { return rewardID == questReward.id; });

	if (questIt != rewards.end())
		return *questIt;

	return nullQuest;
}

UserQuestProgress CQuestManager::GetUserQuestProgress(CQuest* quest, int userID)
{
	int questID = quest->GetID();

	UserQuestProgress progress = {};
	if (g_pUserDatabase->GetQuestProgress(userID, questID, progress))
	{
		return progress;
	}

	progress.questID = questID;
	if (quest->GetType() == QuestType::QUEST_SPECIAL)
		progress.status = 3;

	return progress;
}

UserQuestTaskProgress CQuestManager::GetUserQuestTaskProgress(CQuest* quest, int userID, int taskID)
{
	int questID = quest->GetID();

	UserQuestTaskProgress questTaskProgress = {};
	if (g_pUserDatabase->GetQuestTaskProgress(userID, questID, taskID, questTaskProgress))
	{
		return questTaskProgress;
	}

	questTaskProgress.taskID = taskID;

	return questTaskProgress;
}