#include "QuestEvent.h"

using namespace std;

CQuestEventBaseCondition::CQuestEventBaseCondition(CQuestEventTask* task, int id, int goal)
{
	m_pTask = task;
	m_nID = id;
	m_nGoalPoints = goal;
}

bool CQuestEventBaseCondition::Event_Internal(CUser* user)
{
	UserQuestTaskProgress progress = {};
	if (g_pUserDatabase->GetQuestEventTaskProgress(user->GetID(), m_pTask->GetQuest()->GetID(), m_pTask->GetID(), progress) <= 0)
		return false;

	if (progress.unitsDone >= m_pTask->GetGoal())
		return false;

	return true;
}

void CQuestEventBaseCondition::SetEventType(int eventType)
{
	m_nEventType = eventType;
}

int CQuestEventBaseCondition::GetEventType()
{
	return m_nEventType;
}

CQuestEventTask::CQuestEventTask(CQuestEvent* quest, int id, int goal)
{
	m_pQuest = quest;
	m_nID = id;
	m_nGoal = goal;
	m_nRewardID = 0;
	m_nNoticeGoal = 0;
}

void CQuestEventTask::Event()
{
	//Event_Internal();
}

bool CQuestEventTask::Event_Internal(CUser* user)
{
	UserQuestTaskProgress progress = {};
	if (g_pUserDatabase->GetQuestEventTaskProgress(user->GetID(), m_pQuest->GetID(), m_nID, progress) <= 0)
		return false;

	if (progress.unitsDone >= m_nGoal)
		return false;

	return true;
}

void CQuestEventTask::IncrementCount(CUser* user, int count, bool setForce)
{
	UserQuestTaskProgress progress = {};
	if (g_pUserDatabase->GetQuestEventTaskProgress(user->GetID(), m_pQuest->GetID(), m_nID, progress) <= 0)
		return;

	if (count < 0)
		count = 1;

	// lol
	count ? setForce ? progress.unitsDone += count : progress.unitsDone = count : progress.unitsDone++;
	if (progress.unitsDone >= m_nGoal)
	{
		progress.unitsDone = m_nGoal;
		progress.finished = true;
	}

	if (g_pUserDatabase->UpdateQuestEventTaskProgress(user->GetID(), m_pQuest->GetID(), progress) > 0)
	{
#if 0
		g_pConsole->Log(LOG_USER, CON_LOG, "[User '%s'] CQuestEventTask::IncrementCount: quest name: %s, done: %d, goal: %d\n", user->GetLogName(), m_pQuest->GetName().c_str(), progress.unitsDone, m_nGoal);
#endif
		if (progress.unitsDone >= m_nGoal)
		{
			Done(user, progress);
		}
	}
}

void CQuestEventTask::Done(CUser* user, UserQuestTaskProgress& progress)
{
	m_pQuest->OnTaskDone(user, progress, this);
}

void CQuestEventTask::SetNotice(int goal, string userMsg)
{
	m_nNoticeGoal = goal;
	m_szNoticeUserMsg = userMsg;
}

void CQuestEventTask::SetRewardID(int rewardID)
{
	m_nRewardID = rewardID;
}

void CQuestEventTask::AddCondition(CQuestEventBaseCondition* condition)
{
	m_Conditions.push_back(condition);
}

void CQuestEventTask::ApplyProgress(CUser* user, UserQuestTaskProgress& progress)
{
	if (g_pUserDatabase->UpdateQuestEventTaskProgress(user->GetID(), m_pQuest->GetID(), progress) <= 0)
		return;
}

void CQuestEventTask::OnMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_TIMEMATCH)
		{
			CQuestEventConditionTimeMatch* conditionTime = dynamic_cast<CQuestEventConditionTimeMatch*>(condition);
			conditionTime->OnMinuteTick(userStat, gameMatch);
		}
	}
}

void CQuestEventTask::OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent& killEvent)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILL)
		{
			CQuestEventConditionKill* conditionKill = dynamic_cast<CQuestEventConditionKill*>(condition);
			conditionKill->OnKillEvent(userStat, gameMatch, killEvent);
		}
	}
}

void CQuestEventTask::OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_BOMBEXPLODE)
		{
			CQuestEventConditionBombExplode* conditionExplode = dynamic_cast<CQuestEventConditionBombExplode*>(condition);
			conditionExplode->OnBombExplode(userStat, gameMatch);
		}
	}
}

void CQuestEventTask::OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_BOMBDEFUSE)
		{
			CQuestEventConditionBombDefuse* conditionDefuse = dynamic_cast<CQuestEventConditionBombDefuse*>(condition);
			conditionDefuse->OnBombDefuse(userStat, gameMatch);
		}
	}
}

void CQuestEventTask::OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_HOSTAGEESCAPE)
		{
			CQuestEventConditionHostageEscape* conditionHostageEscape = dynamic_cast<CQuestEventConditionHostageEscape*>(condition);
			conditionHostageEscape->OnHostageEscape(userStat, gameMatch);
		}
	}
}

void CQuestEventTask::OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILLMONSTER)
		{
			CQuestEventConditionKillMonster* conditionKillMonster = dynamic_cast<CQuestEventConditionKillMonster*>(condition);
			conditionKillMonster->OnMonsterKill(userStat, gameMatch, monsterType);
		}
	}
}

void CQuestEventTask::OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILLMOSQUITO)
		{
			CQuestEventConditionKillMosquito* conditionKillMosquito = dynamic_cast<CQuestEventConditionKillMosquito*>(condition);
			conditionKillMosquito->OnMosquitoKill(userStat, gameMatch);
		}
	}
}

void CQuestEventTask::OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_KILLKITE)
		{
			CQuestEventConditionKillKite* conditionKillKite = dynamic_cast<CQuestEventConditionKillKite*>(condition);
			conditionKillKite->OnKiteKill(userStat, gameMatch);
		}
	}
}

void CQuestEventTask::OnLevelUpEvent(CUser* user, int level, int newLevel)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_LEVELUP)
		{
			CQuestEventConditionLevelUp* conditionLevelUP = dynamic_cast<CQuestEventConditionLevelUp*>(condition);
			conditionLevelUP->OnLevelUpEvent(user, level, newLevel);
		}
	}
}

void CQuestEventTask::OnGameMatchLeave(CUser* user)
{

}

void CQuestEventTask::OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_MATCHWIN)
		{
			CQuestEventConditionWin* conditionMatchWin = dynamic_cast<CQuestEventConditionWin*>(condition);
			conditionMatchWin->OnMatchEndEvent(userStat, gameMatch, userTeam);
		}
	}
}

void CQuestEventTask::OnUserLogin(CUser* user)
{
	for (auto condition : m_Conditions)
	{
		if (condition->GetEventType() == QuestTaskEventType::EVENT_LOGIN)
		{
			CQuestEventConditionLogin* conditionLogin = dynamic_cast<CQuestEventConditionLogin*>(condition);
			conditionLogin->OnUserLogin(user);
		}
	}
}

/*std::string CQuestEventBaseTask::GetName()
{
	return m_szName;
}*/

int CQuestEventTask::GetID()
{
	return m_nID;
}

int CQuestEventTask::GetGoal()
{
	return m_nGoal;
}

int CQuestEventTask::GetNoticeGoal()
{
	return m_nNoticeGoal;
}

std::string CQuestEventTask::GetNoticeUserMsg()
{
	return m_szNoticeUserMsg;
}

int CQuestEventTask::GetRewardID()
{
	return m_nRewardID;
}

CQuestEvent* CQuestEventTask::GetQuest()
{
	return m_pQuest;
}

bool CQuestEventTask::IsFinished(CUser* user)
{
	if (!g_pUserDatabase->IsQuestEventTaskFinished(user->GetID(), m_pQuest->GetID(), m_nID))
		return false;

	return true;
}

CQuestEvent::CQuestEvent()
{
	m_nID = 0;
}

CQuestEvent::~CQuestEvent()
{
	for (auto task : m_Tasks)
	{
		delete task;
	}
}

void CQuestEvent::SetID(int id)
{
	m_nID = id;
}

int CQuestEvent::GetID()
{
	return m_nID;
}

void CQuestEvent::AddTask(CQuestEventTask* task)
{
	m_Tasks.push_back(task);
}

CQuestEventTask* CQuestEvent::GetTask(int id)
{
	vector<CQuestEventTask*>::iterator taskIt = find_if(m_Tasks.begin(), m_Tasks.end(),
		[id](CQuestEventTask* task) { return id == task->GetID(); });

	if (taskIt != m_Tasks.end())
		return *taskIt;

	return NULL;
}

vector<CQuestEventTask*>& CQuestEvent::GetTasks()
{
	return m_Tasks;
}

void CQuestEvent::ApplyProgress(CUser* user, UserQuestProgress& progress)
{
	for (auto& taskProgress : progress.tasks)
	{
		CQuestEventTask* task = GetTask(taskProgress.taskID);
		if (task && !taskProgress.finished) // skip finished tasks
		{
			task->ApplyProgress(user, taskProgress);
		}
	}
}

void CQuestEvent::OnMinuteTick(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnMinuteTick(userStat, gameMatch);
	}
}

void CQuestEvent::OnKillEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, GameMatch_KillEvent& killEvent)
{
	for (auto task : m_Tasks)
	{
		task->OnKillEvent(userStat, gameMatch, killEvent);
	}
}

void CQuestEvent::OnBombExplode(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnBombExplode(userStat, gameMatch);
	}
}

void CQuestEvent::OnBombDefuse(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnBombDefuse(userStat, gameMatch);
	}
}

void CQuestEvent::OnHostageEscape(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnHostageEscape(userStat, gameMatch);
	}
}

void CQuestEvent::OnMonsterKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int monsterType)
{
	for (auto task : m_Tasks)
	{
		task->OnMonsterKill(userStat, gameMatch, monsterType);
	}
}

void CQuestEvent::OnMosquitoKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnMosquitoKill(userStat, gameMatch);
	}
}

void CQuestEvent::OnKiteKill(CGameMatchUserStat* userStat, CGameMatch* gameMatch)
{
	for (auto task : m_Tasks)
	{
		task->OnKiteKill(userStat, gameMatch);
	}
}

void CQuestEvent::OnLevelUpEvent(CUser* user, int level, int newLevel)
{
	for (auto task : m_Tasks)
	{
		task->OnLevelUpEvent(user, level, newLevel);
	}
}

void CQuestEvent::OnGameMatchLeave(CUser* user)
{

}

void CQuestEvent::OnMatchEndEvent(CGameMatchUserStat* userStat, CGameMatch* gameMatch, int userTeam)
{
	for (auto task : m_Tasks)
	{
		task->OnMatchEndEvent(userStat, gameMatch, userTeam);
	}
}

void CQuestEvent::OnUserLogin(CUser* user)
{
	for (auto task : m_Tasks)
	{
		task->OnUserLogin(user);
	}
}

void CQuestEvent::OnTaskDone(CUser* user, UserQuestTaskProgress& taskProgress, CQuestEventTask* doneTask)
{
	g_pQuestManager->OnQuestEventTaskFinished(user, taskProgress, doneTask, this);
}

bool CQuestEvent::IsAllTaskFinished(CUser* user)
{
	for (auto task : m_Tasks)
	{
		if (!task->IsFinished(user))
			return false;
	}

	return true;
}