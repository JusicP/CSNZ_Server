#include "main.h"
#ifdef WIN32
#include "crashdump.h"
#endif
#ifdef USE_GUI
#include "gui/igui.h"
#endif
#include "manager/manager.h"
#include "common/utils.h"

CServerInstance* g_pServerInstance;

CEvent g_Event;
CCriticalSection g_ServerCriticalSection;

#ifdef WIN32
void invalid_parameter_function(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	printf("invalid_parameter_function called\n");

	Console().Log(OBFUSCATE("%ls, %ls, %ls, %d, %p\n"), expression, function, file, line, pReserved);
}

BOOL WINAPI CtrlHandler(DWORD ctrlType)
{
	if (ctrlType == CTRL_CLOSE_EVENT)
	{
		g_pServerInstance->SetServerActive(false);

		ExitThread(0); // hack to ignore close event
		return TRUE;
	}

	return FALSE;
}
#endif

#ifdef USE_GUI
CObjectSync g_GUIInitEvent;

void* GUIThread(void*)
{
	if (!GUI()->Init(&Manager(), &g_Event))
	{
		printf("error!\n");
	}
	
	g_GUIInitEvent.Signal();

	GUI()->Exec();
	GUI()->Shutdown();

	// shutdown the server after closing gui
	g_ServerCriticalSection.Enter();

	g_pServerInstance->SetServerActive(false);

	g_ServerCriticalSection.Leave();
	
	return NULL;
}
#endif

int main(int argc, char* argv[])
{
#ifdef WIN32
	SetUnhandledExceptionFilter(ExceptionFilter);
	_set_invalid_parameter_handler(invalid_parameter_function);
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

#ifdef USE_GUI
	// hide console when using gui
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
#endif

#ifdef USE_GUI
	CThread qtThread(GUIThread);
	qtThread.Start();

	// wait for gui init before we init the server
	g_GUIInitEvent.WaitForSignal();
#endif

	g_pServerInstance = new CServerInstance();
	if (!g_pServerInstance->Init())
	{
#ifdef USE_GUI
		qtThread.Join();
#endif
		delete g_pServerInstance;
		return 1;
	}

#ifdef USE_GUI
	if (!GUI()->PostInit(g_pServerInstance))
	{
		qtThread.Join();
		delete g_pServerInstance;
		return 1;
	}

	GUI()->ShowMainWindow();
#endif

	CThread readConsoleThread(ReadConsoleThread);
	CThread eventThread(EventThread);
	readConsoleThread.Start();
	eventThread.Start();

	while (g_pServerInstance->IsServerActive())
	{
		g_Event.AddEventSecondTick();

		SleepMS(1000);
	}

#ifdef USE_GUI
	GUI()->Exit();
	qtThread.Join();
#endif

	eventThread.Join();

	g_ServerCriticalSection.Enter();

	// terminate read console thread because of std::getline (should be safe)
	readConsoleThread.Terminate();

	g_ServerCriticalSection.Leave();

	delete g_pServerInstance;

	return 0;
}
