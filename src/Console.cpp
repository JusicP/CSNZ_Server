#include "Console.h"
#ifdef WIN32
#include <direct.h>
#define getcwd _getcwd // avoid warning C4996
#else
#include <unistd.h>
#endif

#define MAX_DATE_LEN 9

using namespace std;

CConsole::CConsole()
{
	char cwd[MAX_PATH];
	if (!getcwd(cwd, sizeof(cwd)))
	{
		printf("getcwd failed: %s", strerror(errno));
	}

	snprintf(m_szServerLogPath, sizeof(m_szServerLogPath), "%s\\%s", cwd, "Server.log");

	FILE* file;
	fopen_s(&file, m_szServerLogPath, "w");
	fclose(file);

#ifdef WIN32
	m_Output = GetStdHandle(STD_OUTPUT_HANDLE);

	char windowName[512];
#ifdef _DEBUG
	sprintf_s(windowName, sizeof(windowName), OBFUSCATE("CSN:Z Server %s %s DEBUG BUILD"), __DATE__, __TIME__);
#else
	sprintf_s(windowName, sizeof(windowName), OBFUSCATE("CSN:Z Server %s %s"), __DATE__, __TIME__);
#endif
	SetWindowTextA(GetConsoleWindow(), windowName);
#endif

	m_nLogLevel = LOG_INFO | LOG_WARN | LOG_ERROR;
	m_szLastPacket = "none";

	printf("\n");
}

CConsole::~CConsole()
{
	SetTextColor(CON_CYAN);
	printf("Console destroyed\n");
	SetTextColor(CON_WHITE);
}

const char* CConsole::GetCurrTime()
{
	static char buf[32]; // 32 should be enough
	char szTime[MAX_DATE_LEN];
	char szDate[MAX_DATE_LEN];

#ifdef WIN32
	_strtime_s(szTime);
	_strdate_s(szDate);
#else
	time_t myTime = time(NULL);
	strftime(timeStr, MAX_DATE_LEN, "%T", localtime(&myTime));
	strftime(dateStr, MAX_DATE_LEN, "%D", localtime(&myTime));
#endif

	sprintf(buf, "[%s %s]", szDate, szTime);

	return buf;
}

void CConsole::SetTextColor(TextColor color)
{
#ifdef WIN32
	SetConsoleTextAttribute(m_Output, color);
#else
	printf("CConsole::SetTextColor: not implemented\n");
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
	WriteToConsole(msg);
	SetTextColor(CON_WHITE); //восстанавливаем цвет
}

void CConsole::WriteToConsole(const char* msg)
{
	// write to console
	printf("%s %s", GetCurrTime(), msg);

	FILE* file;
	fopen_s(&file, m_szServerLogPath, "a+");
	if (file)
	{
		fprintf(file, "%s %s", GetCurrTime(), msg);
		fclose(file);
	}
}

void CConsole::Error(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_ERROR) == 0)
	{
		return;
	}

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_ERROR, buffer);
}

void CConsole::Warn(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_WARN) == 0)
	{
		return;
	}

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_WARNING, buffer);
}

void CConsole::Log(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_INFO) == 0)
	{
		return;
	}

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_LOG, buffer);
}

void CConsole::Debug(const char* msg, ...)
{
	if ((m_nLogLevel & LOG_DEBUG_PACKET) == 0)
	{
		return;
	}

	va_list argptr;
	static char buffer[8192];

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	Write(CON_LOG, buffer);
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