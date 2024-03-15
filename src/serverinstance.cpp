#include "serverinstance.h"

#include "manager/packetmanager.h"
#include "manager/usermanager.h"
#include "manager/userdatabase.h"
#include "manager/channelmanager.h"
#include "manager/itemmanager.h"
#include "manager/shopmanager.h"
#include "manager/luckyitemmanager.h"
#include "manager/hostmanager.h"
#include "manager/dedicatedservermanager.h"
#include "manager/questmanager.h"
#include "manager/minigamemanager.h"
#include "manager/clanmanager.h"
#include "manager/rankmanager.h"

#include "net/receivepacket.h"
#include "common/buildnum.h"
#include "common/net/netdefs.h"
#include "common/utils.h"

#include "csvtable.h"
#include "serverconfig.h"
#include "consolecommands.h"
#ifdef USE_GUI
#include "gui/igui.h"
#endif

using namespace std;

CServerConfig* g_pServerConfig;
CPacketManager* g_pPacketManager;
#ifdef DB_SQLITE
CUserDatabaseSQLite* g_pUserDatabase;
#elif defined DB_MYSQL
CUserDatabaseMySQL* g_pUserDatabase;
#elif defined DB_POSTGRESQL
CUserDatabasePostgreSQL* g_pUserDatabase;
#else
CUserDatabase* g_pUserDatabase;
#endif
CUserManager* g_pUserManager;
CHostManager* g_pHostManager;
CChannelManager* g_pChannelManager;
CItemManager* g_pItemManager;
CShopManager* g_pShopManager;
CLuckyItemManager* g_pLuckyItemManager;
CQuestManager* g_pQuestManager;
CMiniGameManager* g_pMiniGameManager;
CClanManager* g_pClanManager;
CCSVTable* g_pItemTable;
CCSVTable* g_pMapListTable;
CCSVTable* g_pGameModeListTable;
CDedicatedServerManager* g_pDedicatedServerManager;
CRankManager* g_pRankManager;

CServerInstance::CServerInstance()
{
	m_bIsServerActive = true;
	m_CurrentTime = 0;
	m_pCurrentLocalTime = NULL;
	m_nUptime = 0;

	m_TCPServer.SetCriticalSection(&g_ServerCriticalSection);
	m_TCPServer.SetListener(this);
	m_UDPServer.SetCriticalSection(&g_ServerCriticalSection);
	m_UDPServer.SetListener(this);
}

bool CServerInstance::Init()
{
	if (g_pServerConfig)
	{
		UnloadConfigs();
		if (!LoadConfigs())
		{
			Console().Error("Server initialization failed.\n");
			m_bIsServerActive = false;
			return false;
		}

		return true;
	}

	if (!LoadConfigs())
	{
		Console().Error("Server initialization failed.\n");
		m_bIsServerActive = false;
		return false;
	}

	g_pPacketManager = new CPacketManager();
	g_pUserManager = new CUserManager(g_pServerConfig->maxPlayers);
	g_pHostManager = new CHostManager();
	g_pChannelManager = new CChannelManager();
	g_pItemManager = new CItemManager();
	g_pShopManager = new CShopManager();
	g_pLuckyItemManager = new CLuckyItemManager();
	g_pQuestManager = new CQuestManager();
	g_pMiniGameManager = new CMiniGameManager();
	g_pClanManager = new CClanManager();
	g_pRankManager = new CRankManager();

	g_pItemTable = new CCSVTable("Data/Item.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams(), true);
	g_pMapListTable = new CCSVTable("Data/MapList.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams(), true);
	g_pGameModeListTable = new CCSVTable("Data/GameModeList.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams(), true);
#ifdef DB_SQLITE
	g_pUserDatabase = new CUserDatabaseSQLite();
#elif defined DB_MYSQL
	g_pUserDatabase = new CUserDatabaseMySQL();
#elif defined DB_POSTGRESQL
	g_pUserDatabase = new CUserDatabasePostgreSQL();
#else
	g_pUserDatabase = new CUserDatabase();
#endif

	g_pDedicatedServerManager = new CDedicatedServerManager();

	if (!Manager().InitAll() ||
		!m_TCPServer.Start(g_pServerConfig->tcpPort, g_pServerConfig->tcpSendBufferSize) ||
		!m_UDPServer.Start(g_pServerConfig->udpPort))
	{
		Console().Error("Server initialization failed.\n");
		m_bIsServerActive = false;
		return false;
	}
	else if (g_pItemTable->IsLoadFailed())
	{
		Console().Error("Server initialization failed. Couldn't load Item.csv.\n");
		m_bIsServerActive = false;
		return false;
	}
	else if (g_pMapListTable->IsLoadFailed())
	{
		Console().Error("Server initialization failed. Couldn't load MapList.csv.\n");
		m_bIsServerActive = false;
		return false;
	}
	else if (g_pGameModeListTable->IsLoadFailed())
	{
		Console().Error("Server initialization failed. Couldn't load GameModeList.csv.\n");
		m_bIsServerActive = false;
		return false;
	}

	Console().Log("Server starts listening. Server developers: Jusic, Hardee, NekoMeow. Thx to Ochii for CSO2 server.\nFor more information visit discord.gg/EvUAY6D\n");
	Console().Log("Server build: %s, %s\n", build_number(),
#ifdef PUBLIC_RELEASE
		"Public Release");
#else
		"Private Release");
#endif

	/// @fixme: explanation why we call this
	OnSecondTick();

	return true;
}

CServerInstance::~CServerInstance()
{
	delete g_pPacketManager;
	delete g_pUserDatabase;
	delete g_pUserManager;
	delete g_pChannelManager;
	delete g_pItemManager;
	delete g_pShopManager;
	delete g_pLuckyItemManager;
	delete g_pQuestManager;
	delete g_pItemTable;
	delete g_pServerConfig;
	delete g_pDedicatedServerManager;
	delete g_pMiniGameManager;
	delete g_pClanManager;
	delete g_pRankManager;
	delete g_pHostManager;
}

bool CServerInstance::LoadConfigs()
{
	g_pServerConfig = new CServerConfig();
	return g_pServerConfig->Load();
}

void CServerInstance::UnloadConfigs()
{
	delete g_pServerConfig;
}

void CServerInstance::OnTCPConnectionCreated(IExtendedSocket* socket)
{
	if (g_pUserDatabase->IsIPBanned(socket->GetIP()))
	{
		Console().Log("Client (%d, %s) disconnected from the server due to banned ip\n", socket->GetID(), socket->GetIP().c_str());
		DisconnectClient(socket);

		// return false;
	}

	// return true;
}

void CServerInstance::OnTCPConnectionClosed(IExtendedSocket* socket)
{
	int bytesSent = socket->GetBytesSent();
	int bytesReceived = socket->GetBytesReceived();
	int sock = socket->GetSocket();

	// clean up user
	IUser* user = g_pUserManager->GetUserBySocket(socket);
	int userID = 0;
	string userName = "NULL";
	if (user)
	{
		userID = user->GetID();
		userName = user->GetUsername();
		g_pUserManager->RemoveUser(user);

		Console().Log("User logged out (%d, '%s', 0x%X)\n", userID, userName.c_str(), user);
	}
	else
	{
		g_pDedicatedServerManager->RemoveServer(socket);
	}

	/// @todo remove all events referred to deleted socket object
}

void CServerInstance::OnTCPMessage(IExtendedSocket* socket, CReceivePacket* msg)
{
	g_Event.AddEventPacket(socket, msg);
}

void CServerInstance::OnTCPError(int errorCode)
{
	//g_pChannelManager->EndAllGames();
	//SetServerActive(false);
}

void CServerInstance::OnUDPMessage(Buffer& buf, unsigned short port)
{
	if (buf.getBuffer().size() == 14)
	{
		char signature = buf.readUInt8();
		if (signature != UDP_HOLEPUNCH_PACKET_SIGNATURE_1)
		{
			Console().Log(OBFUSCATE("CPacketIn_UDP::Parse: signature error\n"));
			return;
		}

		int userID = buf.readUInt32_LE();
		int portID = buf.readUInt16_LE();
		long longAddr = buf.readUInt32_BE();
		string localIpAddress = ip_to_string(longAddr);
		short localPort = buf.readUInt16_LE();

		IUser* user = g_pUserManager->GetUserById(userID);
		if (!user)
		{
			return;
		}

		int result = user->UpdateHolepunch(portID, localPort, port);
		if (result == -1)
		{
			Console().Warn("Unknown hole punch port\n");
		}

		Buffer replyBuffer;
		replyBuffer.writeUInt8('W');
		replyBuffer.writeUInt8(0);
		replyBuffer.writeUInt8(1);

		// send reply
		m_UDPServer.SendTo(replyBuffer);
	}
}

void CServerInstance::OnUDPError(int errorCode)
{
}

void CServerInstance::OnCommand(const string& command)
{
	Console().Log("Command: %s\n", command.c_str());

	istringstream iss(command);
	vector<string> args((istream_iterator<string>(iss)), istream_iterator<string>());

	if (args.empty())
		return;

	CCommand* cmd = CmdList()->GetCommand(args[0]);
	if (cmd)
	{
		cmd->Exec(args);
	}
}

void* EventThread(void*)
{
	while (g_pServerInstance->IsServerActive())
	{
		g_Event.WaitForSignal();

		if (g_pServerInstance->IsServerActive())
			g_pServerInstance->OnEvent();
	}
	
	return NULL;
}

void* ReadConsoleThread(void*)
{
	while (g_pServerInstance->IsServerActive())
	{
		string cmd;
		getline(cin, cmd);

		// TODO: ignore empty line?

		g_Event.AddEventConsoleCommand(cmd);
	}
	
	return NULL;
}

void CServerInstance::SetServerActive(bool active)
{
	m_bIsServerActive = active;

	if (!m_bIsServerActive)
	{
		// wake up event thread
		g_Event.Signal();
	}
}

bool CServerInstance::IsServerActive()
{
	return m_bIsServerActive;
}

void CServerInstance::OnEvent()
{
	/// @todo
	bool empty;
	Event_s ev = g_Event.GetNextEvent(empty);
	while (!empty)
	{
		g_ServerCriticalSection.Enter();

		switch (ev.type)
		{
		case SERVER_EVENT_CONSOLE_COMMAND:
			OnCommand(ev.cmd);
			break;
		case SERVER_EVENT_TCP_PACKET:
			OnPackets(ev.socket, ev.msg);
			break;
		case SERVER_EVENT_SECOND_TICK:
			OnSecondTick();
			break;
		case SERVER_EVENT_FUNCTION:
			OnFunction(ev.func);
			break; 
		}

		if (ev.type == SERVER_EVENT_TCP_PACKET)
		{
			delete ev.msg;
		}

		g_ServerCriticalSection.Leave();

		ev = g_Event.GetNextEvent(empty);
	}
}

void CServerInstance::OnPackets(IExtendedSocket* s, CReceivePacket* msg)
{
	/// @todo don't use pointer to deleted object
	if (find(m_TCPServer.GetClients().begin(), m_TCPServer.GetClients().end(), s) == m_TCPServer.GetClients().end())
	{
		// skip packets with deleted socket object
		return;
	}

	switch (msg->GetID())
	{
	case PacketId::Version:
		g_pUserManager->OnVersionPacket(msg, s);
		break;
	case PacketId::Transfer:
		g_pUserManager->OnCharacterPacket(msg, s);
		break;
	case PacketId::Login:
		g_pUserManager->OnLoginPacket(msg, s);
		break;
	case PacketId::RequestChannels:
		g_pChannelManager->OnChannelListPacket(s);
		break;
	case PacketId::RequestRoomList:
		g_pChannelManager->OnRoomListPacket(msg, s);
		break;
	case PacketId::RecvCrypt:
		g_pUserManager->OnCryptPacket(msg, s);
		break;
	case PacketId::Room:
		g_pChannelManager->OnRoomRequest(msg, s);
		break;
	case PacketId::Shop:
		g_pShopManager->OnShopPacket(msg, s);
		break;
	case PacketId::UMsg:
		g_pUserManager->OnUserMessage(msg, s);
		break;
	case PacketId::Host:
		g_pHostManager->OnPacket(msg, s);
		break;
	case PacketId::Favorite:
		g_pUserManager->OnFavoritePacket(msg, s);
		break;
	case PacketId::Option:
		g_pUserManager->OnOptionPacket(msg, s);
		break;
	case PacketId::Udp:
		g_pUserManager->OnUdpPacket(msg, s);
		break;
	case PacketId::Item:
		g_pItemManager->OnItemPacket(msg, s);
		break;
	case PacketId::MiniGame:
		g_pMiniGameManager->OnPacket(msg, s);
		break;
	case PacketId::MileageBingo:
		break;
	case PacketId::UpdateInfo:
		g_pUserManager->OnUpdateInfoPacket(msg, s);
		break;
	case PacketId::Clan:
		g_pClanManager->OnPacket(msg, s);
		break;
	case PacketId::Statistic:
		g_pPacketManager->SendStatistic(s);
		break;
	case PacketId::Rank:
		g_pRankManager->OnRankPacket(msg, s);
		break;
	case PacketId::Hack:
		//printf("shit");
		break;
	case PacketId::Report:
		g_pUserManager->OnReportPacket(msg, s);
		break;
	case PacketId::Alarm:
		g_pUserManager->OnAlarmPacket(msg, s);
		break;
	case PacketId::Quest:
		g_pQuestManager->OnPacket(msg, s);
		break;
	case PacketId::Title:
		g_pQuestManager->OnTitlePacket(msg, s);
		break;
	case PacketId::HostServer:
		g_pDedicatedServerManager->OnPacket(msg, s);
		break;
	case PacketId::Messenger:
		g_pUserManager->OnMessengerPacket(msg, s);
		break;
	case PacketId::UserSurvey:
		g_pUserManager->OnUserSurveyPacket(msg, s);
		break;
	case PacketId::Addon:
		g_pUserManager->OnAddonPacket(msg, s);
		break;
	case PacketId::Ban:
		g_pUserManager->OnBanPacket(msg, s);
		break;
	case PacketId::League:
		g_pUserManager->OnLeaguePacket(msg, s);
		break;
	default:
		Console().Warn("Unimplemented packet: %d\n", msg->GetID());
		break;
	}
}

void CServerInstance::OnSecondTick()
{
	// update current time
	time_t prevTime = m_CurrentTime;
	m_CurrentTime = time(NULL);
	m_pCurrentLocalTime = localtime(&m_CurrentTime);
	m_CurrentTime /= 60; // get current time in minutes(last CSO builds use timestamp in minutes)
	m_nUptime++;

	UpdateConsoleStatus();

#ifdef USE_GUI
	GUI()->UpdateInfo(m_bIsServerActive, m_TCPServer.GetClients().size(), m_nUptime, GetMemoryInfo());
#endif

	Manager().SecondTick(m_CurrentTime);

	if (m_CurrentTime - prevTime > 0)
	{
		OnMinuteTick();
	}
}

void CServerInstance::OnMinuteTick()
{
	Console().Log("%s\n", GetMainInfo());

	Manager().MinuteTick(m_CurrentTime);
}

void CServerInstance::OnFunction(function<void()>& func)
{
	func();
}

double CServerInstance::GetMemoryInfo()
{
#ifdef WIN32
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	SIZE_T mem = pmc.WorkingSetSize;
	if (mem >= 10e9)
		Console().Warn("[ALERT] Server is using more than 1G of memory.\n");

	return mem / (1024.0 * 1024.0);
#else
	Console().Log("CServerInstance::GetMemoryInfo: not implemented\n");
	return 0;
#endif
}

const char* CServerInstance::GetMainInfo()
{
	return va("Memory usage: %.02fmb. Connected users: %d. Logged in users: %d.", GetMemoryInfo(), static_cast<int>(m_TCPServer.GetClients().size()), static_cast<int>(g_pUserManager->GetUsers().size()));
}

void CServerInstance::DisconnectClient(IExtendedSocket* socket)
{
	if (!socket)
		return;

	m_TCPServer.DisconnectClient(socket);
}

std::vector<IExtendedSocket*> CServerInstance::GetClients()
{
	return m_TCPServer.GetClients();
}

IExtendedSocket* CServerInstance::GetSocketByID(unsigned int id)
{
	for (auto s : m_TCPServer.GetClients())
		if (s->GetID() == id)
			return s;

	return NULL;
}

void CServerInstance::UpdateConsoleStatus()
{
	Console().SetStatus(GetMainInfo());
	Console().UpdateStatus();
}

time_t CServerInstance::GetCurrentTime()
{
	return m_CurrentTime; // timestamp in minutes
}

tm* CServerInstance::GetCurrentLocalTime()
{
	return m_pCurrentLocalTime;
}