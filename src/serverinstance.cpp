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
#include "manager/voxelmanager.h"

#include "net/receivepacket.h"
#include "common/buildnum.h"
#include "common/net/netdefs.h"
#include "common/utils.h"

#include "csvtable.h"
#include "serverconfig.h"
#include "servercommands.h"

#include <unordered_map>
#include <vector>
#ifdef USE_GUI
#include "gui/igui.h"
#endif

using namespace std;

CServerConfig* g_pServerConfig;
CCSVTable* g_pItemTable;
CCSVTable* g_pMapListTable;
CCSVTable* g_pGameModeListTable;

CServerInstance::CServerInstance()
{
	m_bIsServerActive = false;
	m_CurrentTime = 0;
	m_pCurrentLocalTime = NULL;
	m_nUptime = 0;

	m_TCPServer.SetCriticalSection(&g_ServerCriticalSection);
	m_TCPServer.SetListener(this);
	m_UDPServer.SetCriticalSection(&g_ServerCriticalSection);
	m_UDPServer.SetListener(this);
}

CServerInstance::~CServerInstance()
{
	Manager().ShutdownAll();

	delete g_pItemTable;
	delete g_pMapListTable;
	delete g_pGameModeListTable;
	delete g_pServerConfig;
}

bool CServerInstance::Init()
{
	if (m_bIsServerActive)
		return true;

	if (!LoadConfigs())
	{
		Logger().Error("Server initialization failed.\n");
		m_bIsServerActive = false;
		return false;
	}

	g_pItemTable = new CCSVTable("Data/Item.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams(), true);
	g_pMapListTable = new CCSVTable("Data/MapList.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams());
	g_pGameModeListTable = new CCSVTable("Data/GameModeList.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams());

	if (!Manager().InitAll() ||
		!m_TCPServer.Start(g_pServerConfig->tcpPort, g_pServerConfig->tcpSendBufferSize, g_pServerConfig->ssl) ||
		!m_UDPServer.Start(g_pServerConfig->udpPort))
	{
		Logger().Error("Server initialization failed.\n");
		m_bIsServerActive = false;
		return false;
	}
	else if (g_pItemTable->IsLoadFailed())
	{
		Logger().Error("Server initialization failed. Couldn't load Item.csv.\n");
		m_bIsServerActive = false;
		return false;
	}
	else if (g_pMapListTable->IsLoadFailed())
	{
		Logger().Error("Server initialization failed. Couldn't load MapList.csv.\n");
		m_bIsServerActive = false;
		return false;
	}
	else if (g_pGameModeListTable->IsLoadFailed())
	{
		Logger().Error("Server initialization failed. Couldn't load GameModeList.csv.\n");
		m_bIsServerActive = false;
		return false;
	}

	Logger().Info("Server starts listening. Server developers: Jusic, Hardee, NekoMeow, Smilex_Gamer, xRiseless. Thx to Ochii for CSO2 server.\nFor more information visit discord.gg/EvUAY6D\n");
	Logger().Info("Server build: %s, %s\n", build_number(),
#ifdef PUBLIC_RELEASE
		"Public Release");
#else
		"Private Release");
#endif

	m_bIsServerActive = true;

	/// @fixme: explanation why we call this
	OnSecondTick();

	return true;
}

bool CServerInstance::Reload()
{
	// reinit all managers and server config without shutting down the server
	// use case: you updated config data and want to apply it without shutting down the server (it can be dangerous)

	// reload server config
	if (g_pServerConfig)
	{
		UnloadConfigs();
		if (!LoadConfigs())
		{
			return false;
		}

		if (g_pServerConfig->ssl)
			m_TCPServer.InitSSLContext();
	}

	if (!Manager().ReloadAll())
		return false;

	return true;
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

bool CServerInstance::OnTCPConnectionCreated(IExtendedSocket* socket)
{
	if (g_UserDatabase.IsIPBanned(socket->GetIP()))
	{
		Logger().Info("Client (%d, %s) disconnected from the server due to banned ip\n", socket->GetID(), socket->GetIP().c_str());
		DisconnectClient(socket);

		return false;
	}

	return true;
}

void CServerInstance::OnTCPConnectionClosed(IExtendedSocket* socket)
{
	int bytesSent = socket->GetBytesSent();
	int bytesReceived = socket->GetBytesReceived();
	int sock = (int)socket->GetSocket();

	// clean up user/dedicated server
	IUser* user = g_UserManager.GetUserBySocket(socket);
	if (user)
	{
		int userID = user->GetID();
		string userName = user->GetUsername();
		g_UserManager.RemoveUser(user);

		Logger().Info("User logged out (%d, '%s', 0x%X)\n", userID, userName.c_str(), user);
	}
	else
	{
		g_DedicatedServerManager.RemoveServer(socket);
	}

	// events with socket should be removed as they refer to deleted object 
	g_Events.RemoveEventsBySocket(socket);
}

void CServerInstance::OnTCPMessage(IExtendedSocket* socket, CReceivePacket* msg)
{
	g_Events.AddEventTCPPacket(socket, std::bind(&CServerInstance::OnPackets, this, socket, msg));
}

void CServerInstance::OnTCPError(int errorCode)
{
	//g_ChannelManager.EndAllGames();
	//SetServerActive(false);
}

namespace
{
	enum
	{
		UDP_RELAY_SOCK_SERVER = 0,
		UDP_RELAY_SOCK_CLIENT = 1,
		UDP_RELAY_SOCK_COUNT
	};

	struct UdpRelayEndpoints_s
	{
		sockaddr_in addr[UDP_RELAY_SOCK_COUNT] = {};
		bool valid[UDP_RELAY_SOCK_COUNT] = {};
	};

	unordered_map<int, UdpRelayEndpoints_s> g_UdpRelayEndpoints;

	int ReadInt32LE(const unsigned char* p)
	{
		return static_cast<int>(p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<unsigned int>(p[3]) << 24));
	}

	bool UpdateRelayEndpoint(int userID, int sock, const sockaddr_in& from)
	{
		if (sock < 0 || sock >= UDP_RELAY_SOCK_COUNT)
			return false;

		IUser* user = g_UserManager.GetUserById(userID);
		if (!user || !user->GetExtendedSocket())
		{
			g_UdpRelayEndpoints.erase(userID);
			return false;
		}

		if (ip_string_to_int(user->GetExtendedSocket()->GetIP()) != ntohl(from.sin_addr.s_addr))
			return false;

		UdpRelayEndpoints_s& endpoints = g_UdpRelayEndpoints[userID];
		endpoints.addr[sock] = from;
		endpoints.valid[sock] = true;
		return true;
	}

	bool GetRelayEndpoint(int userID, int sock, sockaddr_in& out)
	{
		if (sock < 0 || sock >= UDP_RELAY_SOCK_COUNT)
			return false;

		auto it = g_UdpRelayEndpoints.find(userID);
		if (it == g_UdpRelayEndpoints.end() || !it->second.valid[sock])
			return false;

		out = it->second.addr[sock];
		return true;
	}
}

void CServerInstance::OnUDPMessage(Buffer& buf, const sockaddr_in& from)
{
	const vector<unsigned char>& data = buf.getBuffer();
	if (data.empty())
		return;

	switch (data[0])
	{
	case UDP_HOLEPUNCH_PACKET_SIGNATURE_1:
		OnUDPHolepunchMessage(buf, from);
		break;
	case UDP_RELAY_PACKET_SIGNATURE:
		OnUDPRelayMessage(buf, from);
		break;
	default:
		break;
	}
}

void CServerInstance::OnUDPHolepunchMessage(Buffer& buf, const sockaddr_in& from)
{
	// 7, 14 is well known
	// 14 = type 0 (Punch)
	// 7 = type 1 (HeartBeat)
	if (buf.getBuffer().size() < 7)
	{
		Logger().Error("[CServerInstance::OnUDPMessage] invalid packet???\n");
		return;
	}

	buf.readUInt8(); // signature

	int userID = buf.readUInt32_LE();
	int type = buf.readUInt8();

	IUser* user = g_UserManager.GetUserById(userID);
	if (!user)
	{
		Logger().Info(OBFUSCATE("[CServerInstance::OnUDPMessage] User '%d' Sent UDP Packet but not inside g_UserManager\n"), userID);
		return;
	}

	if (type == 0)
	{
		if (buf.getBuffer().size() < 14)
		{
			Logger().Debug("[CServerInstance::OnUDPMessage] invalid holepunch packet\n");
			return;
		}

		// 0 = NS_CLIENT socket (m_nExternalServerPort), 1 = NS_SERVER socket (m_nExternalClientPort)
		int portID = buf.readUInt8();
		int localAddr = ~buf.readUInt32_BE(); // the client sends its LAN address bitwise-inverted
		string localIpAddress = ip_to_string(localAddr);
		int localPort = buf.readUInt16_LE();
		int tries = buf.readUInt8();

		const int externalPort = ntohs(from.sin_port);

		Logger().Info("OnUDPMessage(0) - userID: %d, portID: %d, localAddr: %d (%s), localPort: %d, tries: %d\n", userID, portID, localAddr, localIpAddress.c_str(), localPort, tries);

		if (user->UpdateHolepunch(portID, localIpAddress, localPort, externalPort) == -1)
		{
			Logger().Warn("UpdateHolepunch Failed: %d, %d, %d\n", portID, localPort, externalPort);
		}
		else
		{
			UpdateRelayEndpoint(userID, portID == 0 ? UDP_RELAY_SOCK_CLIENT : UDP_RELAY_SOCK_SERVER, from);
		}

		Buffer replyBuffer;
		replyBuffer.writeUInt8('W');
		replyBuffer.writeUInt8(0);
		replyBuffer.writeUInt8(1);
		m_UDPServer.SendTo(replyBuffer, from);
	}
	else if (type == 1)
	{
		int tries = buf.readUInt8();

#ifdef _DEBUG
		Logger().Debug("OnUDPMessage(1) - userID: %d, tries: %d\n", userID, tries);
#endif
	}
}

void CServerInstance::OnUDPRelayMessage(Buffer& buf, const sockaddr_in& from)
{
	const vector<unsigned char>& data = buf.getBuffer();
	if (data.size() < UDP_RELAY_HEADER_SIZE)
		return;

	const int userID = ReadInt32LE(&data[1]);
	const int sock = data[5];
	const int destUserID = ReadInt32LE(&data[6]);

	if (!UpdateRelayEndpoint(userID, sock, from))
		return;

	if (destUserID == 0)
		return; // keepalive

	const unsigned char* payload = &data[UDP_RELAY_HEADER_SIZE];
	const int payloadLen = static_cast<int>(data.size()) - UDP_RELAY_HEADER_SIZE;

	if (payloadLen >= 5 && (payload[0] == UDP_HOLEPUNCH_PACKET_SIGNATURE_2 || payload[0] == UDP_GAME_PACKET_SIGNATURE) &&
		ReadInt32LE(&payload[1]) != userID)
	{
		return;
	}

	sockaddr_in to;
	if (!GetRelayEndpoint(destUserID, UDP_RELAY_SOCK_COUNT - 1 - sock, to))
		return;

	m_UDPServer.SendTo(payload, payloadLen, to);
}

void CServerInstance::OnUDPError(int errorCode)
{
}

void CServerInstance::OnCommand(const string& command)
{
	Logger().Info("Command: %s\n", command.c_str());

	istringstream iss(command);
	vector<string> args((istream_iterator<string>(iss)), istream_iterator<string>());

	if (args.empty())
		return;

	CCommand* cmd = CmdList().GetCommand(args[0]);
	if (cmd)
	{
		cmd->Exec(args);
	}
}

/**
 * Processes server events such as console commands, incoming messages, GUI requests
 */
void* EventThread(void*)
{
	while (g_pServerInstance->IsServerActive())
	{
		g_Events.WaitForSignal();

		// check for IsServerActive again because the state may change
		if (g_pServerInstance->IsServerActive())
			g_pServerInstance->OnEvent();
	}
	
	return NULL;
}

/**
 * Thread that reads console input
 */
void* ReadConsoleThread(void*)
{
	while (g_pServerInstance->IsServerActive())
	{
		string cmd;
		getline(cin, cmd);

		// TODO: ignore empty line?

		g_Events.AddEventFunction(std::bind(&CServerInstance::OnCommand, g_pServerInstance, cmd));
	}
	
	return NULL;
}

void CServerInstance::SetServerActive(bool active)
{
	m_bIsServerActive = active;

	if (!m_bIsServerActive)
	{
		// wake up event thread
		g_Events.Signal();
	}
}

bool CServerInstance::IsServerActive()
{
	return m_bIsServerActive;
}

/**
 * Processes server events from the queue
 */
void CServerInstance::OnEvent()
{
	IEvent* ev = g_Events.GetNextEvent();
	while (ev)
	{
		g_ServerCriticalSection.Enter();
			
		ev->Execute();

		g_ServerCriticalSection.Leave();

		ev = g_Events.GetNextEvent();
	}
}

void CServerInstance::OnPackets(IExtendedSocket* s, CReceivePacket* msg)
{
	switch (msg->GetID())
	{
	case PacketId::Version:
		g_UserManager.OnVersionPacket(msg, s);
		break;
	case PacketId::CreateCharacter:
		g_UserManager.OnCharacterPacket(msg, s);
		break;
	case PacketId::Login:
		g_UserManager.OnLoginPacket(msg, s);
		break;
	case PacketId::RequestServerList:
		g_ChannelManager.OnChannelListPacket(s);
		break;
	case PacketId::RequestTransfer:
		g_ChannelManager.OnRoomListPacket(msg, s);
		break;
	case PacketId::RecvCrypt:
		g_UserManager.OnCryptPacket(msg, s);
		break;
	case PacketId::Room:
		g_ChannelManager.OnRoomRequest(msg, s);
		break;
	case PacketId::Shop:
		g_ShopManager.OnShopPacket(msg, s);
		break;
	case PacketId::UMsg:
		g_UserManager.OnUserMessage(msg, s);
		break;
	case PacketId::Host:
		g_HostManager.OnPacket(msg, s);
		break;
	case PacketId::Favorite:
		g_UserManager.OnFavoritePacket(msg, s);
		break;
	case PacketId::Option:
		g_UserManager.OnOptionPacket(msg, s);
		break;
	case PacketId::Udp:
		g_UserManager.OnUdpPacket(msg, s);
		break;
	case PacketId::Item:
		g_ItemManager.OnItemPacket(msg, s);
		break;
	case PacketId::MiniGame:
		g_MiniGameManager.OnPacket(msg, s);
		break;
	case PacketId::MileageBingo:
		break;
	case PacketId::UpdateInfo:
		g_UserManager.OnUpdateInfoPacket(msg, s);
		break;
	case PacketId::Clan:
		g_ClanManager.OnPacket(msg, s);
		break;
	case PacketId::Statistic:
		g_PacketManager.SendStatistic(s);
		break;
	case PacketId::Rank:
		g_RankManager.OnRankPacket(msg, s);
		break;
	case PacketId::Hack:
		break;
	case PacketId::Report:
		g_UserManager.OnReportPacket(msg, s);
		break;
	case PacketId::Alarm:
		g_UserManager.OnAlarmPacket(msg, s);
		break;
	case PacketId::Quest:
		g_QuestManager.OnPacket(msg, s);
		break;
	case PacketId::Title:
		g_QuestManager.OnTitlePacket(msg, s);
		break;
	case PacketId::HostServer:
		g_DedicatedServerManager.OnPacket(msg, s);
		break;
	case PacketId::Messenger:
		g_UserManager.OnMessengerPacket(msg, s);
		break;
	case PacketId::UserSurvey:
		g_UserManager.OnUserSurveyPacket(msg, s);
		break;
	case PacketId::Addon:
		g_UserManager.OnAddonPacket(msg, s);
		break;
	case PacketId::Ban:
		g_UserManager.OnBanPacket(msg, s);
		break;
	case PacketId::League:
		g_UserManager.OnLeaguePacket(msg, s);
		break;
	case PacketId::Kick:
		g_UserManager.OnKickPacket(msg, s);
		break;
	case PacketId::Voxel:
		g_VoxelManager.OnPacket(msg, s);
		break;
	case PacketId::ClassMod:
		g_UserManager.OnClassModPacket(msg, s);
		break;
	default:
		Logger().Warn("Unimplemented packet: %d\n", msg->GetID());
		break;
	}

	delete msg;
}

void CServerInstance::OnSecondTick()
{
	// update current time
	time_t prevTime = m_CurrentTime;
	m_CurrentTime = time(NULL);
	m_pCurrentLocalTime = localtime(&m_CurrentTime);
	m_CurrentTime /= 60; // get current time in minutes(last CSO builds use timestamp in minutes)
	m_nUptime++;

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
	Logger().Info("%s\n", GetMainInfo());

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
		Logger().Warn("[ALERT] Server is using more than 1G of memory.\n");

	return mem / (1024.0 * 1024.0);
#else
	Logger().Info("CServerInstance::GetMemoryInfo: not implemented\n");
	return 0;
#endif
}

const char* CServerInstance::GetMainInfo()
{
	return va("Memory usage: %.02fmb. Connected users: %d. Logged in users: %d. Connected dedicated servers: %d.", GetMemoryInfo(), static_cast<int>(m_TCPServer.GetClients().size() - g_DedicatedServerManager.GetServers().size()), static_cast<int>(g_UserManager.GetUsers().size()), static_cast<int>(g_DedicatedServerManager.GetServers().size()));
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

time_t CServerInstance::GetCurrentTime()
{
	return m_CurrentTime; // timestamp in minutes
}

tm* CServerInstance::GetCurrentLocalTime()
{
	return m_pCurrentLocalTime;
}