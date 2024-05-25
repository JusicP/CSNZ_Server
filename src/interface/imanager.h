#pragma once

#include <string>

class IBaseManager
{
public:
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;
	virtual bool CanReload() = 0;
	virtual std::string GetName() = 0;
	virtual void OnSecondTick(time_t curTime) = 0;
	virtual void OnMinuteTick(time_t curTime) = 0;
	virtual bool ShouldDoSecondTick() = 0;
	virtual bool ShouldDoMinuteTick() = 0;
};

class IManager
{
public:
	virtual bool InitAll() = 0;
	virtual void ShutdownAll() = 0;
	virtual bool ReloadAll() = 0;
	virtual void AddManager(IBaseManager* pElem) = 0;
	virtual void RemoveManager(IBaseManager* pElem) = 0;
	virtual IBaseManager* GetManager(const std::string& name) = 0;
	virtual void SecondTick(time_t curTime) = 0;
	virtual void MinuteTick(time_t curTime) = 0;
};