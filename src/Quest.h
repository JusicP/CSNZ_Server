#pragma once

#include "GameMatch.h"

class CQuest;
class CQuestTask;
class CQuestBaseCondition
{
public:
	CQuestBaseCondition(CQuestTask* task, int id);

	virtual bool Event_Internal(CUser* user);

	void SetEventType(int eventType);
	int GetID();
	int GetEventType();

protected:
	CQuestTask* m_pTask;
	int m_nID;
	int m_nEventType;
};

class CQuestTask
{
public:
	CQuestTask(CQuest* quest, int id, std::string name, int goal);

	void Event();

	// internal event for inheritance classes
	bool Event_Internal(CUser* user);
	void IncrementCount(CUser* user, int count = 0, bool setForce = false);
	void Done(CUser* user, UserQuestTaskProgress progress);
	void ApplyProgress(CUser* user, UserQuestTaskProgress progress);
	void AddCondition(CQuestBaseCondition* condition);

	void OnMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent killEvent);

	void OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType);
	void OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch);

	void OnLevelUpEvent(CUser* user, int level, int newLevel);
	void OnGameMatchLeave(CUser* user);
	void OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam);
	void OnUserLogin(CUser* user);

	std::string GetName();
	int GetID();
	int GetGoal();
	CQuest* GetQuest();
	bool IsFinished(CUser* user);

protected:
	CQuest* m_pQuest;
	int m_nID;
	int m_nGoal;
	std::string m_szName;
	std::vector<CQuestBaseCondition*> m_Conditions;
};

class CQuest
{
public:
	CQuest();
	~CQuest();

	void SetName(std::string name);
	std::string GetName();
	void SetDescription(std::string desc);
	std::string GetDescription();
	void SetID(int id);
	int GetID();
	void SetType(int type);
	int GetType();
	void SetNPCID(int npcID);
	int GetNPCID();
	void AddTask(CQuestTask* task);
	void AddReward(QuestReward_s& reward);
	CQuestTask* GetTask(int id);
	std::vector<CQuestTask*> GetTasks();
	std::vector<QuestReward_s> GetRewards();

	void ApplyProgress(CUser* user, UserQuestProgress& progress);

	void OnMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent& killEvent);

	void OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType);
	void OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch);
	void OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch);

	void OnLevelUpEvent(CUser* user, int level, int newLevel);
	void OnGameMatchLeave(CUser* user);
	void OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam);

	void OnTaskDone(CUser* user, UserQuestTaskProgress& taskProgress, CQuestTask* task);
	bool IsAllTaskFinished(CUser* user);

private:
	std::string m_szName;
	std::string m_szDescription;
	int m_nID;
	int m_nType; // type of quest(0 - daily, 1 - special, 2 - honor,  3 - weekly)
	int m_nNPCID; // npc shown on client side
	std::vector<CQuestTask*> m_Tasks;
	std::vector<QuestReward_s> m_Rewards;
};

class CQuestBaseConditionGameMatch : public CQuestBaseCondition
{
public:
	CQuestBaseConditionGameMatch(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseCondition(task, id)
	{
		m_GameModes = gameModes;
		m_Maps = maps;
		m_nPlayerCount = playerCount;
		m_bTemp = temp;
	}

	bool Event_Internal(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseCondition::Event_Internal(userStat->m_pUser))
			return false;

		if (m_GameModes.size() > 0 && std::find(m_GameModes.begin(), m_GameModes.end(), gameMatch->m_nGameMode) == m_GameModes.end())
			return false;

		if (m_Maps.size() > 0 && userStat->m_pUser->GetCurrentRoom() && std::find(m_Maps.begin(), m_Maps.end(), userStat->m_pUser->GetCurrentRoom()->GetSettings()->mapId) == m_Maps.end())
			return false;

		if (m_nPlayerCount > 0 && m_nPlayerCount > (int)gameMatch->m_UserStats.size())
			return false;

		return true;
	}

	void IncrementTempCount(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		UserQuestTaskProgress& tempProgress = userStat->GetTempQuestTaskProgress(m_pTask->GetQuest()->GetID(), m_pTask->GetID());
		tempProgress.unitsDone++;
		if (tempProgress.unitsDone >= m_pTask->GetGoal())
		{
			m_pTask->IncrementCount(userStat->m_pUser, m_pTask->GetGoal(), true);
		}
#if 0
		printf("[User '%s'] CQuestBaseTaskGameMatch::IncrementTempCount: quest name: %s, done: %d, goal: %d\n", user->GetLogName(), m_pQuest->GetName().c_str(), tempProgress.unitsDone, m_nGoal);
#endif
	}

	// what is it?
	bool IsTemp()
	{
		return m_bTemp;
	}

private:
	std::vector<int> m_GameModes;
	std::vector<int> m_Maps;
	int m_nPlayerCount;
	bool m_bTemp;
};

class CQuestConditionLevelUp : public CQuestBaseCondition
{
public:
	CQuestConditionLevelUp(CQuestTask* task, int id) : CQuestBaseCondition(task, id)
	{
	}

	void OnLevelUpEvent(CUser* user, int level, int newLevel)
	{
		if (!CQuestBaseCondition::Event_Internal(user))
			return;

		m_pTask->IncrementCount(user, newLevel - level);
	}
};

class CQuestTaskMonsterKill : public CQuestBaseConditionGameMatch
{
public:
	CQuestTaskMonsterKill(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount, int monsterType) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
		m_nMonsterType = monsterType;
	}

	void OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		if (monsterType == m_nMonsterType)
			IncrementTempCount(userStat, gameMatch);
	}

private:
	int m_nMonsterType;
};

class CQuestConditionKill : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionKill(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
		m_nKillerTeam = 0;
		m_nVictimKillType = 0;
		m_nVictimTeam = 0;
		m_nContinuous = 0;
		m_nGun = 0;
		m_nGunID = 0;
		m_nBot = 0;
		m_bHuman = 0;
	}

	void OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent killEvent)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		if (m_nKillerTeam > 0 && m_nKillerTeam != killEvent.killerTeam)
		{
			ResetContinuousCounter(userStat, gameMatch);
			return;
		}

		if (m_nVictimKillType >= 0 && m_nVictimKillType != killEvent.victimKillType)
		{
			ResetContinuousCounter(userStat, gameMatch);
			return;
		}

		if (m_nVictimTeam > 0 && m_nVictimTeam != killEvent.victimTeam)
		{
			ResetContinuousCounter(userStat, gameMatch);
			return;
		}
		else if (m_nVictimTeam == 0 && killEvent.killerTeam == killEvent.victimTeam)
		{
			ResetContinuousCounter(userStat, gameMatch);
			return;
		}

		if (m_nGun >= 0 && m_nGun != killEvent.gunID)
		{
			ResetContinuousCounter(userStat, gameMatch);
			return;
		}

		if (m_nBot >= 0 && m_nBot && killEvent.victimUserID != 0)
		{
			ResetContinuousCounter(userStat, gameMatch);
			return;
		}

		UserQuestTaskProgress& tempProgress = userStat->GetTempQuestTaskProgress(m_pTask->GetQuest()->GetID(), m_nID);
		if (m_nContinuous <= 0 || m_nContinuous <= tempProgress.killEventProgress.continuousKillsCounter)
		{
			IncrementTempCount(userStat, gameMatch);
			if (tempProgress.killEventProgress.continuousKillsCounter)
				ResetContinuousCounter(userStat, gameMatch);
		}
		else if (m_nContinuous > tempProgress.killEventProgress.continuousKillsCounter)
		{
			tempProgress.killEventProgress.continuousKillsCounter++;
		}
	}

	void SetCondition(int killerTeam, int victimKillType, int victimTeam, int continuous, int gun, int gunID, int bot)
	{
		m_nKillerTeam = killerTeam;
		m_nVictimKillType = victimKillType;
		m_nVictimTeam = victimTeam;
		m_nContinuous = continuous;
		m_nGun = gun;
		m_nGunID = gunID;
		m_nBot = bot;
	}

	void ResetContinuousCounter(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (m_nContinuous > 0)
		{
			UserQuestTaskProgress& tempProgress = userStat->GetTempQuestTaskProgress(m_pTask->GetQuest()->GetID(), m_nID);
			tempProgress.killEventProgress.continuousKillsCounter = 0;
		}
	}

private:
	int m_nKillerTeam;
	int m_nVictimKillType;
	int m_nVictimTeam;
	int m_nContinuous;
	int m_nGun; // 0 - melee, 1 - primary, 2 - secondary, 3 - he
	int m_nGunID;
	int m_nBot;
	bool m_bHuman;
};

class CQuestConditionBombExplode : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionBombExplode(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		IncrementTempCount(userStat, gameMatch);
	}
};


class CQuestConditionBombDefuse : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionBombDefuse(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		IncrementTempCount(userStat, gameMatch);
	}
};

class CQuestConditionVipKill : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionVipKill(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnVipKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		IncrementTempCount(userStat, gameMatch);
	}
};

class CQuestConditionWin : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionWin(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		if (userTeam == Terrorist && gameMatch->m_nTerWinCount < gameMatch->m_nCtWinCount)
			return;
		else if (userTeam == CounterTerrorist && gameMatch->m_nTerWinCount > gameMatch->m_nCtWinCount)
			return;

		m_pTask->IncrementCount(userStat->m_pUser);
	}
};

class CQuestConditionHostageEscape : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionHostageEscape(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		IncrementTempCount(userStat, gameMatch);
	}
};

class CQuestConditionTimeMatch : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionTimeMatch(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		IncrementTempCount(userStat, gameMatch);
	}
};

class CQuestConditionKillMonster : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionKillMonster(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void SetMonsterType(int monsterType)
	{
		m_nMonsterType = monsterType;
	}

	void OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		if (monsterType == m_nMonsterType)
			IncrementTempCount(userStat, gameMatch);
	}

private:
	int m_nMonsterType;
};

class CQuestConditionKillMosquito : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionKillMosquito(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		IncrementTempCount(userStat, gameMatch);
	}
};

class CQuestConditionKillKite : public CQuestBaseConditionGameMatch
{
public:
	CQuestConditionKillKite(CQuestTask* task, int id, bool temp, std::vector<int> gameModes, std::vector<int> maps, int playerCount) : CQuestBaseConditionGameMatch(task, id, temp, gameModes, maps, playerCount)
	{
	}

	void OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
	{
		if (!CQuestBaseConditionGameMatch::Event_Internal(userStat, gameMatch))
			return;

		IncrementTempCount(userStat, gameMatch);
	}
};