#include <doctest/doctest.h>
#include "../event.h"
#include <thread>
#include <atomic>

using namespace std;

TEST_CASE("Events - test events")
{
	// Add two events, call WaitForSignal() from another thread, check if events executed
	int counter = 0;
	auto func = [&counter]() {
		counter++;
	};

	CEvents events;
	events.AddEventFunction(func);
	events.AddEventFunction(func);

	std::atomic<bool> done{ false };

	auto eventThreadFunc = [&done, &events]() {
		events.WaitForSignal();

		IEvent* ev = events.GetNextEvent();

		CHECK(ev);
		ev->Execute();
		ev = events.GetNextEvent();

		CHECK(ev);
		ev->Execute();
		ev = events.GetNextEvent();

		CHECK(!ev);

		done = true;
	};

	thread t(eventThreadFunc);

	// wait for a thread to end
	this_thread::sleep_for(200ms);

	// check if thread is done
	CHECK(done == true);

	t.join();

	CHECK(counter == 2);
}
