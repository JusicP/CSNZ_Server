#include "manager.h"
#include "common/logger.h"

#include <chrono>
#include <thread>
#include <future>

using namespace std;

/**
 * DEPRECATED! Manager singleton (the same as CManager::GetInstance())
 * @return reference to manager instance
 */
CManager& Manager()
{
	return CManager::GetInstance();
}

/**
 * Manager singleton accessor
 * @return reference to manager instance
 */
CManager& CManager::GetInstance()
{
	static CManager mgr;
	return mgr;
}

/**
 * Inits all managers
 * @return true when managers initialized successfully, false otherwise
 */
bool CManager::InitAll()
{
	return InitAll_Multithread();

	//auto t1 = chrono::high_resolution_clock::now();

	//for (auto p : m_Managers)
	//{
	//	if (!p->Init())
	//		return false;
	//}

	//auto t2 = chrono::high_resolution_clock::now();
	//Logger().Info("CManager::InitAll: %d ms\n", chrono::duration_cast<chrono::milliseconds>(t2 - t1).count());

	//return true;
}

/**
 * Inits all managers in separate threads
 * @return true when all managers initialized successfully, false otherwise
 */
bool CManager::InitAll_Multithread()
{
	//auto t1 = chrono::high_resolution_clock::now();

	// run init threads
	vector<future<bool>> results;
	for (auto manager : m_Managers)
	{
		results.emplace_back(async(
			[manager]()
			{
				return manager->Init();
			}
		));
	}

	// check if init failed
	for (auto& f : results)
	{
		if (!f.get())
			return false;
	}

	//auto t2 = chrono::high_resolution_clock::now();
	//Logger().Info("CManager::InitAll_Multithread: %d ms\n", chrono::duration_cast<chrono::milliseconds>(t2 - t1).count());

	return true;
}

/**
 * Shutdowns all managers
 */
void CManager::ShutdownAll()
{
	for (auto manager : m_Managers)
	{
		manager->Shutdown();
	}
}

/**
 * Reinitializes all managers that can be reinitialized
 * @return true when all managers init successfully or there is nothing to init, false otherwise
 */
bool CManager::ReloadAll()
{
	for (auto manager : m_Managers)
	{
		if (manager->CanReload())
		{
			manager->Shutdown();

			if (!manager->Init())
				return false;
		}
	}

	return true;
}

/**
 * Adds manager to manager list
 * @param pElem Pointer to manager
 */
void CManager::AddManager(IBaseManager* pElem)
{
	// check for duplicate by manager name
	/// @fixme: should duplicate pointers be checked?
	for (auto manager : m_Managers)
	{
		if (manager->GetName() == pElem->GetName())
		{
			Logger().Error("CManager::AddManager: %s duplicate!!\n", pElem->GetName().c_str());
			return;
		}
	}

	m_Managers.push_back(pElem);
}

/**
 * Removes manager from manager list
 * @param pElem Pointer to manager
 */
void CManager::RemoveManager(IBaseManager* pElem)
{
	m_Managers.erase(remove(m_Managers.begin(), m_Managers.end(), pElem), m_Managers.end());
}

/**
 * Gets manager from list by name
 * @param name Manager name
 * @return Pointer to manager, NULL if not found
 */
IBaseManager* CManager::GetManager(const string& name)
{
	for (auto p : m_Managers)
	{
		if (p->GetName() == name)
			return p;
	}

	return NULL;
}

/**
 * Notifies managers about second tick
 * @param curTime Time at the moment of call
 */
void CManager::SecondTick(time_t curTime)
{
	for (auto p : m_Managers)
	{
		if (p->ShouldDoSecondTick())
			p->OnSecondTick(curTime);
	}
}

/**
 * Notifies managers about minute tick
 * @param curTime Time at the moment of call
 */
void CManager::MinuteTick(time_t curTime)
{
	for (auto p : m_Managers)
	{
		if (p->ShouldDoMinuteTick())
			p->OnMinuteTick(curTime);
	}
}