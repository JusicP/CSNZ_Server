#include "logger.h"

#include <time.h>

#ifdef _WIN32
#include <direct.h> // getcwd
#else
#include <stdio.h>
#define MAX_PATH FILENAME_MAX
#endif

#ifdef USE_GUI
#include "gui/igui.h"
#endif

#define MAX_DATE_LEN 9
#define MAX_TIME_PREFIX_LEN 21

enum TextColor
{
	CON_COLOR_WHITE = 1,
	CON_COLOR_YELLOW = 2,
	CON_COLOR_RED = 3,
	CON_COLOR_GREEN = 4,
	CON_COLOR_CYAN = 5
};

using namespace std;

// default logger
CBaseLogger& Logger()
{
	static CLoggerPrefix logger(new CCompositeLogger(true, {
		new CConsoleLogger() }
	));

	return logger;
}

void AddLogger(ILogger* logger)
{
	CLoggerPrefix* prefixLogger = static_cast<CLoggerPrefix*>(&Logger());
	CCompositeLogger* compositeLogger = static_cast<CCompositeLogger*>(prefixLogger->GetLogger());

	compositeLogger->AddLogger(logger);
}

/**
 * @param loggers Loggers list 
 */
CCompositeLogger::CCompositeLogger(bool criticalSection, initializer_list<ILogger*> loggers) : CBaseLogger(criticalSection), m_Loggers{loggers}
{
}

CCompositeLogger::~CCompositeLogger()
{
	for (auto logger : m_Loggers)
		delete logger;
}

void CCompositeLogger::AddLogger(ILogger* logger)
{
	m_Loggers.push_back(logger);
}

void CCompositeLogger::RemoveLogger(ILogger* logger)
{
	m_Loggers.erase(remove(m_Loggers.begin(), m_Loggers.end(), logger), m_Loggers.end());
}

void CCompositeLogger::LogVarg(int level, const char* msg, va_list argptr)
{
	// format message only once
	vsnprintf(m_szBuf, sizeof(m_szBuf), msg, argptr);

	for (auto logger : m_Loggers)
	{
		// don't pass variadic arguments, because message is already formatted
		logger->LogVarg(level, m_szBuf, NULL);
	}
}

CLoggerPrefix::CLoggerPrefix(ILogger* logger) : CBaseLogger(false)
{
	m_pLogger = logger;
}

CLoggerPrefix::~CLoggerPrefix()
{
	delete m_pLogger;
}

void CLoggerPrefix::LogVarg(int level, const char* msg, va_list argptr)
{
	const char* msgWithPrefix = FormatPrefix(level, msg);

	m_pLogger->LogVarg(level, msgWithPrefix, argptr);
}

/**
 * Adds prefix to message in format "[time][level] Message"
 */
const char* CLoggerPrefix::FormatPrefix(int level, const char* msg)
{
	char timePrefix[MAX_TIME_PREFIX_LEN];
	GetCurrTimePrefix(timePrefix, sizeof(timePrefix));

	int levelLen = 0;
	const char* levelStr = GetLevelPrefix(level, levelLen);
	int indent = GetLevelPrefixMaxLen() - levelLen; // this is how much extra blank characters need to be added for indent

	snprintf(m_szBuf, sizeof(m_szBuf), "[%s][%s]%*s %s", timePrefix, levelStr, indent, "", msg);

	return m_szBuf;
}

void CLoggerPrefix::GetCurrTimePrefix(char* timePrefix, int prefixLen)
{
	char szTime[MAX_DATE_LEN];
	char szDate[MAX_DATE_LEN];

#ifdef _WIN32
	_strtime_s(szTime);
	_strdate_s(szDate);
#else
	time_t myTime = time(NULL);
	strftime(szTime, MAX_DATE_LEN, "%T", localtime(&myTime));
	strftime(szDate, MAX_DATE_LEN, "%D", localtime(&myTime));
#endif

	snprintf(timePrefix, prefixLen, "%s %s", szDate, szTime);
}

const char* CLoggerPrefix::GetLevelPrefix(int level, int& levelLen)
{
	switch (level)
	{
	case LOG_LEVEL_INFO:
		levelLen = sizeof("INFO");
		return "INFO";
	case LOG_LEVEL_WARN:
		levelLen = sizeof("WARN");
		return "WARN";
	case LOG_LEVEL_ERROR:
		levelLen = sizeof("ERROR");
		return "ERROR";
	case LOG_LEVEL_FATAL_ERROR:
		levelLen = sizeof("FATAL");
		return "FATAL";
	case LOG_LEVEL_DEBUG:
		levelLen = sizeof("DEBUG");
		return "DEBUG";
	default:
		levelLen = 0;
		return "";
	}
}

/**
 * Get maximum length of level prefix strings for indentation, calculated only once
 */
int CLoggerPrefix::GetLevelPrefixMaxLen()
{
	static int maxLevelStrLen = 0;
	if (maxLevelStrLen)
		return maxLevelStrLen;

	int levelLen = 0;
	for (int i = LOG_LEVEL_INFO; i != LOG_LEVEL_END; i *= 2)
	{
		GetLevelPrefix(i, levelLen);
		if (!levelLen)
			break;

		if (levelLen > maxLevelStrLen)
			maxLevelStrLen = levelLen;
	}

	return maxLevelStrLen;
}

ILogger* CLoggerPrefix::GetLogger()
{
	return m_pLogger;
}

void CConsoleLogger::LogVarg(int level, const char* msg, va_list argptr)
{
	TextColor color;
	switch (level)
	{
	case LOG_LEVEL_INFO:
	case LOG_LEVEL_DEBUG:
		color = CON_COLOR_WHITE;
		break;
	case LOG_LEVEL_WARN:
		color = CON_COLOR_YELLOW;
		break;
	case LOG_LEVEL_ERROR:
	case LOG_LEVEL_FATAL_ERROR:
		color = CON_COLOR_RED;
		break;
	default:
		color = CON_COLOR_WHITE;
		break;
	}

	SetTextColor(color);

	vprintf(msg, argptr);

	SetTextColor(CON_COLOR_WHITE); // reset color to default

	if (level == LOG_LEVEL_FATAL_ERROR)
	{
#ifdef USE_GUI
		GUI()->ShowMessageBox("Critical error", msg, true);
#elif _WIN32
		MessageBox(NULL, msg, "Critical error", MB_ICONERROR);
#endif
	}
}

void CConsoleLogger::SetTextColor(int color)
{
	switch (color)
	{
	case CON_COLOR_YELLOW:
		printf("\033[0;33m");
		break;
	case CON_COLOR_RED:
		printf("\033[0;31m");
		break;
	case CON_COLOR_WHITE:
		printf("\033[0m");
		break;
	case CON_COLOR_GREEN:
		printf("\033[0;32m");
		break;
	case CON_COLOR_CYAN:
		printf("\033[0;36m");
		break;
	default:
		printf("\033[0m");
		break;
	}
}

/**
 * Prepares log file path and file name. Log path format: <working directory>/Logs/<filename>_<date>.log
 * @param filename File name
 */
CFileLogger::CFileLogger(const char* filename)
{
	// format path to log
	char cwd[MAX_PATH];
	if (!getcwd(cwd, sizeof(cwd)))
	{
		printf("getcwd failed: %s\n", strerror(errno));
	}

	char path[MAX_PATH];
	snprintf(path, sizeof(path), "%s/Logs", cwd);

#ifdef _WIN32
	_mkdir(path);
#else 
	mkdir(path, 0755);
#endif

	time_t currTime = time(NULL);
	tm* pTime = localtime(&currTime);

	snprintf(m_szLogPath, sizeof(m_szLogPath), "%s/%s_%d-%.2d-%.2d %.2d-%.2d-%.2d.log", path, filename,
		pTime->tm_year + 1900,
		pTime->tm_mon + 1,
		pTime->tm_mday,
		pTime->tm_hour,
		pTime->tm_min,
		pTime->tm_sec
	);
}

void CFileLogger::LogVarg(int level, const char* msg, va_list argptr)
{
	FILE* file = fopen(m_szLogPath, "a+");
	if (file)
	{
		vfprintf(file, msg, argptr);

		fclose(file);
	}
}

void CGUILogger::LogVarg(int level, const char* msg, va_list argptr)
{
#ifdef USE_GUI
	GUI()->LogMessage(level, msg);
#endif
}