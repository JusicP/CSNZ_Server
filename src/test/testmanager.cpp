#include <doctest/doctest.h>
#include "../manager/manager.h"

using namespace std;

class ITestManager : public IBaseManager
{
public:
	virtual void DoSomething() = 0;
};

/**
 * Test manager class to test basic functionality (init, shutdown, ticks)
 */
class CTestManager : public CBaseManager<ITestManager>
{
public:
	CTestManager() : CBaseManager("TestManager", true, true)
	{
		m_bInitCalled = false;
		m_bShutdownCalled = false;
		m_bReloaded = false;
		m_bSecondTickCalled = false;
		m_bMinuteTickCalled = false;
	}

	virtual bool Init()
	{
		// ReloadAll called
		if (m_bInitCalled && m_bShutdownCalled)
			m_bReloaded = true;

		m_bInitCalled = true;
		return true;
	}

	virtual void Shutdown()
	{
		m_bShutdownCalled = true;
	}

	virtual void OnSecondTick(time_t curTime)
	{
		m_bSecondTickCalled = true;
	}

	virtual void OnMinuteTick(time_t curTime)
	{
		m_bMinuteTickCalled = true;
	}

	virtual void DoSomething()
	{
	}

	bool m_bInitCalled;
	bool m_bShutdownCalled;
	bool m_bReloaded;
	bool m_bSecondTickCalled;
	bool m_bMinuteTickCalled;
};

TEST_CASE("Manager - Test init/shutdown, reload, remove, ticks")
{
	CTestManager mgr;

	CHECK(Manager().GetManager("TestManager") != NULL);

	CHECK(Manager().InitAll() == true);
	CHECK(mgr.m_bInitCalled == true);

	SUBCASE("Test remove manager")
	{
		// test tick calling
		Manager().SecondTick(0);
		Manager().MinuteTick(0);

		CHECK(mgr.m_bSecondTickCalled == true);
		CHECK(mgr.m_bMinuteTickCalled == true);

		// test tick not calling
		mgr.m_bSecondTickCalled = false;
		mgr.m_bMinuteTickCalled = false;

		mgr.SetSecondTick(false);
		mgr.SetMinuteTick(false);

		Manager().SecondTick(0);
		Manager().MinuteTick(0);

		CHECK(mgr.m_bSecondTickCalled == false);
		CHECK(mgr.m_bMinuteTickCalled == false);
	}

	SUBCASE("Test remove manager")
	{
		Manager().RemoveManager(&mgr);
		CHECK(Manager().GetManager("TestManager") == NULL);
	}

	SUBCASE("Test shutdown all")
	{
		Manager().ShutdownAll();
		CHECK(mgr.m_bShutdownCalled == true);
	}

	SUBCASE("Test reload all")
	{
		CHECK(Manager().ReloadAll() == true);
		CHECK(mgr.m_bReloaded == true);
	}

	SUBCASE("Test reload all if manager can't be reloaded")
	{
		mgr.SetCanReload(false);
		CHECK(Manager().ReloadAll() == true);
		CHECK(mgr.m_bReloaded == false);
	}
}
