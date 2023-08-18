#include "ServerInstance.h"
#include "ReceivePacket.h"
#include "BuildNum.h"
#include "CSVTable.h"
#include "ServerConfig.h"

using namespace std;

CServerConfig* g_pServerConfig;
CNetwork* g_pNetwork;
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
CDedicatedServerManager* g_pDedicatedServerManager;
CRankManager* g_pRankManager;

CServerInstance::CServerInstance()
{
	m_nNextClientIndex = 1;
	m_nAutoSaveCounter = 0;
	m_bIsServerActive = true;
	m_CurrentTime = 0;
	m_pCurrentLocalTime = NULL;
}

void CServerInstance::Init()
{
	if (g_pServerConfig)
	{
		UnloadConfigs();
		if (!LoadConfigs())
		{
			g_pConsole->Error("Server initialization failed.\n");
			m_bIsServerActive = false;
			return;
		}

		return;
	}

	if (!LoadConfigs())
	{
		g_pConsole->Error("Server initialization failed.\n");
		m_bIsServerActive = false;
		return;
	}

	g_pNetwork = new CNetwork();
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

	g_pItemTable = new CCSVTable(OBFUSCATE("Data/item.csv"), rapidcsv::LabelParams(), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true));
	g_pMapListTable = new CCSVTable(OBFUSCATE("Data/MapList.csv"), rapidcsv::LabelParams(), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true));
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

	if (!g_pUserDatabase->Init() || !g_pNetwork->ServerInit() || !g_pNetwork->UDPInit())
	{
		g_pConsole->Error("Server initialization failed.\n");
		m_bIsServerActive = false;
		return;
	}
	else if (g_pItemTable->IsLoadFailed())
	{
		g_pConsole->Error("Server initialization failed. Couldn't load item.csv.\n");
		m_bIsServerActive = false;
		return;
	}
	else
	{
		g_pConsole->Log("Server starts listening. Server developers: Jusic, Hardee, NekoMeow. Thx to Ochii for CSO2 server.\nFor more information visit vk.com/csnz_server or discord.gg/EvUAY6D\n");
		g_pConsole->Log("Server build: %s, %s\n", build_number(),
#ifdef PUBLIC_RELEASE
			"Public Release");
#else
			"Private Release");
#endif
	}

	OnSecondTick();
}

CServerInstance::~CServerInstance()
{
	delete g_pNetwork;
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

void CServerInstance::AddBanIP(string ip, bool unban)
{
	g_pUserDatabase->UpdateIPBanList(ip, unban);
}

void CServerInstance::AddBanHWID(vector<unsigned char>& hwid, bool unban)
{
	g_pUserDatabase->UpdateHWIDBanList(hwid, unban);
}

DWORD WINAPI ListenThreadUDP(LPVOID lpParameter)
{
	while (g_pServerInstance->IsServerActive())
	{
		g_pNetwork->m_Read_fds_u = g_pNetwork->m_Master_u; // reset

		int activity = select(g_pNetwork->m_nFDmax_u + 1, &g_pNetwork->m_Read_fds_u, NULL, NULL, NULL);
		if (activity == SOCKET_ERROR)
		{
			g_pConsole->Error("select(udp) failed with error: %d.\n", WSAGetLastError());
			continue; // since select failed, we can't do any shits
		}

		g_EventCriticalSection.Enter();

		for (int i = 0; i <= g_pNetwork->m_nFDmax_u; i++)
		{ 
			if (FD_ISSET(i, &g_pNetwork->m_Read_fds_u))
			{
				if (i == g_pNetwork->m_UDPSocket)
				{
					g_pServerInstance->ReceiveUdpMessage();
				}
				else
				{
					g_pServerInstance->ReceiveUdpMessage();
				}
			}
		}

		g_EventCriticalSection.Leave();
	}

	return 1;
}

DWORD WINAPI MinuteTick(LPVOID lpParameter)
{
	while (g_pServerInstance->IsServerActive())
	{
		g_EventCriticalSection.Enter();

		Event_s ev;
		ev.type = 3;
		ev.socket = NULL;
		g_Events.push_back(ev);

		g_Event.Signal();

		g_EventCriticalSection.Leave();

		Sleep(60000);
	}

	return 1;
}

DWORD WINAPI EventThread(LPVOID lpParameter)
{
	while (g_pServerInstance->IsServerActive())
	{
		g_Event.WaitForSignal();

		g_pServerInstance->OnEvent();
	}

	return 1;
}

void CServerInstance::OnCommand(string command)
{
	istringstream iss(command);
	vector<string> args((istream_iterator<string>(iss)), istream_iterator<string>());

	if (args.size() == 0)
		return;

	if (args[0] == "users")
	{
		g_pConsole->Log(va("%-6s|%-32s|%-8s|%-6s|%-15s| (online: %d | connected: %d)\n",
			"UserID", "Username/IP", "Uptime", "Status", "IP Address", g_pUserManager->users.size(), g_pNetwork->m_Sessions.size()));

		for (auto sock : g_pNetwork->m_Sessions)
		{
			auto user = g_pUserManager->GetUserBySocket(sock);
			if (user)
			{
				g_pConsole->Log(va("%-6d|%-32s|%-8s|%-6d|%-15s\n",
					user->GetID(),
					user->GetUsername().c_str(),
					FormatSeconds(user->GetUptime()),
					user->GetStatus(),
					user->GetExtendedSocket()->GetIP().c_str()));
			}
			else
			{
				g_pConsole->Log(va("%-6d|%-32s|%-8s|%-6d|%-15s\n",
					-1,
					va("[Not Login] %s", sock->GetIP().c_str()),
					"-1", // todo: make a global timer?
					-1,
					sock->GetIP().c_str()));
			}
		}
	}
	else if (args[0] == "kickall")
	{
		g_pUserManager->DisconnectAllFromServer();
	}
	else if (args[0] == "crash")
	{
		*(int*)0 = NULL;
	}
	else if (args[0] == "shutdown")
	{
		// kick user from game
		for (auto channel : g_pChannelManager->channelServers)
		{
			for (auto sub : channel->channels)
			{
				for (auto room : sub->m_Rooms)
				{
					g_pConsole->Log("Force ending RoomID: %d game\n", room->GetID());
					room->EndGame();
				}
			}
		}

		for (auto user : g_pUserManager->users)
		{
			g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), "Server down for maintenance");
		}

		g_pServerInstance->SetServerActive(false);
	}
	else if (args[0] == "giverewardtoall")
	{
		if (!(args.size() >= 2) || !isNumber(args[1]))
		{
			printf("giverewardtoall usage: giverewardtoall <rewardID>. Give a reward from ItemRewards.txt to all users' accounts\n");
			return;
		}

		int rewardID = 0;

		iss.clear();
		iss.str(args[1]);
		iss >> rewardID;

		if (iss.fail())
			return;

		if (rewardID <= 0)
			return;

		auto users = g_pUserDatabase->GetUsers();
		for (auto userID : users)
		{
			g_pItemManager->GiveReward(userID, g_pUserManager->GetUserById(userID), rewardID);
		}
	}
	else if (args[0] == "sendnotice")
	{
		if (args.size() >= 2)
		{
			string msg(command);
			msg = msg.substr(msg.find(" ") + 1);

			for (auto user : g_pUserManager->users)
				g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), msg.c_str());

			g_pConsole->Log("Sent to %d online users: %s\n", g_pUserManager->users.size(), msg.c_str());
		}
		else
		{
			printf("sendnotice usage: sendnotice <message>. Sends notice to all online users\n");
		}
	}
	else if (args[0] == "ban")
	{
		if (args.size() != 5 || !isNumber(args[1]) || !isNumber(args[2]) || !isNumber(args[4]))
		{
			printf("ban usage: ban <userID> <type> <reason> <term>. Ban user's account\n");
			return;
		}

		int userID = atoi(args[1].c_str());
		int banType = atoi(args[2].c_str());
		string reason = args[3];
		int term = atoi(args[4].c_str());

		if (!g_pUserDatabase->IsUserExists(userID))
		{
			printf(OBFUSCATE("[Ban] User does not exist\n"));
			return;
		}

		if (!term)
			term = 999999;

		UserBan ban;
		ban.banType = banType;
		ban.reason = reason;
		ban.term = term * CSO_24_HOURS_IN_MINUTES + g_pServerInstance->GetCurrentTime(); // convert days to minutes

		CUser* user = g_pUserManager->GetUserById(userID);
		if (g_pUserDatabase->UpdateUserBan(userID, ban) > 0)
		{
			if (user)
				g_pUserManager->DisconnectUser(user);
		}

		CUserCharacter character = {};
		character.flag = UFLAG_GAMENAME;
		g_pUserDatabase->GetCharacter(userID, character);
		if (banType == 1)
		{
			for (auto u : g_pUserManager->users)
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(u->GetExtendedSocket(), va(OBFUSCATE("%s is banned. Reason: %s"), character.gameName.c_str(), reason.c_str()));
			}
		}
	}
	else if (args[0] == "unban")
	{
		if (args.size() != 2 || !isNumber(args[1]))
		{
			printf("unban usage: unban <userID>. Unban user account.\n");
			return;
		}

		int userID = atoi(args[1].c_str());

		UserBan ban = {};
		g_pUserDatabase->GetUserBan(userID, ban);

		if (ban.banType)
		{
			ban = {};

			g_pUserDatabase->UpdateUserBan(userID, ban);
		}
		else
			printf(OBFUSCATE("[UnBan] Failed to find user(%s).\n"), args[1].c_str());
	}
	else if (args[0] == "hban")
	{
		if (!(args.size() >= 2) || !isNumber(args[1]))
		{
			printf("hban usage: hban <userID>. Ban user by HWID\n");
			return;
		}

		int userID = atoi(args[1].c_str());

		if (!g_pUserDatabase->IsUserExists(userID))
		{
			printf(OBFUSCATE("[HBan] User does not exist\n"));
			return;
		}

		CUserData data = {};
		data.flag = UDATA_FLAG_LASTHWID;
		if (g_pUserDatabase->GetUserData(userID, data) > 0)
		{
			CUser* user = g_pUserManager->GetUserById(userID);
			if (user)
				g_pUserManager->DisconnectUser(user);

			g_pServerInstance->AddBanHWID(data.lastHWID);
		}
	}
	else if (args[0] == "unhban")
	{
		if (args.size() != 2)
		{
			printf("unhban usage: unhban <HWID>. Unban HWID.\n");
			return;
		}

		vector<unsigned char> hwid(args[1].begin(), args[1].end());

		g_pServerInstance->AddBanHWID(hwid, true);
	}
	else if (args[0] == "ipban")
	{
		if (!(args.size() >= 2) || !isNumber(args[1]))
		{
			printf("ipban usage: ipban <userID>. Ban user by IP\n");
			return;
		}

		int userID = atoi(args[1].c_str());

		if (!g_pUserDatabase->IsUserExists(userID))
		{
			printf(OBFUSCATE("[IPBan] User does not exist\n"));
			return;
		}

		CUserData data = {};
		data.flag = UDATA_FLAG_LASTIP;
		if (g_pUserDatabase->GetUserData(userID, data) > 0)
		{
			CUser* user = g_pUserManager->GetUserById(userID);
			if (user)
				g_pUserManager->DisconnectUser(user);

			g_pServerInstance->AddBanIP(data.lastIP);
		}
	}
	else if (args[0] == "unipban")
	{
		if (args.size() != 2)
		{
			printf("unipban usage: unipban <IP>. Unban IP Address.\n");
			return;
		}

		g_pServerInstance->AddBanIP(args[1], true);
	}
	else if (args[0] == "togglegamemaster")
	{
		if (args.size() != 2 || !isNumber(args[1]))
		{
			printf("togglegamemaster usage: togglegamemaster <userID>. Toggle Game Master privileges of a user\n");
			return;
		}

		int userID = atoi(args[1].c_str());

		if (!g_pUserDatabase->IsUserExists(userID))
		{
			printf(OBFUSCATE("[ToggleGM] User does not exist\n"));
			return;
		}

		CUserCharacterExtended character = {};
		character.flag = EXT_UFLAG_GAMEMASTER;
		g_pUserDatabase->GetCharacterExtended(userID, character);

		character.gameMaster = character.gameMaster ? false : true;
		g_pConsole->Log(OBFUSCATE("Updated '%d' gameMaster privilege to '%s'\n"), userID, character.gameMaster ? (const char*)OBFUSCATE("true") : (const char*)OBFUSCATE("false"));

		g_pUserDatabase->UpdateCharacterExtended(userID, character);
	}
	else if (args[0] == "shopreload")
	{
		g_pShopManager->Init();
		// send shop update to users
		for (auto u : g_pUserManager->users)
			g_pPacketManager->SendShopUpdate(u->GetExtendedSocket(), g_pShopManager->m_Products);

		g_pConsole->Log("Sent shop update to: %d\n", g_pUserManager->users.size());
	}
	else if (args[0] == "dbreload")
	{
		g_pServerInstance->Init();
		g_pUserDatabase->Init();
		g_pItemManager->Init();
		g_pShopManager->Init();
		g_pLuckyItemManager->Init();
		g_pQuestManager->Init();

		// send userinfo to users
		for (auto u : g_pUserManager->users)
		{
			CUserCharacter character = u->GetCharacter(UFLAG_ALL);
			g_pPacketManager->SendUserUpdateInfo(u->GetExtendedSocket(), u, character);
		}

		g_pConsole->Log("Sent user update info to: %d\n", g_pUserManager->users.size());

		// send shop update to users
		for (auto u : g_pUserManager->users)
			g_pPacketManager->SendShopUpdate(u->GetExtendedSocket(), g_pShopManager->m_Products);

		g_pConsole->Log("Sent shop update to: %d\n", g_pUserManager->users.size());

		g_pConsole->Log("Database reload successfull.\n");
	}
	else if (args[0] == "bans")
	{
		ostringstream log;
		log << va(OBFUSCATE("\n%-5s|%-5s|%-32s|%-8s|%-6s|%-7s\n"), ("UID"), ("Type"), ("Reason"), ("HWID Ban"), ("IP Ban"), ("Acc Ban"));

		map<int, UserBan> banList = g_pUserDatabase->GetUserBanList();
		vector<string> ipBanList = g_pUserDatabase->GetIPBanList();
		vector<vector<unsigned char>> hwidBanList = g_pUserDatabase->GetHWIDBanList();

		for (auto b : banList)
		{
			bool hCheck = false;
			bool iCheck = false;
			bool aCheck = false;

			int userID = b.first;
			UserBan ban = b.second;

			if (ban.banType != 0)
				aCheck = true;

			CUserData data = {};
			g_pUserDatabase->GetUserData(userID, data);

			for (auto& i : ipBanList)
			{
				if (data.lastIP == i)
					iCheck = true;
			}

			for (auto& h : hwidBanList)
			{
				if (data.lastHWID == h)
					hCheck = true;
			}

			if (hCheck || iCheck || aCheck)
				log << va(OBFUSCATE("%-5d|%-5d|%-32s|%-8s|%-6s|%-7s\n"),
					userID, ban.banType, ban.reason.c_str(),
					hCheck ? (const char*)OBFUSCATE("YES") : (const char*)OBFUSCATE("NO"),
					iCheck ? (const char*)OBFUSCATE("YES") : (const char*)OBFUSCATE("NO"),
					aCheck ? (const char*)OBFUSCATE("YES") : (const char*)OBFUSCATE("NO"));
		}

		g_pConsole->Log(log.str().c_str());
	}
	else if (args[0] == "giveitem")
	{
		if (args.size() < 3)
		{
			printf("giveitem usage: giveitem <gameName/userID> <itemID> <count> <duration>\n");
			return;
		}

		int itemID, userID, count = 1, duration = 0;

		if (isNumber(args[1]))
		{
			userID = atoi(args[1].c_str());
			if (!g_pUserDatabase->IsUserExists(userID))
				userID = 0;
		}
		else
		{
			userID = g_pUserDatabase->IsUserExists(args[1], false);
		}

		if (!userID)
		{
			printf(OBFUSCATE("[GiveItem] User not found.\n"));
			return;
		}

		if (args.size() >= 3 && isNumber(args[2]))
			itemID = atoi(args[2].c_str());

		if (itemID <= 0)
			return;

		if (args.size() >= 4 && isNumber(args[3]))
			count = atoi(args[3].c_str());

		if (args.size() >= 5 && isNumber(args[4]))
			duration = atoi(args[4].c_str());

		CUser* user = g_pUserManager->GetUserById(userID);
		int status = g_pItemManager->AddItem(userID, user, itemID, count, duration); // add permanent item by default
		switch (status)
		{
		case ITEM_ADD_INVENTORY_FULL:
			printf(OBFUSCATE("[GiveItem] User's inventory is full"));
			break;
		case ITEM_ADD_UNKNOWN_ITEMID:
			printf(OBFUSCATE("[GiveItem] Item ID does not exist in the item database"));
			break;
		case ITEM_ADD_SUCCESS:
		{
			// send notification about new item
			RewardItem rewardItem;
			rewardItem.itemID = itemID;
			rewardItem.count = count;
			rewardItem.duration = duration;

			RewardNotice rewardNotice;
			rewardNotice.rewardId = 1;
			rewardNotice.exp = 0;
			rewardNotice.points = 0;
			rewardNotice.honorPoints = 0;
			rewardNotice.items.push_back(rewardItem);

			if (!user)
			{
				//uData->customRewardsNotices.push_back(rewardNotice);
				//g_pUserDatabase->UpdateCustomRewardsNotices(uData);
			}
			else
			{
				g_pPacketManager->SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice);
				g_pPacketManager->SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice, "", "", true);
			}

			break;
		}
		};
	}
	else if (args[0] == "status")
	{
		g_pConsole->Log("%s\n", GetMemoryInfo());
	}
	else if (args[0] == "sendevent")
	{
		int num = stoi(args[1]);

		CUser* user = g_pUserManager->GetUserById(1);

		g_pPacketManager->SendEventAdd(user->GetExtendedSocket(), num);
	}
	else if (args[0] == "sendevent2")
	{
		CUser* user = g_pUserManager->GetUserById(1);

		vector<UserWeaponReleaseCharacter> characters;
		vector<UserWeaponReleaseRow> rows;
		int totalCharacterCount = 0;

		g_pUserDatabase->GetWeaponReleaseCharacters(user->GetID(), characters, totalCharacterCount);
		g_pUserDatabase->GetWeaponReleaseRows(user->GetID(), rows);

		g_pPacketManager->SendMiniGameWeaponReleaseUpdate(user->GetExtendedSocket(), g_pServerConfig->weaponRelease, rows, characters, totalCharacterCount);
	}
	else if (args[0] == "sendinventory")
	{
		CUser* user = g_pUserManager->GetUserById(1);

		vector<CUserInventoryItem> items;
		g_pUserDatabase->GetInventoryItems(user->GetID(), items);

		g_pPacketManager->SendDefaultItems(user->GetExtendedSocket(), g_pUserManager->GetDefaultInventoryItems());
		g_pPacketManager->SendInventoryAdd(user->GetExtendedSocket(), items);
	}
	else
	{
		printf("Available commands: users, kickall, crash, shutdown, status\n");
		printf("Utility commands: sendnotice, giverewardtoall, togglegamemaster\n");
		printf("Ban commands: ban, unban, ipban, unipban, hban, unhban\n");
		printf("DB commands: dbsave, dbreload, shopreload\n");
	}
}

DWORD WINAPI ReadConsoleThread(LPVOID lpParameter)
{
	while (g_pServerInstance->IsServerActive())
	{
		string cmd;
		getline(cin, cmd);

		g_EventCriticalSection.Enter();

		Event_s ev;
		ev.cmd = cmd;
		ev.type = 0;
		ev.socket = NULL;
		g_Events.push_back(ev);

		g_Event.Signal();

		g_EventCriticalSection.Leave();
	}

	return 1;
}

DWORD WINAPI ListenThread(LPVOID lpParameter)
{
	while (g_pServerInstance->IsServerActive())
	{
		g_pServerInstance->Listen();
	}

	return 1;
}

void CServerInstance::Listen()
{
	FD_ZERO(&g_pNetwork->m_Write_fds);
	FD_ZERO(&g_pNetwork->m_Read_fds);

	FD_SET(g_pNetwork->m_TCPSocket, &g_pNetwork->m_Read_fds);

	for (auto socket : g_pNetwork->m_Sessions)
	{
		if (socket->m_SendPackets.size())
			FD_SET(socket->m_Socket, &g_pNetwork->m_Write_fds);

		FD_SET(socket->m_Socket, &g_pNetwork->m_Read_fds);

		if ((int)socket->m_Socket > g_pNetwork->m_nFDmax)
			g_pNetwork->m_nFDmax = socket->m_Socket;
	}

	int activity = select(g_pNetwork->m_nFDmax + 1, &g_pNetwork->m_Read_fds, &g_pNetwork->m_Write_fds, NULL, NULL);
	if (activity == SOCKET_ERROR)
	{
		g_pConsole->Error("select() failed with error: %d\n", WSAGetLastError());
		ForceEndServer();
		SetServerActive(false);
		return;
	}

	g_EventCriticalSection.Enter();

	for (int i = 0; i <= g_pNetwork->m_nFDmax; i++)
	{
		if (FD_ISSET(i, &g_pNetwork->m_Read_fds))
		{
			if (i == g_pNetwork->m_TCPSocket) // accept new client
			{
				CExtendedSocket* sock = g_pNetwork->AcceptNewClient(m_nNextClientIndex);
				if (!sock)
				{
					Sleep(100);
					break;
				}

				g_pConsole->Log(OBFUSCATE("Client (%d, %s) has been connected to the server\n"), m_nNextClientIndex, sock->GetIP().c_str());

				if (g_pUserDatabase->IsIPBanned(sock->GetIP()))
				{
					g_pConsole->Log(OBFUSCATE("Client (%d, %s) disconnected from the server due to banned ip\n"), m_nNextClientIndex, sock->GetIP().c_str());
					DisconnectClient(sock);
					break;
				}

				m_nNextClientIndex++;
			}
			else // data from client
			{
				CExtendedSocket* s = g_pNetwork->GetExSocketBySocket(i);
				if (!s)
				{
					continue;
				}

				CReceivePacket* msg = s->Read();
				if (s->m_nReadResult == 0 || s->m_nReadResult == -1)
				{
					int bytesSent = s->m_nBytesSent;
					int bytesReceived = s->m_nBytesReceived;
					int socket = s->m_Socket;

					// clean up user
					CUser* user = g_pUserManager->GetUserBySocket(s);
					int userID = 0;
					string userName = "NULL";
					if (user)
					{
						userID = user->GetID();
						userName = user->GetUsername();
						g_pUserManager->DisconnectUser(user);
					}
					else
					{
						g_pDedicatedServerManager->RemoveServer(s);
						g_pNetwork->RemoveSocket(s);
					}

					g_pConsole->Log(OBFUSCATE("User logged out (%d, '%s', sent: %d, received: %d, %d, %d, 0x%X)\n"), userID, userName.c_str(), bytesSent, bytesReceived, socket, WSAGetLastError(), user);
				}
				else if (s->m_nReadResult == SOCKET_ERROR)
				{
					int wsaResult = WSAGetLastError();
					if (wsaResult == WSAECONNABORTED || wsaResult == WSAECONNRESET)
					{
						int bytesSent = s->m_nBytesSent;
						int bytesReceived = s->m_nBytesReceived;
						int socket = s->m_Socket;

						// clean up user
						CUser* user = g_pUserManager->GetUserBySocket(s);
						int userID = 0;
						string userName = "NULL";
						if (user)
						{
							userID = user->GetID();
							userName = user->GetUsername();
							g_pUserManager->DisconnectUser(user);
						}
						else
						{
							g_pDedicatedServerManager->RemoveServer(s);
							g_pNetwork->RemoveSocket(s);
						}

						g_pConsole->Log(OBFUSCATE("User logged out (%d, '%s', sent: %d, received: %d, %d, %d, 0x%X)\n"), userID, userName.c_str(), bytesSent, bytesReceived, socket, WSAGetLastError(), user);
					}
					else
					{
						g_pConsole->Log(OBFUSCATE("Unhandled WSA error: %d, user object will remain...\n"), wsaResult);
						g_pDedicatedServerManager->RemoveServer(s);
						g_pNetwork->RemoveSocket(s);
					}
				}
				else if (!msg)
				{
					continue;
				}
				else
				{
					Event_s ev;
					ev.type = 1;
					ev.socket = s;
					ev.msgs.push_back(s->m_pMsg);
					g_Events.push_back(ev);

					g_Event.Signal();

					s->m_pMsg = NULL;
				}
			}
		}

		if (FD_ISSET(i, &g_pNetwork->m_Write_fds))
		{
			CExtendedSocket* sock = g_pNetwork->GetExSocketBySocket(i);
			if (sock && sock->m_SendPackets.size())
			{
				CSendPacket* msg = sock->m_SendPackets[0]; // send only one packet from vector
				if (sock->Send(msg, true) <= 0)
				{
					g_pConsole->Warn("An error occurred while sending packet from queue: WSAGetLastError: %d, queue.size: %d\n", WSAGetLastError(), sock->m_SendPackets.size());
					
					DisconnectClient(sock);
				}
				else
				{
					sock->m_SendPackets.erase(sock->m_SendPackets.begin());
				}
			}
		}
	}

	g_EventCriticalSection.Leave();
}

void CServerInstance::SetServerActive(bool active)
{
	m_bIsServerActive = active;
}

bool CServerInstance::IsServerActive()
{
	return m_bIsServerActive;
}

void CServerInstance::OnEvent()
{
	g_EventCriticalSection.Enter();

	for (auto& ev : g_Events)
	{
		switch (ev.type)
		{
		case 0:
			OnCommand(ev.cmd.c_str());
			break;
		case 1:
			OnPackets(ev.socket, ev.msg, ev.msgs);
			break;
		case 2:
			OnSecondTick();
			break;
		case 3:
			OnMinuteTick();
			break;
		}
	}

	for (auto& ev : g_Events)
	{
		switch (ev.type)
		{
		case 1:
			if (!ev.msgs.size())
				g_pConsole->Warn("CServerInstance::OnEvent: !ev.msg.size(), memleak\n");

			for (auto packet : ev.msgs)
				delete packet;

			break;
		}
	}

	g_Events.clear();

	g_EventCriticalSection.Leave();
}

void CServerInstance::OnPackets(CExtendedSocket* s, CReceivePacket* msg, vector<CReceivePacket*>& msgs)
{
	if (find(g_pNetwork->m_Sessions.begin(), g_pNetwork->m_Sessions.end(), s) == g_pNetwork->m_Sessions.end())
	{
		// skip packets with deleted socket object
		return;
	}

	for (auto msg : msgs)
	{
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
		default:
			g_pConsole->Warn("Unimplemented packet: %d\n", msg->GetID());
			break;
		}
	}
}

void CServerInstance::ReceiveUdpMessage()
{
	struct sockaddr_in from;
	int fromlen = sizeof(from);

	int datalen = recvfrom(g_pNetwork->m_UDPSocket, network_data, 15000, 0, (sockaddr*)&from, &fromlen);
	if (datalen == 14)
	{
		Buffer buf(vector<unsigned char>(network_data, network_data + datalen));

		char signature = buf.readUInt8();
		if (signature != 'W')
		{
			g_pConsole->Log(OBFUSCATE("CPacketIn_UDP::Parse: signature error\n"));
			return;
		}

		int userID = buf.readUInt32_LE();
		int portID = buf.readUInt16_LE();
		long longAddr = buf.readUInt32_BE();
		string localIpAddress = ip_to_string(longAddr);
		short port = buf.readUInt16_LE();

		CUser* user = g_pUserManager->GetUserById(userID);
		if (!user)
		{
			return;
		}

		int result = user->UpdateHolepunch(portID, port, from.sin_port);
		if (result == -1)
		{
			g_pConsole->Warn("Unknown hole punch port\n");
		}

		Buffer replyBuffer;
		replyBuffer.writeUInt8('W');
		replyBuffer.writeUInt8(0);
		replyBuffer.writeUInt8(1);

		// send reply
		const vector<unsigned char>& buffer = replyBuffer.getBuffer();
		sendto(g_pNetwork->m_UDPSocket, reinterpret_cast<const char*>(&buffer[0]), buffer.size(), 0, (sockaddr*)&from, fromlen);
	}
}

void CServerInstance::OnSecondTick()
{
	// update current time
	m_CurrentTime = time(NULL);
	m_pCurrentLocalTime = localtime(&m_CurrentTime);
	m_CurrentTime /= 60; // get current time in minutes(last CSO builds use timestamp in minutes)

#if 0
	// update autosave counter
	m_nAutoSaveCounter++;

	if (m_nAutoSaveCounter == 300)
	{
		m_nAutoSaveCounter = 0;
	}
#endif

	UpdateConsoleStatus();

	g_pUserManager->OnSecondTick();
}

const char* CServerInstance::GetMemoryInfo()
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	// fuck math? should we use 1000 or 1024 as kb
	auto mem = static_cast<double>(pmc.WorkingSetSize);
	// convert to mb
	mem /= 1000000;

	if (mem >= 1000)
		g_pConsole->Error("[ALERT] Server is using more than 1G of memory.\n");

	return va("Memory usage: %.2fmb. Connected users: %d. Logged in users: %d.", mem, static_cast<int>(g_pNetwork->m_Sessions.size()), static_cast<int>(g_pUserManager->users.size()));
}

void CServerInstance::DisconnectClient(CExtendedSocket* socket)
{
	CUser* user = g_pUserManager->GetUserBySocket(socket);
	if (user)
	{
		g_pUserManager->DisconnectUser(user);
	}
	else
	{
		g_pDedicatedServerManager->RemoveServer(socket);
		g_pNetwork->RemoveSocket(socket);
	}
}

void CServerInstance::UpdateConsoleStatus()
{
	g_pConsole->SetStatus(GetMemoryInfo());
	g_pConsole->UpdateStatus();
}

time_t CServerInstance::GetCurrentTime()
{
	return m_CurrentTime; // timestamp in minutes
}

tm* CServerInstance::GetCurrentLocalTime()
{
	return m_pCurrentLocalTime;
}

void CServerInstance::OnMinuteTick()
{
	time_t curTime = g_pServerInstance->GetCurrentTime();
	tm* localTime = g_pServerInstance->GetCurrentLocalTime();

	g_pItemManager->ProcessEvents(curTime);
	g_pUserDatabase->OnMinuteTick(curTime);

	g_pConsole->Log("%s\n", GetMemoryInfo());
}
