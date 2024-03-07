#include "channelserver.h"
#include "common/utils.h"

using namespace std;

CChannelServer::CChannelServer(string serverName, int serverIndex, int totalServers, int numOfChannels)
{
	m_Name = FormatServerName(serverName, serverIndex, totalServers);
	m_nIndex = serverIndex;
	m_nNextChannelID = 1;

	for (m_nIndex = 0; m_nIndex < numOfChannels; m_nIndex++)
	{
		int newChannelIndex = m_nNextChannelID;
		string newChannelName = FormatChannelName(serverName, serverIndex, newChannelIndex);
		m_Channels.push_back(new CChannel(this, newChannelIndex, newChannelName, 100, ""));
		m_nNextChannelID++;
	}
}

CChannelServer::~CChannelServer()
{
	for (auto channel : m_Channels)
	{
		delete channel;
	}
}

string CChannelServer::FormatServerName(string serverName, int serverIndex, int totalServers)
{
	/*
	//иновационный подход
	//(почему-то std::operator+ для std::string не перегружен для работы с int,
	//будем делать с СИ хуйней пока что):

	string temp = "";
	char formatted[128];
	sprintf(formatted, "%s [%i / %i]", serverName.c_str(), serverIndex, totalServers);
	temp += formatted;
	return temp;
	*/

	string name(va("%s [%i / %i]", serverName.c_str(), serverIndex, totalServers));
	return name;
}

string CChannelServer::FormatChannelName(string serverName, int serverIndex, int channedlIndex)
{
	/*
	//иновационный подход
	//(почему-то std::operator+ для std::string не перегружен для работы с int,
	//будем делать с СИ хуйней пока что):

	string temp = "";
	char formatted[128];
	sprintf(formatted, "%s %i-%i", serverName.c_str(), serverIndex, channedlIndex);
	temp += formatted;
	return temp;
	*/

	string name(va("%s %i-%i", serverName.c_str(), serverIndex, channedlIndex));
	return name;
}

CChannel* CChannelServer::GetChannelByIndex(int index)
{
	for (auto channel : m_Channels)
	{
		if (channel->GetID() == index)
			return channel;
	}

	return NULL;
}

std::vector<CChannel*> CChannelServer::GetChannels()
{
	return m_Channels;
}

std::string CChannelServer::GetName()
{
	return m_Name;
}

int CChannelServer::GetID()
{
	return m_nIndex;
}