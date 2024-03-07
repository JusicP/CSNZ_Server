#pragma once

#include "consolecommands.h"

#include "serverconfig.h"

#include "manager/usermanager.h"
#include "manager/userdatabase.h"
#include "manager/channelmanager.h"
#include "manager/itemmanager.h"
#include "manager/packetmanager.h"
#include "manager/minigamemanager.h"
#include "manager/questmanager.h"
#include "manager/luckyitemmanager.h"
#include "manager/shopmanager.h"

#include "common/utils.h"

using namespace std;

static CCommandList g_CommandList;
CCommandList* g_pCommandList = NULL;

CCommandList::CCommandList()
{
	g_pCommandList = this;
}

CCommandList::~CCommandList()
{
	g_pCommandList = NULL;
}

vector<string> CCommandList::GetCommandList()
{
	vector<string> cmdlist;
	for (auto cmd : m_Commands)
	{
		cmdlist.push_back(cmd->GetName());
	}

	return cmdlist;
}

void CCommandList::AddCommand(CCommand* cmd)
{
	m_Commands.push_back(cmd);
}

void CCommandList::RemoveCommand(CCommand* cmd)
{
	m_Commands.erase(remove(begin(m_Commands), end(m_Commands), cmd), end(m_Commands));
}

CCommand* CCommandList::GetCommand(const string& name)
{
	for (auto cmd : m_Commands)
	{
		if (cmd->GetName() == name)
		{
			return cmd;
		}
	}

	return NULL;
}

CCommand::CCommand(const string& name, const string& desc, const string& usage, const function<void(CCommand*, const vector<string>&)>& func)
{
	m_Name = name;
	m_Description = desc;
	m_Usage = usage;
	m_Func = func;

	g_pCommandList->AddCommand(this);
}

CCommand::~CCommand()
{
	g_pCommandList->RemoveCommand(this);
}

string CCommand::GetName()
{
	return m_Name;
}

string CCommand::GetDescription()
{
	return m_Description;
}

string CCommand::GetUsage()
{
	return m_Usage;
}

void CCommand::Exec(const vector<string>& args)
{
	m_Func(this, args);
}

void CommandHelp(CCommand* cmd, const vector<string>& args)
{
	printf("Command list:\n");

	vector<string> cmdList = g_pCommandList->GetCommandList();
	for (auto& name : cmdList)
	{
		printf("%s\n", name.c_str());
	}
}

void CommandUsers(CCommand* cmd, const vector<string>& args)
{
	Console().Log(va("%-6s|%-32s|%-8s|%-6s|%-15s| (online: %d | connected: %d)\n",
		"UserID", "Username/IP", "Uptime", "Status", "IP Address", g_pUserManager->GetUsers().size(), g_pServerInstance->GetClients().size()));

	for (auto sock : g_pServerInstance->GetClients())
	{
		IUser* user = g_pUserManager->GetUserBySocket(sock);
		if (user)
		{
			Console().Log(va("%-6d|%-32s|%-8s|%-6d|%-15s\n",
				user->GetID(),
				user->GetUsername().c_str(),
				FormatSeconds(user->GetUptime()),
				user->GetStatus(),
				user->GetExtendedSocket()->GetIP().c_str()));
		}
		else
		{
			Console().Log(va("%-6d|%-32s|%-8s|%-6d|%-15s\n",
				-1,
				va("[Not Login] %s", sock->GetIP().c_str()),
				"-1", // todo: make a global timer?
				-1,
				sock->GetIP().c_str()));
		}
	}
}

void CommandKickAll(CCommand* cmd, const vector<string>& args)
{
	g_pUserManager->DisconnectAllFromServer();
}

void CommandCrash(CCommand* cmd, const vector<string>& args)
{
	*(int*)0 = NULL;
}

void CommandShutdown(CCommand* cmd, const vector<string>& args)
{
	// kick user from game
	for (auto channel : g_pChannelManager->channelServers)
	{
		for (auto sub : channel->GetChannels())
		{
			for (auto room : sub->GetRooms())
			{
				Console().Log("Force ending RoomID: %d game\n", room->GetID());
				room->EndGame();
			}
		}
	}

	g_pUserManager->SendNoticeMsgBoxToAll("Server down for maintenance");

	g_pServerInstance->SetServerActive(false);
}

void CommandGiveRewardToAll(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int rewardID = stoi(args[1]);
	if (rewardID <= 0)
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	auto users = g_pUserDatabase->GetUsers();
	for (auto userID : users)
	{
		g_pItemManager->GiveReward(userID, g_pUserManager->GetUserById(userID), rewardID);
	}
}

void CommandSendNotice(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	string out;
	for (size_t i = 1; i < args.size(); i++)
	{
		out += ' ' + args[i];
	}

	g_pUserManager->SendNoticeMsgBoxToAll(out);

	Console().Log("Sent to %d online users: %s\n", g_pUserManager->GetUsers().size(), out.c_str());
}

void CommandBan(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 5 || !isNumber(args[1]) || !isNumber(args[2]) || !isNumber(args[4]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	int banType = stoi(args[2]);
	string reason = args[3];
	int term = stoi(args[4]);

	if (!g_pUserDatabase->IsUserExists(userID))
	{
		Console().Log("[Ban] User does not exist\n");
		return;
	}

	if (!term)
		term = 999999;

	UserBan ban;
	ban.banType = banType;
	ban.reason = reason;
	ban.term = term * CSO_24_HOURS_IN_MINUTES + g_pServerInstance->GetCurrentTime(); // convert days to minutes

	IUser* user = g_pUserManager->GetUserById(userID);
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
		g_pUserManager->SendNoticeMessageToAll(va(OBFUSCATE("%s is banned. Reason: %s"), character.gameName.c_str(), reason.c_str()));
	}
}

void CommandUnban(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);

	UserBan ban = {};
	g_pUserDatabase->GetUserBan(userID, ban);

	if (ban.banType)
	{
		ban = {};

		g_pUserDatabase->UpdateUserBan(userID, ban);
	}
	else
	{
		Console().Log("[UnBan] Failed to find user(%d).\n", userID);
	}
}

void CommandHban(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	if (!g_pUserDatabase->IsUserExists(userID))
	{
		Console().Log("[HBan] User does not exist\n");
		return;
	}

	CUserData data = {};
	data.flag = UDATA_FLAG_LASTHWID;
	if (g_pUserDatabase->GetUserData(userID, data) > 0)
	{
		IUser* user = g_pUserManager->GetUserById(userID);
		if (user)
			g_pUserManager->DisconnectUser(user);

		g_pUserDatabase->UpdateHWIDBanList(data.lastHWID);
	}
}

void CommandUnhban(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	vector<unsigned char> hwid(args[1].begin(), args[1].end());

	g_pUserDatabase->UpdateHWIDBanList(hwid, true);
}

void CommandIpban(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	if (!g_pUserDatabase->IsUserExists(userID))
	{
		Console().Log(OBFUSCATE("[IPBan] User does not exist\n"));
		return;
	}

	CUserData data = {};
	data.flag = UDATA_FLAG_LASTIP;
	if (g_pUserDatabase->GetUserData(userID, data) > 0)
	{
		IUser* user = g_pUserManager->GetUserById(userID);
		if (user)
			g_pUserManager->DisconnectUser(user);

		g_pUserDatabase->UpdateIPBanList(data.lastIP);
	}
}

void CommandUnipban(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	g_pUserDatabase->UpdateIPBanList(args[1], true);
}

void CommandToggleGameMaster(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	if (!g_pUserDatabase->IsUserExists(userID))
	{
		Console().Log(OBFUSCATE("[ToggleGM] User does not exist\n"));
		return;
	}

	CUserCharacterExtended character = {};
	character.flag = EXT_UFLAG_GAMEMASTER;
	g_pUserDatabase->GetCharacterExtended(userID, character);

	character.gameMaster = character.gameMaster ? false : true;
	Console().Log(OBFUSCATE("Updated '%d' gameMaster privilege to '%s'\n"), userID, character.gameMaster ? (const char*)OBFUSCATE("true") : (const char*)OBFUSCATE("false"));

	g_pUserDatabase->UpdateCharacterExtended(userID, character);
}

void CommandShopReload(CCommand* cmd, const vector<string>& args)
{
	g_pShopManager->Init();
	// send shop update to users
	for (auto u : g_pUserManager->GetUsers())
		g_pPacketManager->SendShopUpdate(u->GetExtendedSocket(), g_pShopManager->GetProducts());

	Console().Log("Sent shop update to: %d\n", g_pUserManager->GetUsers().size());
}

void CommandDbReload(CCommand* cmd, const vector<string>& args)
{
	g_pServerInstance->Init();
	g_pUserDatabase->Init();
	g_pItemManager->Init();
	g_pShopManager->Init();
	g_pLuckyItemManager->Init();
	g_pQuestManager->Init();

	// send userinfo to users
	for (auto u : g_pUserManager->GetUsers())
	{
		CUserCharacter character = u->GetCharacter(UFLAG_ALL);
		g_pPacketManager->SendUserUpdateInfo(u->GetExtendedSocket(), u, character);
	}

	Console().Log("Sent user update info to: %d\n", g_pUserManager->GetUsers().size());

	// send shop update to users
	for (auto u : g_pUserManager->GetUsers())
		g_pPacketManager->SendShopUpdate(u->GetExtendedSocket(), g_pShopManager->GetProducts());

	Console().Log("Sent shop update to: %d\n", g_pUserManager->GetUsers().size());

	Console().Log("Database reload successfull.\n");
}

void CommandBans(CCommand* cmd, const vector<string>& args)
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

	Console().Log(log.str().c_str());
}

void CommandGiveItem(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 3)
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int itemID = 0, userID, count = 1, duration = 0;
	if (isNumber(args[1]))
	{
		userID = stoi(args[1]);
		if (!g_pUserDatabase->IsUserExists(userID))
			userID = 0;
	}
	else
	{
		userID = g_pUserDatabase->IsUserExists(args[1], false);
	}

	if (!userID)
	{
		Console().Log(OBFUSCATE("[GiveItem] User not found.\n"));
		return;
	}

	if (args.size() >= 3 && isNumber(args[2]))
		itemID = stoi(args[2]);

	if (itemID <= 0)
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	if (args.size() >= 4 && isNumber(args[3]))
		count = stoi(args[3]);

	if (args.size() >= 5 && isNumber(args[4]))
		duration = stoi(args[4]);

	IUser* user = g_pUserManager->GetUserById(userID);
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

void CommandStatus(CCommand* cmd, const vector<string>& args)
{
	Console().Log("%s\n", g_pServerInstance->GetMainInfo());
}

void CommandSendEvent(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 3 || !isNumber(args[1]) || !isNumber(args[2]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	int num = stoi(args[2]);

	IUser* user = g_pUserManager->GetUserById(userID);
	if (!user)
	{
		Console().Log("User is offline");
		return;
	}

	g_pPacketManager->SendEventAdd(user->GetExtendedSocket(), num);
}

void CommandSendEvent2(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	IUser* user = g_pUserManager->GetUserById(userID);
	if (!user)
	{
		Console().Log("User is offline");
		return;
	}

	vector<UserWeaponReleaseCharacter> characters;
	vector<UserWeaponReleaseRow> rows;
	int totalCharacterCount = 0;

	g_pUserDatabase->GetWeaponReleaseCharacters(user->GetID(), characters, totalCharacterCount);
	g_pUserDatabase->GetWeaponReleaseRows(user->GetID(), rows);

	g_pPacketManager->SendMiniGameWeaponReleaseUpdate(user->GetExtendedSocket(), g_pServerConfig->weaponRelease, rows, characters, totalCharacterCount);
}

void CommandSendInventory(CCommand* cmd, const vector<string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Console().Log("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	IUser* user = g_pUserManager->GetUserById(userID);
	if (!user)
	{
		Console().Log("User is offline");
		return;
	}

	vector<CUserInventoryItem> items;
	g_pUserDatabase->GetInventoryItems(user->GetID(), items);

	g_pPacketManager->SendDefaultItems(user->GetExtendedSocket(), g_pUserManager->GetDefaultInventoryItems());
	g_pPacketManager->SendInventoryAdd(user->GetExtendedSocket(), items);
}

CCommand help("help", "Print command list", "", CommandHelp);
CCommand users("users", "Print connected client list", "", CommandUsers);
CCommand kickall("kickall", "Kick all users", "", CommandKickAll);
CCommand crash("crash", "Crash the server", "", CommandCrash);
CCommand shutdownsrv("shutdown", "Shutdown the server", "", CommandShutdown);
CCommand giverewardtoall("giverewardtoall", "Give reward by ID to all users", "giverewardtoall <rewardID>", CommandGiveRewardToAll);
CCommand sendnotice("sendnotice", "Send notice to all users", "sendnotice <msg>", CommandSendNotice);
CCommand ban("ban", "Ban user by userID", "ban <userID> <type> <reason> <term>", CommandBan);
CCommand unban("unban", "Unban user by userID", "unban <userID>", CommandUnban);
CCommand hban("hban", "Ban hwid", "hban <hwid>", CommandHban);
CCommand unhban("unhban", "Unban hwid", "unhban <hwid>", CommandUnhban);
CCommand ipban("ipban", "Ban IP", "ipban <ip>", CommandIpban);
CCommand unipban("unipban", "Unban IP", "unipban <ip>", CommandUnipban);
CCommand togglegamemaster("togglegamemaster", "Toggle Game Master privileges of a user", "togglegamemaster <userID>", CommandToggleGameMaster);
CCommand shopreload("shopreload", "Reload shop config", "", CommandShopReload);
CCommand dbreload("dbreload", "Reload server (dangerous command)", "", CommandDbReload);
CCommand bans("bans", "Print ban list", "", CommandBans);
CCommand giveitem("giveitem", "Give item to user", "giveitem <gameName/userID> <itemID> <count> <duration>", CommandGiveItem);
CCommand status("status", "Print server status", "", CommandStatus);
CCommand sendevent("sendevent", "Send event packet", "sendevent <userID> <event>", CommandSendEvent);
CCommand sendevent2("sendevent2", "Send weapon release event update", "sendevent2 <userID>", CommandSendEvent2);
CCommand sendinventory("sendinventory", "Send inventory packet to user by userID", "sendinventory <userID>", CommandSendInventory);
