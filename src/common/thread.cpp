#include "thread.h"

ThreadId GetCurrentThreadID()
{
#ifdef WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

CThread::CThread(const Handler& function, void* data)
{
    m_Object = function;
	m_pData = data;
#ifdef WIN32
    m_hHandle = 0;
#endif
    m_ID = 0;
}

CThread::~CThread()
{
#ifdef WIN32
    CloseHandle(m_hHandle);
#endif
}

// just start thread
bool CThread::Start()
{
    if (IsAlive())
    {
        return false;
    }

#ifdef WIN32
    m_hHandle = CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(m_Object), m_pData, 0, &m_ID);
    if (m_hHandle == 0)
    {
        return false;
    }
#else
    int result = pthread_create(&m_ID, NULL, m_Object, m_pData);
    if (result != 0)
    {
        return false;
    }
#endif

    return true;
}

// pauses current thread execution until other thread is finish
void CThread::Join()
{
#ifdef WIN32
    if (!m_hHandle)
#else
    if (!m_ID)
#endif
        return;
     
    if (IsCurrentThreadSame())
        return;

#ifdef WIN32
    DWORD dwResult = WaitForSingleObject(m_hHandle, INFINITE);
    if (dwResult == WAIT_FAILED)
        printf("CThread::Join: dwResult == WAIT_FAILED\n");
#else
    if (pthread_join(m_ID, NULL) != 0)
        printf("CThread::Join: pthread_join != 0\n");
#endif
}

// terminates thread (very unsafe if you don't know what you're doing)
void CThread::Terminate()
{
#ifdef WIN32
    TerminateThread(m_hHandle, 0);
    CloseHandle(m_hHandle);
    m_hHandle = 0;
#else
    pthread_kill(m_ID, SIGKILL);
#endif
    m_ID = 0;
}

// checks if thread is alive (created and not exited)
bool CThread::IsAlive()
{
#ifdef WIN32
    DWORD exitCode;
    return (m_hHandle && GetExitCodeThread(m_hHandle, &exitCode) && exitCode == STILL_ACTIVE);
#else
    return m_ID;
#endif
}

bool CThread::IsCurrentThreadSame()
{
    ThreadId id = GetCurrentThreadID();
#ifdef WIN32
    return id == m_ID;
#else
    return pthread_equal(id, m_ID);
#endif
}