#pragma once

#include "igui.h"
#include "mainwindow.h"
#undef slots // undef keyword to avoid conflict 
//#include "IUserManager.h"
//#include "IUserDatabase.h"
#define slots Q_SLOTS

#include <QApplication>

class CGUI : public IGUI
{
public:
	CGUI();
	virtual ~CGUI();
	
	virtual bool Init(IManager* mgr, IEvents* events);
	virtual bool PostInit(IServerInstance* srv);
	virtual void Shutdown();
	virtual void Exec();

	// thread safe methods to update GUI
	virtual void Exit();
	virtual void LogMessage(int level, const std::string& msg);
	virtual void UpdateInfo(int status, int totalConnections, int uptime, double memoryUsage);
	virtual void ShowMessageBox(const std::string& title, const std::string& msg, bool fatalError = false);
	virtual void ShowMainWindow();
	virtual void OnSessionListUpdated(const std::vector<Session>& sessions);
	virtual void OnCommandListUpdated(const std::vector<std::string>& cmdList);

private:
	QApplication* m_pApplication;
	CMainWindow* m_pMainWindow;
};

extern IEvents* g_pEvents;
extern IManager* g_pManager;
extern IServerInstance* g_pServerInstance;
extern class IPacketManager* g_pPacketManager;
extern class IUserManager* g_pUserManager;
extern class IUserDatabase* g_pUserDatabase;