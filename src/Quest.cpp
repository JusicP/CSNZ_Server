#include "Quest.h"

using namespace std;

CQuestBaseCondition::CQuestBaseCondition(CQuestTask* task, int id)
{
	m_pTask = task;
	m_nID = id;
}

bool CQuestBaseCondition::Event_Internal(CUser* user)
{
	UserQuestTaskProgress progress = {};
	if (g_pUserDatabase->GetQuestTaskProgress(user->GetID(), m_pTask->GetQuest()->GetID(), m_pTask->GetID(), progress) <= 0)
		return false;

	if (progress.unitsDone >= m_pTask->GetGoal())
		return false;


	return true;
}

void CQuestBaseCondition::SetEventType(int eventType)
{
	m_nEventType = eventType;
}

int CQuestBaseCondition::GetEventType()
{
	return m_nEventType;
}

CQuestTask::CQuestTask(CQuest* quest, int id, string name, int goal)
{
	m_pQuest = quest;
	m_nID = id;
	m_szName = name;
	m_nGoal = goal;
}

void CQuestTask::Event()
{
	//Event_Internal();
}

// internal event for inheritance classes
bool CQuestTask::Event_Internal(CUser* user)
{
	UserQuestTaskProgress progress = {};
	if (g_pUserDatabase->GetQuestTaskProgress(user->GetID(), m_pQuest->GetID(), m_nID, progress) <= 0)
		return false;

	if (progress.unitsDone >= m_nGoal)
		return false;

	return true;
}

void CQuestTask::IncrementCount(CUser* user, int count, bool setForce)
{
	UserQuestTaskProgress progress = {};
	if (g_pUserDatabase->GetQuestTaskProgress(user->GetID(), m_pQuest->GetID(), m_nID, progress) <= 0)
		return;

	if (count < 0)
		count = 1;

	// lol
	count ? setForce ? progress.unitsDone += count : progress.unitsDone = count : progress.unitsDone++;
	if (progress.unitsDone >= m_nGoal)
	{
		progress.finished = true;
		progress.unitsDone = m_nGoal;
	}

	if (g_pUserDatabase->UpdateQuestTaskProgress(user->GetID(), m_pQuest->GetID(), progress) > 0)
	{
#if 0
		g_pConsole->Log(LOG_USER, CON_LOG, "[User '%s'] CQuestTask::IncrementCount: quest name: %s, done: %d, goal: %d\n", user->GetLogName(), m_pQuest->GetName().c_str(), progress.unitsDone, m_nGoal);
#endif
		g_pPacketManager->SendQuestUpdateTaskInfo(user->GetExtendedSocket(), 0xFF, m_pQuest->GetID(), this, progress);
		if (progress.unitsDone >= m_nGoal)
		{
			Done(user, progress);
		}
	}
}

void CQuestTask::Done(CUser* user, UserQuestTaskProgress progress)
{
	m_pQuest->OnTaskDone(user, progress, this);
}

void CQuestTask::AddCondition(CQuestBaseCondition* condition)
{
	m_Conditions.push_back(condition);
}

void CQuestTask::OnMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_TIMEMATCH)
		{
			CQuestConditionTimeMatch* conditionTime = dynamic_cast<CQuestConditionTimeMatch*>(condition);
			conditionTime->OnMinuteTick(userStat, gameMatch);
		}
	}
}

void CQuestTask::OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent killEvent)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILL)
		{
			CQuestConditionKill* conditionKill = dynamic_cast<CQuestConditionKill*>(condition);
			conditionKill->OnKillEvent(userStat, gameMatch, killEvent);
		}
	}
}

void CQuestTask::OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_BOMBEXPLODE)
		{
			CQuestConditionBombExplode* conditionExplode = dynamic_cast<CQuestConditionBombExplode*>(condition);
			conditionExplode->OnBombExplode(userStat, gameMatch);
		}
	}
}

void CQuestTask::OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_BOMBDEFUSE)
		{
			CQuestConditionBombDefuse* conditionDefuse = dynamic_cast<CQuestConditionBombDefuse*>(condition);
			conditionDefuse->OnBombDefuse(userStat, gameMatch);
		}
	}
}

void CQuestTask::OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_HOSTAGEESCAPE)
		{
			CQuestConditionHostageEscape* conditionHostageEscape = dynamic_cast<CQuestConditionHostageEscape*>(condition);
			conditionHostageEscape->OnHostageEscape(userStat, gameMatch);
		}
	}
}

void CQuestTask::OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILLMONSTER)
		{
			CQuestConditionKillMonster* conditionKillMonster = dynamic_cast<CQuestConditionKillMonster*>(condition);
			conditionKillMonster->OnMonsterKill(userStat, gameMatch, monsterType);
		}
	}
}

void CQuestTask::OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILLMOSQUITO)
		{
			CQuestConditionKillMosquito* conditionKillMosquito = dynamic_cast<CQuestConditionKillMosquito*>(condition);
			conditionKillMosquito->OnMosquitoKill(userStat, gameMatch);
		}
	}
}

void CQuestTask::OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILLKITE)
		{
			CQuestConditionKillKite* conditionKillKite = dynamic_cast<CQuestConditionKillKite*>(condition);
			conditionKillKite->OnKiteKill(userStat, gameMatch);
		}
	}
}

void CQuestTask::OnLevelUpEvent(CUser* user, int level, int newLevel)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_LEVELUP)
		{
			CQuestConditionLevelUp* conditionLevelUP = dynamic_cast<CQuestConditionLevelUp*>(condition);
			conditionLevelUP->OnLevelUpEvent(user, level, newLevel);
		}
	}
}

void CQuestTask::OnGameMatchLeave(CUser* user)
{

}

void CQuestTask::OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_MATCHWIN)
		{
			CQuestConditionWin* conditionMatchWin = dynamic_cast<CQuestConditionWin*>(condition);
			conditionMatchWin->OnMatchEndEvent(userStat, gameMatch, userTeam);
		}
	}
}

void CQuestTask::OnUserLogin(CUser* user)
{
	/*for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_LOGIN)
		{
			CQuestConditionLogin* conditionLogin = dynamic_cast<CQuestConditionLogin*>(condition);
			conditionLogin->OnUserLogin(user);
		}
	}*/
}

void CQuestTask::ApplyProgress(CUser* user, UserQuestTaskProgress progress)
{
	if (g_pUserDatabase->UpdateQuestTaskProgress(user->GetID(), m_pQuest->GetID(), progress) <= 0)
		return;

	g_pPacketManager->SendQuestUpdateTaskInfo(user->GetExtendedSocket(), 0xFF, m_pQuest->GetID(), this, progress);
}

string CQuestTask::GetName()
{
	return m_szName;
}

int CQuestTask::GetID()
{
	return m_nID;
}

int CQuestTask::GetGoal()
{
	return m_nGoal;
}

CQuest* CQuestTask::GetQuest()
{
	return m_pQuest;
}

bool CQuestTask::IsFinished(CUser* user)
{
	if (!g_pUserDatabase->IsQuestTaskFinished(user->GetID(), m_pQuest->GetID(), m_nID))
		return false;

	return true;
}

CQuest::CQuest()
{
	m_nID = 0;
	m_nType = 0;
	m_nNPCID = 0;
}

CQuest::~CQuest()
{
	for (auto task : m_Tasks)
		delete task;
}

void CQuest::SetName(string name)
{
	m_szName = name;
}

string CQuest::GetName()
{
	return m_szName;
}

void CQuest::SetDescription(string desc)
{
	m_szDescription = desc;
}

string CQuest::GetDescription()
{
	return m_szDescription;
}

void CQuest::SetID(int id)
{
	m_nID = id;
}

int CQuest::GetID()
{
	return m_nID;
}

void CQuest::SetType(int type)
{
	m_nType = type;
}

int CQuest::GetType()
{
	return m_nType;
}

void CQuest::SetNPCID(int npcID)
{
	m_nNPCID = npcID;
}

int CQuest::GetNPCID()
{
	return m_nNPCID;
}

void CQuest::AddTask(CQuestTask* task)
{
	m_Tasks.push_back(task);
}

void CQuest::AddReward(QuestReward_s& reward)
{
	m_Rewards.push_back(reward);
}

CQuestTask* CQuest::GetTask(int id)
{
	vector<CQuestTask*>::iterator taskIt = find_if(m_Tasks.begin(), m_Tasks.end(),
		[id](CQuestTask* task) { return id == task->GetID(); });

	if (taskIt != m_Tasks.end())
		return *taskIt;

	return NULL;
}

vector<CQuestTask*> CQuest::GetTasks()
{
	return m_Tasks;
}

vector<QuestReward_s> CQuest::GetRewards()
{
	return m_Rewards;
}

void CQuest::ApplyProgress(CUser* user, UserQuestProgress& progress)
{
	for (auto& taskProgress : progress.tasks)
	{
		CQuestTask* task = GetTask(taskProgress.taskID);
		if (task && task->GetGoal() > taskProgress.unitsDone) // skip finished tasks
		{
			task->ApplyProgress(user, taskProgress);
		}
	}
}

void CQuest::OnMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnMinuteTick(userStat, gameMatch);
	}
}

void CQuest::OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent& killEvent)
{
	for (auto task : m_Tasks)
	{
		task->OnKillEvent(userStat, gameMatch, killEvent);
	}
}

void CQuest::OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnBombExplode(userStat, gameMatch);
	}
}

void CQuest::OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnBombDefuse(userStat, gameMatch);
	}
}

void CQuest::OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnHostageEscape(userStat, gameMatch);
	}
}

void CQuest::OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType)
{
	for (auto task : m_Tasks)
	{
		task->OnMonsterKill(userStat, gameMatch, monsterType);
	}
}

void CQuest::OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnMosquitoKill(userStat, gameMatch);
	}
}

void CQuest::OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnKiteKill(userStat, gameMatch);
	}
}

void CQuest::OnLevelUpEvent(CUser* user, int level, int newLevel)
{
	for (auto task : m_Tasks)
	{
		task->OnLevelUpEvent(user, level, newLevel);
	}
}

void CQuest::OnGameMatchLeave(CUser* user)
{

}

void CQuest::OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam)
{
	for (auto task : m_Tasks)
	{
		task->OnMatchEndEvent(userStat, gameMatch, userTeam);
	}
}

void CQuest::OnTaskDone(CUser* user, UserQuestTaskProgress& taskProgress, CQuestTask* doneTask)
{
	g_pQuestManager->OnQuestTaskFinished(user, taskProgress, doneTask, this);
}

bool CQuest::IsAllTaskFinished(CUser* user)
{
	for (auto task : m_Tasks)
	{
		if (!task->IsFinished(user))
			return false;
	}

	return true;
}