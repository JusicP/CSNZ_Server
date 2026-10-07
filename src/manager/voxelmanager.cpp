#include "voxelmanager.h"
#include "packetmanager.h"
#include "serverconfig.h"
#include "common/utils.h"

using namespace std;

#define VOXELCONFIG_LIST_VERSION 1

CVoxelManager g_VoxelManager;

CVoxelManager::CVoxelManager() : CBaseManager("VoxelManager")
{
}

CVoxelManager::~CVoxelManager()
{
}

bool CVoxelManager::Init()
{
	if (!LoadVoxelConfigList())
		return false;

	return true;
}

void CVoxelManager::Shutdown()
{
	m_VoxelConfigList.clear();
}

bool CVoxelManager::LoadVoxelConfigList()
{
	try
	{
		ifstream f("Data/VoxelConfigList.json");
		ordered_json cfg = ordered_json::parse(f, nullptr, false, true);

		if (cfg.is_discarded())
		{
			Logger().Fatal("CUserManager::VoxelConfigList: couldn't load Data/VoxelConfigList.json.\n");
			return false;
		}

		int version = cfg.value("Version", 0);
		if (version != VOXELCONFIG_LIST_VERSION)
		{
			Logger().Fatal("CUserManager::VoxelConfigList: %d != VOXELCONFIG_LIST_VERSION(%d)\n", version, VOXELCONFIG_LIST_VERSION);
			return false;
		}

		json voxelConfigList = cfg["VoxelConfigList"];

		for (auto& voxelConfig : voxelConfigList)
		{
			VoxelConfig voxelCfg;
			voxelCfg.id = voxelConfig.value("ID", 0);
			voxelCfg.vxlURL = voxelConfig.value("VxlURL", "");
			voxelCfg.vmgURL = voxelConfig.value("VmgURL", "");

			json httpIPList = voxelConfig["HTTPIPList"];

			for (auto& httpIP : httpIPList)
			{
				VoxelHTTP voxelHTTP;
				voxelHTTP.ip = httpIP.value("IP", "");

				json ports = httpIP["Ports"];
				for (auto& port : ports)
				{
					voxelHTTP.ports.push_back(port);
				}

				voxelCfg.httpIPList.push_back(voxelHTTP);
			}

			m_VoxelConfigList.push_back(voxelCfg);
		}
	}
	catch (exception& ex)
	{
		Logger().Fatal("CUserManager::LoadVoxelConfigList: an error occured while parsing Data/VoxelConfigList.json: %s\n", ex.what());
		return false;
	}

	return true;
}

std::vector<VoxelConfig> CVoxelManager::GetVoxelConfigList()
{
	return m_VoxelConfigList;
}

bool CVoxelManager::OnPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case 4:
		g_PacketManager.SendVoxelUnk4(socket);
		break;
	case 8:
		g_PacketManager.SendVoxelUnk8(socket);
		break;
	case 9:
		g_PacketManager.SendVoxelUnk9(socket);
		break;
	case 10:
		g_PacketManager.SendVoxelUnk10(socket);
		break;
	case 38:
		g_PacketManager.SendVoxelUnk38(socket);
		break;
	case 46:
		g_PacketManager.SendVoxelUnk46(socket);
		break;
	case 47:
		g_PacketManager.SendVoxelUnk47(socket);
		break;
	case 58:
		g_PacketManager.SendVoxelUnk58(socket);
		break;
	default:
		Logger().Warn("Unknown voxel request %d\n", type);
		break;
	}

	return true;
}

static const int TIMEOUT = 3000;

std::string CVoxelManager::GetSlotDetails(const std::string& slotId, int serverId)
{
	if (serverId >= m_VoxelConfigList.size())
	{
		Logger().Warn("CVoxelManager::GetSlotDetails: ServerID is out of range.\n");
		return "";
	}

	for (int i = 0; i < m_VoxelConfigList[serverId].httpIPList.size(); i++)
	{
		for (int j = 0; j < m_VoxelConfigList[serverId].httpIPList[i].ports.size(); j++)
		{
			sockaddr_in servaddr;
			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			if (inet_pton(AF_INET, m_VoxelConfigList[serverId].httpIPList[i].ip.substr(7).c_str(), &servaddr.sin_addr) == 0)
			{
				Logger().Warn("CVoxelManager::GetSlotDetails: Error parsing host address.\n");
				continue;
			}
			servaddr.sin_port = htons(m_VoxelConfigList[serverId].httpIPList[i].ports[j]);

			SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&TIMEOUT), sizeof(TIMEOUT));

			if (sock < 0)
			{
				Logger().Warn("CVoxelManager::GetSlotDetails: Error creating socket.\n");
				continue;
			}

			if (connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
			{
				closesocket(sock);
				Logger().Warn("CVoxelManager::GetSlotDetails: Could not connect.\n");
				continue;
			}

			std::stringstream ss;
			ss << "GET /v6/slots/detail/" << slotId.c_str() << " HTTP/1.1\r\n"
				<< "Connection: Keep-Alive\r\n"
				<< "User-Agent: cpprestsdk/2.10.2\r\n"
				<< "Host: " << m_VoxelConfigList[serverId].httpIPList[i].ip.substr(7).c_str() << ":" << m_VoxelConfigList[serverId].httpIPList[i].ports[j] << "\r\n"
				<< "\r\n\r\n";
			std::string request = ss.str();

			if (send(sock, request.c_str(), (int)request.length(), 0) != (int)request.length())
			{
				closesocket(sock);
				Logger().Warn("CVoxelManager::GetSlotDetails: Error sending request.\n");
				continue;
			}

			std::string response;
			char cur;
			bool found = false;
			while (recv(sock, &cur, 1, 0) > 0)
			{
				if (!found && cur == '{')
					found = true;

				if (found)
					response += cur;
			}

			closesocket(sock);
			return response;
		}
	}

	return "";
}