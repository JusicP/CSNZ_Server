#include "dedicatedservermanager.h"
#include "usermanager.h"
#include "packetmanager.h"
#include "common/utils.h"

CDedicatedServer::CDedicatedServer(IExtendedSocket* socket, int ip, int port)
{
	m_pSocket = socket;
	m_iIP = ip;
	m_iPort = port; // dedi client/server port
	m_pRoom = NULL;
	m_iLastMemory = 0;

	g_UserManager.SendMetadata(socket);
}

void CDedicatedServer::SetRoom(IRoom* room)
{
	m_pRoom = room;
}

void CDedicatedServer::SetMemoryUsage(int memShift)
{
	m_iLastMemory = memShift << 20;
}

IExtendedSocket* CDedicatedServer::GetSocket()
{
	return m_pSocket;
}

IRoom* CDedicatedServer::GetRoom()
{
	return m_pRoom;
}

int CDedicatedServer::GetMemoryUsage()
{
	return m_iLastMemory;
}

int CDedicatedServer::GetIP()
{
	return m_iIP;
}

int CDedicatedServer::GetPort()
{
	return m_iPort;
}

CDedicatedServerManager g_DedicatedServerManager;

CDedicatedServerManager::CDedicatedServerManager() : CBaseManager("DedicatedServerManager")
{
	SetCanReload(false);
}

CDedicatedServerManager::~CDedicatedServerManager()
{
}

/**
 * Sends a stop message to all dedi
 */
void CDedicatedServerManager::Shutdown()
{
	for (auto server : m_vServerPools)
		g_PacketManager.SendHostServerStop(server->GetSocket());
}

bool CDedicatedServerManager::OnPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case HostServerPacketType::AddServer:
	{
		/// @todo make whitelist...
		int port = msg->ReadUInt16(); // -port, default is 27015
		int ip = msg->ReadUInt32(false); // ip from -hostip dedi argument

		// if IP is not specified by dedi server, use IP from socket
		if (ip == 0)
		{
			ip = ntohl(ip_string_to_int(socket->GetIP())); // big endian to little endian
			Logger().Warn("CDedicatedServerManager::OnPacket(AddServer): no IP specified, using IP: %s\n", socket->GetIP().c_str());
		}

		AddServer(socket, ip, port);

		break;
	}
	case HostServerPacketType::MemoryUsage:
	{
		CDedicatedServer* server = GetServerBySocket(socket);
		if (server)
			server->SetMemoryUsage(msg->ReadUInt16());

		break;
	}
	case HostServerPacketType::Unk2:
	{
		// Something related to a file named "rev.txt", what the fuck?
		int unk = msg->ReadUInt32();
		Logger().Warn("CDedicatedServerManager::OnPacket(2): %d\n", unk);

		break;
	}
	case HostServerPacketType::Unk3:
	{
		// I think it's a string
		std::string unk = msg->ReadString();
		Logger().Warn("CDedicatedServerManager::OnPacket(3): %s\n", unk.c_str());

		break;
	}
	default:
		Logger().Warn("Unknown host server request %d\n", type);
		break;
	}

	return true;
}

/**
 * Get free dedi server and link it with room
 * @return NULL if no such dedi server
 */
CDedicatedServer* CDedicatedServerManager::GetAvailableServerFromPools(IRoom* room)
{
	for (auto server : m_vServerPools)
	{
		if (!server->GetRoom())
		{
			server->SetRoom(room);
			return server;
		}
	}

	return NULL;
}

/**
 * Checks if there is a free dedi server to play
 */
bool CDedicatedServerManager::IsPoolAvailable()
{
	for (auto server : m_vServerPools)
		if (!server->GetRoom())
			return true;

	return false;
}

/**
 * Create and add dedi server object to the pool
 */
void CDedicatedServerManager::AddServer(IExtendedSocket* socket, int ip, int port)
{
	if (GetServerBySocket(socket))
	{
		Logger().Error("CDedicatedServerManager::AddServer: %s:%d duplicate\n", socket->GetIP(), ntohs(port));
		return;
	}

	CDedicatedServer* server = new CDedicatedServer(socket, ip, port);
	m_vServerPools.push_back(server);
}

CDedicatedServer* CDedicatedServerManager::GetServerBySocket(IExtendedSocket* socket)
{
	for (auto server : m_vServerPools)
		if (server->GetSocket() == socket)
			return server;

	return NULL;
}

/**
 * Removes dedicated server from pool if it crashes or just turns off
 */
void CDedicatedServerManager::RemoveServer(IExtendedSocket* socket)
{
	CDedicatedServer* server = GetServerBySocket(socket);
	if (!server)
		return;

	IRoom* room = server->GetRoom();
	if (room)
	{
		room->SetServer(NULL);

		if (room->GetGameMatch())
			room->EndGame(true);
	}

	delete server;
	m_vServerPools.erase(remove(m_vServerPools.begin(), m_vServerPools.end(), server), m_vServerPools.end());
}

/**
 * Connects dedi server to another master server
 */
void CDedicatedServerManager::TransferServer(IExtendedSocket* socket, const std::string& ipAddress, int port)
{
	CDedicatedServer* server = GetServerBySocket(socket);
	if (!server)
		return;

	g_PacketManager.SendHostServerTransfer(socket, ipAddress, port);
}