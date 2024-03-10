#include "console.h"
#ifdef USE_GUI
#include "gui/igui.h"
#endif

#define MAX_DATE_LEN 9

using namespace std;

CConsole& Console()
{
	static CConsole g_Console;
	return g_Console;
}

CConsole::CConsole() : m_szLogPath {}
{
#ifdef WIN32
	m_Output = GetStdHandle(STD_OUTPUT_HANDLE);
	printf("\n");
#endif

	m_nLogLevel = LOG_INFO | LOG_WARN | LOG_ERROR;
	m_szLastPacket = "none";
}

CConsole::~CConsole()
{
	SetTextColor(CON_CYAN);
	printf("Console destroyed\n");
	SetTextColor(CON_WHITE);
}

const char* CConsole::GetCurrTime()
{
	static char buf[21]; // 32 should be enough
	char szTime[MAX_DATE_LEN];
	char szDate[MAX_DATE_LEN];

#ifdef WIN32
	_strtime_s(szTime);
	_strdate_s(szDate);
#else
	time_t myTime = time(NULL);
	strftime(szTime, MAX_DATE_LEN, "%T", localtime(&myTime));
	strftime(szDate, MAX_DATE_LEN, "%D", localtime(&myTime));
#endif

	sprintf(buf, "[%s %s]", szDate, szTime);

	return buf;
}

void CConsole::SetTextColor(TextColor color)
{
#ifdef WIN32
	SetConsoleTextAttribute(m_Output, color);
#else
	switch (color)
	{
	case CON_YELLOW:
		printf("\033[0;33m");
		break;
	case CON_RED:
		printf("\033[0;31m");
		break;
	case CON_WHITE:
		printf("\033[0;37m");
		break;
	case CON_GREEN:
		printf("\033[0;32m");
		break;
	case CON_CYAN:
		printf("\033[0;36m");
		break;
	}
#endif
}

void CConsole::Write(OutMode mode, const char* msg)
{
	TextColor color;
	switch (mode)
	{
	case CON_WARNING:
		color = CON_YELLOW;
		break;
	case CON_ERROR:
	case CON_FATAL_ERROR:
		color = CON_RED;
		break;
	case CON_LOG:
		color = CON_WHITE;
		break;
	default:
		color = CON_WHITE;
		break;
	}

	SetTextColor(color);
	WriteToConsole(mode, msg);
	SetTextColor(CON_WHITE); //восстанавливаем цвет
}

void CConsole::WriteToConsole(OutMode mode, const char* msg)
{
	static char buffer[8192];
	snprintf(buffer, sizeof(buffer), "%s %s", GetCurrTime(), msg);

	// write to console
	printf("%s", buffer);

	FILE* file = fopen(m_szLogPath, "a+");
	if (file)
	{
		fprintf(file, "%s", buffer);
		fclose(file);
	}

#ifdef USE_GUI
	GUI()->LogMessage(mode, buffer);
#endif

	if (mode == CON_FATAL_ERROR)
	{
#ifdef USE_GUI
		GUI()->ShowMessageBox("Critical error", msg, true);
#elif WIN32
		MessageBox(NULL, msg, "Critical error", MB_ICONERROR);
#endif
	}

}

void CConsole::Error(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_ERROR) == 0)
	{
		return;
	}

	m_CriticalSection.Enter();

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_ERROR, buffer);

	m_CriticalSection.Leave();
}

void CConsole::FatalError(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_ERROR) == 0)
	{
		return;
	}

	m_CriticalSection.Enter();

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_FATAL_ERROR, buffer);

	m_CriticalSection.Leave();
}

void CConsole::Warn(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_WARN) == 0)
	{
		return;
	}

	m_CriticalSection.Enter();

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_WARNING, buffer);

	m_CriticalSection.Leave();
}

void CConsole::Log(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_INFO) == 0)
	{
		return;
	}

	m_CriticalSection.Enter();

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_LOG, buffer);

	m_CriticalSection.Leave();
}

void CConsole::Debug(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_DEBUG_PACKET) == 0)
	{
		return;
	}

	m_CriticalSection.Enter();

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_LOG, buffer);

	m_CriticalSection.Leave();
}

void CConsole::SetStatus(const char* status)
{
#ifdef WIN32
	strncpy(m_szStatusLine, status, sizeof(m_szStatusLine) - 1);
	m_szStatusLine[sizeof(m_szStatusLine) - 2] = '\0';
	UpdateStatus();
#endif
}

void CConsole::SetLogLevel(int logLevel)
{
	m_nLogLevel = logLevel;
}

void CConsole::SetLastPacket(const char* name)
{
	m_szLastPacket = name;
}

const char* CConsole::GetLastPacket()
{
	return m_szLastPacket;
}

void CConsole::UpdateStatus()
{
#ifdef WIN32
	COORD coord;
	DWORD dwWritten = 0;
	WORD wAttrib[80];

	for (int i = 0; i < 80; i++)
	{
		wAttrib[i] = FOREGROUND_GREEN; // FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;
	}

	coord.X = coord.Y = 0;

	WriteConsoleOutputAttribute(m_Output, wAttrib, 80, coord, &dwWritten);
	WriteConsoleOutputCharacter(m_Output, m_szStatusLine, 80, coord, &dwWritten);
#endif
}

void CConsole::SetLogFile(const char* path)
{
	strncpy(m_szLogPath, path, sizeof(m_szLogPath) - 1);
	m_szLogPath[sizeof(m_szLogPath) - 1] = '\0';
}
