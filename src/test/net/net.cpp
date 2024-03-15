#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "testbasicfuncs.h"
#include "testpacketsequence.h"

#define TEST_PORT "30002"

using namespace std;

TEST_CASE("Network (TCP) - Test send/receive")
{
	CTCPServer_TestBasicFuncs server(TEST_PORT);
	CTCPClient_TestBasicFuncs client("127.0.0.1", TEST_PORT);

	while (!server.m_bFailed && !server.m_bFinished)
	{
		// wait for the test finish/error
	}
}

TEST_CASE("Network (TCP) - Test packet sequence and/or segmentation")
{
	CTCPServer_TestPacketSequence server(TEST_PORT);
	CTCPClient_TestPacketSequence client("127.0.0.1", TEST_PORT);

	while (!server.m_bFailed && !server.m_bFinished)
	{
		// wait for the test finish/error
	}
}