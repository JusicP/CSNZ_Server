#pragma once

#include <string>
#undef slots
#include "definitions.h"
#define slots Q_SLOTS

class IServerInstance;
class IManager;
class IEvents;

class IGUI
{
public:
	virtual bool Init(IManager* mgr, IEvents* events) = 0;
	virtual bool PostInit(IServerInstance* srv) = 0;
	virtual void Shutdown() = 0;
	virtual void Exec() = 0;

	// thread safe methods to update GUI
	virtual void Exit() = 0;
	virtual void LogMessage(int level, const std::string& msg) = 0;
	virtual void UpdateInfo(int status, int totalConnections, int uptime, double memoryUsage) = 0;
	virtual void ShowMessageBox(const std::string& title, const std::string& msg, bool fatalError = false) = 0;
	virtual void ShowMainWindow() = 0;
	virtual void OnSessionListUpdated(const std::vector<Session>& sessions) = 0;
	virtual void OnCommandListUpdated(const std::vector<std::string>& cmdList) = 0;
};

inline IGUI* GUI()
{
	extern IGUI* g_pGUI;
	return g_pGUI;
}