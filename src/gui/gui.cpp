#include "gui.h"
#include "interface/imanager.h"

static CGUI g_GUI;
IGUI* g_pGUI = NULL;

IEvents* g_pEvents = NULL;
IManager* g_pManager = NULL;
IServerInstance* g_pServerInstance = NULL;
IUserManager* g_pUserManager = NULL;
IUserDatabase* g_pUserDatabase = NULL;
IPacketManager* g_pPacketManager = NULL;

CGUI::CGUI()
{
	g_pGUI = this;
	m_pApplication = NULL;
	m_pMainWindow = NULL;
}

CGUI::~CGUI()
{
	g_pGUI = NULL;
	Shutdown();
}

bool CGUI::Init(IManager* mgr, IEvents* events)
{
	g_pManager = mgr;
	g_pEvents = events;

	int argc = 0;
	m_pApplication = new QApplication(argc, NULL);
	m_pMainWindow = new CMainWindow();

	return true;
}

bool CGUI::PostInit(IServerInstance* srv)
{
	g_pServerInstance = srv;
	if (!g_pServerInstance)
	{
		ShowMessageBox("Fatal Error", "Could not get ServerInstance interface", true);
		return false;
	}

	g_pUserManager = (IUserManager*)g_pManager->GetManager("UserManager");
	if (!g_pUserManager)
	{
		ShowMessageBox("Fatal Error", "Could not get UserManager interface", true);
		return false;
	}

	g_pUserDatabase = (IUserDatabase*)g_pManager->GetManager("UserDatabase");
	if (!g_pUserDatabase)
	{
		ShowMessageBox("Fatal Error", "Could not get UserDatabase interface", true);
		return false;
	}

	g_pPacketManager = (IPacketManager*)g_pManager->GetManager("PacketManager");
	if (!g_pPacketManager)
	{
		ShowMessageBox("Fatal Error", "Could not get PacketManager interface", true);
		return false;
	}

	return true;
}

void CGUI::Shutdown()
{
	delete m_pMainWindow;
	m_pMainWindow = NULL;
	delete m_pApplication;
	m_pApplication = NULL;
}

void CGUI::Exec()
{
	int result = m_pApplication->exec();
}

void CGUI::Exit()
{
	QMetaObject::invokeMethod(m_pApplication, "exit");
}

void CGUI::LogMessage(int level, const std::string& msg)
{
	if (m_pMainWindow)
		QMetaObject::invokeMethod(m_pMainWindow->GetConsoleTab(), "Log", Q_ARG(int, level), Q_ARG(const std::string&, msg));
}

void CGUI::UpdateInfo(int status, int totalConnections, int uptime, double memoryUsage)
{
	if (m_pMainWindow)
		QMetaObject::invokeMethod(m_pMainWindow->GetMainTab(), "UpdateInfo", Q_ARG(int, status), Q_ARG(int, totalConnections), Q_ARG(int, uptime), Q_ARG(double, memoryUsage));
}

void CGUI::ShowMessageBox(const std::string& title, const std::string& msg, bool fatalError)
{
	if (m_pMainWindow)
		QMetaObject::invokeMethod(m_pMainWindow, "ShowMessageBox", Q_ARG(const std::string&, title), Q_ARG(const std::string&, msg), Q_ARG(bool, fatalError));
}

void CGUI::ShowMainWindow()
{
	if (m_pMainWindow)
		QMetaObject::invokeMethod(m_pMainWindow, "show");
}

void CGUI::OnSessionListUpdated(const std::vector<Session>& sessions)
{
	if (m_pMainWindow)
		QMetaObject::invokeMethod(m_pMainWindow->GetSessionTab(), "OnSessionListUpdated", Q_ARG(const std::vector<Session>&, sessions));
}

void CGUI::OnCommandListUpdated(const std::vector<std::string>& cmdList)
{
	if (m_pMainWindow)
		QMetaObject::invokeMethod(m_pMainWindow->GetConsoleTab(), "OnCommandListUpdated", Q_ARG(const std::vector<std::string>&, cmdList));
}