#pragma once

#include <string>
#include <vector>

#include "thread.h"

enum LogLevel
{
	LOG_LEVEL_INFO = 1 << 0,
	LOG_LEVEL_WARN = 1 << 1,
	LOG_LEVEL_ERROR = 1 << 2,
	LOG_LEVEL_FATAL_ERROR = 1 << 3,
	LOG_LEVEL_DEBUG = 1 << 4,
};

/**
 * Logger interface
 */
class ILogger
{
public:
	virtual void Log(int level, const char* msg, ...) = 0;
	virtual void LogVarg(int level, const char* msg, va_list arg) = 0;
};

/**
 * Base logger class.
 * Implements Log() method with variadic arguments, which calls LogVarg() method
 */
class CBaseLogger : public ILogger
{
public:
	CBaseLogger(bool criticalSection = false)
	{
		if (criticalSection)
			m_pCriticalSection = new CCriticalSection();
		else
			m_pCriticalSection = NULL;
	}

	~CBaseLogger()
	{
		if (m_pCriticalSection)
			delete m_pCriticalSection;
	}

	virtual void Log(int level, const char* msg, ...)
	{
		va_list valist;
		va_start(valist, msg);

		if (m_pCriticalSection)
			m_pCriticalSection->Enter();

		LogVarg(level, msg, valist);

		if (m_pCriticalSection)
			m_pCriticalSection->Leave();

		va_end(valist);
	}

	// maybe use macro?
	template<typename... Args>
	void Info(const char* msg, const Args&... args)
	{
		Log(LOG_LEVEL_INFO, msg, args...);
	}

	template<typename... Args>
	void Warn(const char* msg, const Args&... args)
	{
		Log(LOG_LEVEL_WARN, msg, args...);
	}

	template<typename... Args>
	void Error(const char* msg, const Args&... args)
	{
		Log(LOG_LEVEL_ERROR, msg, args...);
	}

	template<typename... Args>
	void Fatal(const char* msg, const Args&... args)
	{
		Log(LOG_LEVEL_FATAL_ERROR, msg, args...);
	}

	template<typename... Args>
	void Debug(const char* msg, const Args&... args)
	{
		Log(LOG_LEVEL_DEBUG, msg, args...);
	}

private:
	CCriticalSection* m_pCriticalSection;
};

// default logger
CBaseLogger& Logger();
void AddLogger(ILogger* logger);

/**
 * Composite logger class.
 * This class controls the lifetime of loggers.
 */
class CCompositeLogger : public CBaseLogger
{
public:
	CCompositeLogger(bool criticalSection, std::initializer_list<ILogger*> loggers);
	~CCompositeLogger();
	
	void AddLogger(ILogger* logger);
	void RemoveLogger(ILogger* logger);
	
	virtual void LogVarg(int level, const char* msg, va_list argptr);

private:
	char m_szBuf[8192]; // buffer for formatted message
	std::vector<ILogger*> m_Loggers;
};

/**
 * Decorator for logger that adds prefix before message (time, log level).
 * Controls the lifetime of logger.
 */
class CLoggerPrefix : public CBaseLogger
{
public:
	CLoggerPrefix(ILogger* logger);
	~CLoggerPrefix();

	virtual void LogVarg(int level, const char* msg, va_list argptr);

	const char* FormatPrefix(int level, const char* msg);
	static void GetCurrTimePrefix(char* timePrefix, int prefixLen);
	static const char* GetLevelPrefix(int level);

	ILogger* GetLogger();

private:
	char m_szBuf[8192]; // buffer for message with prefix
	ILogger* m_pLogger;
};

/**
 * Class for printing log to console.
 */
class CConsoleLogger : public CBaseLogger
{
public:
	virtual void LogVarg(int level, const char* msg, va_list argptr);

private:
	void SetTextColor(int color);
};

/**
 * Class for writing log to file.
 */
class CFileLogger : public CBaseLogger
{
public:
	CFileLogger(const char* filename);

	virtual void LogVarg(int level, const char* msg, va_list argptr);

private:
	char m_szLogPath[MAX_PATH];
};

/**
 * Class for passing log message to GUI.
 * Must be used with CCompositeLogger because GUI requires formatted message
 * @todo don't create separate class for gui logging and move it to console logger?
 */
class CGUILogger : public CBaseLogger
{
public:
	void LogVarg(int level, const char* msg, va_list argptr);
};