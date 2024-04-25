#include <doctest/doctest.h>
#include "../command.h"

using namespace std;

TEST_CASE("Command - add, search, execute")
{
	bool executed = false;
	auto commandFunc = [&executed](CCommand* cmd, const vector<string>& args) {
		executed = true;
	};

	const string cmdName = "test_cmd";
	CCommand cmd(cmdName, "description", "", commandFunc);

	// check if can find command
	CHECK(CCommandList::GetInstance().GetCommand(cmdName));

	// check if command in command list
	auto cmdList = CCommandList::GetInstance().GetCommandList();
	CHECK(find(cmdList.begin(), cmdList.end(), cmdName) != cmdList.end());

	cmd.Exec({});

	// check if command executed
	CHECK(executed);
}
