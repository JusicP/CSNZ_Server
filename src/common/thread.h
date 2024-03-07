#pragma once

#include <stdio.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#ifdef WIN32
typedef DWORD ThreadId;
#else
typedef pthread_t ThreadId;
#endif

class CObjectSync
{
public:
	CObjectSync()
	{
#ifdef _WIN32
		m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_hEvent == NULL)
			printf("CObjectSync: Failed to create event\n");
#else
		m_bSignalled = false;
		pthread_mutex_init(&m_Mutex, NULL);
		pthread_cond_init(&m_Cond, NULL);
#endif
	}
	~CObjectSync()
	{
#ifdef _WIN32
		CloseHandle(m_hEvent);
#else
		pthread_mutex_destroy(&m_Mutex);
		pthread_cond_destroy(&m_Cond);
#endif
	}

	void WaitForSignal()
	{
#ifdef _WIN32
		DWORD dwEvent = WaitForSingleObject(m_hEvent, INFINITE);
		switch (dwEvent)
		{
		case WAIT_ABANDONED:
			printf("CObjectSync: WAIT_ABANDONED: %d\n", GetLastError());
			break;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			printf("CObjectSync: WAIT_TIMEOUT: %d\n", GetLastError());
			break;
		case WAIT_FAILED:
			printf("CObjectSync: WAIT_FAILED: %d\n", GetLastError());
			break;
		}
#else
		pthread_mutex_lock(&m_Mutex);
		while (!m_bSignalled)
		{
			pthread_cond_wait(&m_Cond, &m_Mutex);
		}
		m_bSignalled = false;
		pthread_mutex_unlock(&m_Mutex);
#endif
	}

	void Signal()
	{
#ifdef _WIN32
		SetEvent(m_hEvent);
#else
		pthread_mutex_lock(&m_Mutex);
		m_bSignalled = true;
		pthread_mutex_unlock(&m_Mutex);
		pthread_cond_signal(&m_Cond);
#endif
	}

private:
#ifdef _WIN32
	HANDLE m_hEvent;
#else
	bool m_bSignalled;
	pthread_mutex_t m_Mutex;
	pthread_cond_t m_Cond;
#endif
};

class CCriticalSection
{
public:
	CCriticalSection()
	{
#ifdef _WIN32
		InitializeCriticalSection(&m_CriticalSection);
#else
		pthread_mutex_init(&m_Mutex, NULL);
#endif
	}

	~CCriticalSection()
	{
#ifdef _WIN32
		DeleteCriticalSection(&m_CriticalSection);
#else
		pthread_mutex_destroy(&m_Mutex);
#endif
	}

	void Enter()
	{
#ifdef _WIN32
		EnterCriticalSection(&m_CriticalSection);
#else
		pthread_mutex_lock(&m_Mutex);
#endif
	}

	int TryEnter()
	{
#ifdef _WIN32
		return TryEnterCriticalSection(&m_CriticalSection);
#else
		return pthread_mutex_trylock(&m_Mutex);
#endif
	}

	void Leave()
	{
#ifdef _WIN32
		LeaveCriticalSection(&m_CriticalSection);
#else
		pthread_mutex_unlock(&m_Mutex);
#endif
	}

private:
#ifdef _WIN32
	CRITICAL_SECTION m_CriticalSection;
#else
	pthread_mutex_t m_Mutex;
#endif
};

typedef void* (*Handler)(void*);

ThreadId GetCurrentThreadID();

// Win32/POSIX thread class
class CThread
{
public:
	CThread(const Handler& function, void* data = NULL);
	~CThread();

	bool Start();
	void Join();
	void Terminate();
	bool IsAlive();
	bool IsCurrentThreadSame();

	// threads are non-copyable
	CThread(const CThread&) = delete;
	CThread& operator=(const CThread&) = delete;

private:
	Handler m_Object;
#ifdef WIN32
	HANDLE m_hHandle;
#endif
	ThreadId m_ID;
	void* m_pData;
};