#include "dedicatedservermanager.h"
#include "usermanager.h"
#include "packetmanager.h"

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

bool CDedicatedServerManager::OnPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case HostServerPacketType::AddServer:
	{
		int port = msg->ReadUInt16();
		int ip = msg->ReadUInt32(false);

		g_DedicatedServerManager.AddServer(new CDedicatedServer(socket, ip, port));

		break;
	}
	case HostServerPacketType::MemoryUsage:
	{
		CDedicatedServer* server = g_DedicatedServerManager.GetServerBySocket(socket);
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
		// Supposedly it's a variable size, but it doesn't tell what size it sends and it's always sending one byte, so...
		int unk = msg->ReadUInt8();
		Logger().Warn("CDedicatedServerManager::OnPacket(3): %d\n", unk);

		break;
	}
	default:
		Logger().Warn("Unknown host server request %d\n", type);
		break;
	}

	return true;
}

CDedicatedServerManager g_DedicatedServerManager;

CDedicatedServerManager::CDedicatedServerManager() : CBaseManager("DedicatedServerManager")
{
}

CDedicatedServerManager::~CDedicatedServerManager()
{
	printf("~CDedicatedServerManager\n");
}

void CDedicatedServerManager::Shutdown()
{
	std::lock_guard<std::mutex> lg(hMutex);

	for (auto s : vServerPools)
		g_PacketManager.SendHostServerStop(s->GetSocket());
}

CDedicatedServer* CDedicatedServerManager::GetAvailableServerFromPools(IRoom* room)
{
	std::lock_guard<std::mutex> lg(hMutex);

	for (auto s : vServerPools)
	{
		if (!s->GetRoom())
		{
			s->SetRoom(room);
			return s;
		}
	}

	return NULL;
}

bool CDedicatedServerManager::IsPoolAvailable()
{
	std::lock_guard<std::mutex> lg(hMutex);

	for (auto s : vServerPools)
		if (!s->GetRoom())
			return true;

	return false;
}

void CDedicatedServerManager::AddServer(CDedicatedServer* server)
{
	std::lock_guard<std::mutex> lg(hMutex);
	vServerPools.push_back(server);
}

CDedicatedServer* CDedicatedServerManager::GetServerBySocket(IExtendedSocket* socket)
{
	std::lock_guard<std::mutex> lg(hMutex);

	for (auto s : vServerPools)
		if (s->GetSocket() == socket)
			return s;

	return NULL;
}

void CDedicatedServerManager::RemoveServer(IExtendedSocket* socket)
{
	CDedicatedServer* server = GetServerBySocket(socket);
	if (!server)
		return;

	IRoom* room = server->GetRoom();
	if (room)
		room->SetServer(NULL);

	vServerPools.erase(remove(vServerPools.begin(), vServerPools.end(), server), vServerPools.end());
}

void CDedicatedServerManager::TransferServer(IExtendedSocket* socket, const std::string& ipAddress, int port)
{
	CDedicatedServer* server = GetServerBySocket(socket);
	if (!server)
		return;

	IRoom* room = server->GetRoom();
	if (room)
		room->SetServer(NULL);

	g_PacketManager.SendHostServerTransfer(socket, ipAddress, port);
}