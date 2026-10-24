#include "voxelmanager.h"
#include "packetmanager.h"
#include "serverconfig.h"
#include "common/utils.h"
#include <regex>

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
			Logger().Fatal("CVoxelManager::LoadVoxelConfigList: couldn't load Data/VoxelConfigList.json.\n");
			return false;
		}

		int version = cfg.value("Version", 0);
		if (version != VOXELCONFIG_LIST_VERSION)
		{
			Logger().Fatal("CVoxelManager::LoadVoxelConfigList: %d != VOXELCONFIG_LIST_VERSION(%d)\n", version, VOXELCONFIG_LIST_VERSION);
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
		Logger().Fatal("CVoxelManager::LoadVoxelConfigList: an error occured while parsing Data/VoxelConfigList.json: %s\n", ex.what());
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

bool isIP(std::string host)
{
	auto pos = host.find("://");
	if (pos != std::string::npos)
		host = host.substr(pos + 3);

	host = host.substr(0, host.find_first_of("/:?#"));

	in_addr ipv4;
	in6_addr ipv6;

	return inet_pton(AF_INET, host.c_str(), &ipv4) == 1 ||
		inet_pton(AF_INET6, host.c_str(), &ipv6) == 1;
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
			bool isIp = isIP(m_VoxelConfigList[serverId].httpIPList[i].ip);
			struct hostent* he;

			if (!isIp)
			{
				std::string voxelHttpDomain;
				std::regex r("https?:\\/\\/(?:www\\.)?([-a-zA-Z0-9@:%._\\+~#=]{1,256})");
				std::smatch sm;
				regex_search(m_VoxelConfigList[serverId].httpIPList[i].ip, sm, r);
				voxelHttpDomain = sm[1];

				he = gethostbyname(voxelHttpDomain.c_str());
				if (he == NULL)
				{
					Logger().Warn("CVoxelManager::GetSlotDetails: Error getting IP address by hostname.\n");
					continue;
				}
			}

			sockaddr_in servaddr;
			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			if (inet_pton(AF_INET, isIp ? m_VoxelConfigList[serverId].httpIPList[i].ip.substr(7).c_str() : inet_ntoa(*((struct in_addr*)he->h_addr_list[0])), &servaddr.sin_addr) == 0)
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
			char buffer[8192];
			ssize_t n;

			while (response.find("\r\n\r\n") == std::string::npos)
			{
				n = recv(sock, buffer, sizeof(buffer), 0);
				if (n <= 0)
					break;

				response.append(buffer, n);
			}

			size_t bodyStart = response.find("\r\n\r\n") + 4;

			string responseLower = response;
			transform(responseLower.begin(), responseLower.end(), responseLower.begin(),
				[](unsigned char c) { return tolower(c); });

			size_t pos = responseLower.find("content-length: ");
			if (pos == std::string::npos)
				continue;

			size_t contentLength = stoul(response.substr(pos + 16));

			while (response.size() - bodyStart < contentLength)
			{
				n = recv(sock, buffer, sizeof(buffer), 0);
				if (n <= 0)
					break;

				response.append(buffer, n);
			}

			std::string body = response.substr(bodyStart, contentLength);

			closesocket(sock);

			return body;
		}
	}

	return "";
}