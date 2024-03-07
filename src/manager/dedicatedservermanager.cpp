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

	g_pUserManager->SendMetadata(socket);
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
	case 0:
	{
		int port = msg->ReadUInt16();
		int ip = msg->ReadUInt32(false);

		g_pDedicatedServerManager->AddServer(new CDedicatedServer(socket, ip, port));

		break;
	}
	case 1:
	{
		CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
		if (server)
			server->SetMemoryUsage(msg->ReadUInt16());

		break;
	}
	case 2:
	{
		int unk = msg->ReadUInt32();
		Console().Warn("CDedicatedServerManager::OnPacket(2): %d\n", unk);

		break;
	}
	default:
		Console().Warn("Unknown host server request %d\n", type);
		break;
	}

	return true;
}

CDedicatedServerManager::CDedicatedServerManager() : CBaseManager("DedicatedServerManager")
{
}

CDedicatedServerManager::~CDedicatedServerManager()
{
	printf("~CItemManager\n");
	Shutdown();
}

void CDedicatedServerManager::Shutdown()
{
	std::lock_guard<std::mutex> lg(hMutex);

	for (auto s : vServerPools)
		g_pPacketManager->SendHostServerStop(s->GetSocket());
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
