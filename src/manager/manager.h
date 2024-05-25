#pragma once

#include <vector>
#include <string>

#include "interface/imanager.h"

/**
 * Class for controlling all managers (init/shutdown all managers at the same time, calling tick method for all managers).
 * Only one instance allowed.
 */
class CManager : public IManager
{
private:
	CManager() = default;
	CManager(const CManager&) = delete;
	CManager(CManager&&) = delete;
	CManager& operator=(const CManager&) = delete;
	CManager& operator=(CManager&&) = delete;

public:
	static CManager& GetInstance();

	bool InitAll();
	void ShutdownAll();
	bool ReloadAll();
	void AddManager(IBaseManager* pElem);
	void RemoveManager(IBaseManager* pElem);
	IBaseManager* GetManager(const std::string& name);
	void SecondTick(time_t curTime);
	void MinuteTick(time_t curTime);

private:
	bool InitAll_Multithread();

	std::vector<IBaseManager*> m_Managers;
};

extern CManager& Manager();

/**
 * Class that represents base manager. Every manager must inherit this class and every manager interface must inherit base manager interface.
 */
template<class IInterface>
class CBaseManager : public IInterface
{
public:
	CBaseManager(const std::string& name, bool secondTick = false, bool minuteTick = false)
	{
		m_Name = name;
		m_bSecondTick = secondTick;
		m_bMinuteTick = minuteTick;
		m_bCanReload = true;

		Manager().AddManager(this);
	}

	virtual ~CBaseManager()
	{
		printf("~CBaseManager called, %p\n\n", this);

		Manager().RemoveManager(this);
	}

	// stub methods
	virtual bool Init() { return true; }
	virtual void Shutdown() { printf("Shutdown called, %p\n", this); }
	virtual bool CanReload() { return m_bCanReload; }
	virtual std::string GetName() { return m_Name; }
	virtual void OnSecondTick(time_t curTime) {}
	virtual void OnMinuteTick(time_t curTime) {}
	virtual bool ShouldDoSecondTick() { return m_bSecondTick; }
	virtual bool ShouldDoMinuteTick() { return m_bMinuteTick; }

	void SetMinuteTick(bool tick) { m_bMinuteTick = tick; }
	void SetSecondTick(bool tick) { m_bSecondTick = tick; }
	void SetCanReload(bool canReload) {	m_bCanReload = canReload; }

private:
	std::string m_Name;
	bool m_bSecondTick;
	bool m_bMinuteTick;
	bool m_bCanReload;
};
