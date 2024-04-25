#pragma once

#include "command.h"

void CommandHelp(CCommand* cmd, const std::vector<std::string>& args)
{
	printf("Command list:\n");

	std::vector<std::string> cmdList = CCommandList::GetInstance().GetCommandList();
	for (auto& name : cmdList)
	{
		printf("%s\n", name.c_str());
	}
}

void CommandUsers(CCommand* cmd, const std::vector<std::string>& args)
{
	Logger().Info(va("%-6s|%-32s|%-8s|%-6s|%-15s| (online: %d | connected: %d)\n",
		"UserID", "Username/IP", "Uptime", "Status", "IP Address", g_UserManager.GetUsers().size(), g_pServerInstance->GetClients().size()));

	for (auto sock : g_pServerInstance->GetClients())
	{
		IUser* user = g_UserManager.GetUserBySocket(sock);
		if (user)
		{
			Logger().Info(va("%-6d|%-32s|%-8s|%-6d|%-15s\n",
				user->GetID(),
				user->GetUsername().c_str(),
				FormatSeconds(user->GetUptime()),
				user->GetStatus(),
				user->GetExtendedSocket()->GetIP().c_str()));
		}
		else
		{
			Logger().Info(va("%-6d|%-32s|%-8s|%-6d|%-15s\n",
				-1,
				va("[Not Login] %s", sock->GetIP().c_str()),
				"-1", // todo: make a global timer?
				-1,
				sock->GetIP().c_str()));
		}
	}
}

void CommandKickAll(CCommand* cmd, const std::vector<std::string>& args)
{
	g_UserManager.DisconnectAllFromServer();
}

void CommandCrash(CCommand* cmd, const std::vector<std::string>& args)
{
	*(int*)0 = NULL;
}

void CommandShutdown(CCommand* cmd, const std::vector<std::string>& args)
{
	// kick user from game
	for (auto channel : g_ChannelManager.channelServers)
	{
		for (auto sub : channel->GetChannels())
		{
			for (auto room : sub->GetRooms())
			{
				Logger().Info("Force ending RoomID: %d game\n", room->GetID());
				room->EndGame(true);
			}
		}
	}

	g_UserManager.SendNoticeMsgBoxToAll("Server down for maintenance");

	g_pServerInstance->SetServerActive(false);
}

void CommandGiveRewardToAll(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int rewardID = stoi(args[1]);
	if (rewardID <= 0)
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	auto users = g_UserDatabase.GetUsers();
	for (auto userID : users)
	{
		g_ItemManager.GiveReward(userID, g_UserManager.GetUserById(userID), rewardID);
	}
}

void CommandSendNotice(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	std::string out;
	for (size_t i = 1; i < args.size(); i++)
	{
		out += ' ' + args[i];
	}

	g_UserManager.SendNoticeMsgBoxToAll(out);

	Logger().Info("Sent to %d online users: %s\n", g_UserManager.GetUsers().size(), out.c_str());
}

void CommandBan(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 5 || !isNumber(args[1]) || !isNumber(args[2]) || !isNumber(args[4]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	int banType = stoi(args[2]);
	std::string reason = args[3];
	int term = stoi(args[4]);

	if (!g_UserDatabase.IsUserExists(userID))
	{
		Logger().Info("[Ban] User does not exist\n");
		return;
	}

	if (!term)
		term = 999999;

	UserBan ban;
	ban.banType = banType;
	ban.reason = reason;
	ban.term = term * CSO_24_HOURS_IN_MINUTES + g_pServerInstance->GetCurrentTime(); // convert days to minutes

	IUser* user = g_UserManager.GetUserById(userID);
	if (g_UserDatabase.UpdateUserBan(userID, ban) > 0)
	{
		if (user)
			g_UserManager.DisconnectUser(user);
	}

	CUserCharacter character = {};
	character.flag = UFLAG_GAMENAME;
	g_UserDatabase.GetCharacter(userID, character);
	if (banType == 1)
	{
		g_UserManager.SendNoticeMessageToAll(va(OBFUSCATE("%s is banned. Reason: %s"), character.gameName.c_str(), reason.c_str()));
	}
}

void CommandUnban(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);

	UserBan ban = {};
	g_UserDatabase.GetUserBan(userID, ban);

	if (ban.banType)
	{
		ban = {};

		g_UserDatabase.UpdateUserBan(userID, ban);
	}
	else
	{
		Logger().Info("[UnBan] Failed to find user(%d).\n", userID);
	}
}

void CommandHban(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	if (!g_UserDatabase.IsUserExists(userID))
	{
		Logger().Info("[HBan] User does not exist\n");
		return;
	}

	CUserData data = {};
	data.flag = UDATA_FLAG_LASTHWID;
	if (g_UserDatabase.GetUserData(userID, data) > 0)
	{
		IUser* user = g_UserManager.GetUserById(userID);
		if (user)
			g_UserManager.DisconnectUser(user);

		g_UserDatabase.UpdateHWIDBanList(data.lastHWID);
	}
}

void CommandUnhban(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	std::vector<unsigned char> hwid(args[1].begin(), args[1].end());

	g_UserDatabase.UpdateHWIDBanList(hwid, true);
}

void CommandIpban(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	if (!g_UserDatabase.IsUserExists(userID))
	{
		Logger().Info(OBFUSCATE("[IPBan] User does not exist\n"));
		return;
	}

	CUserData data = {};
	data.flag = UDATA_FLAG_LASTIP;
	if (g_UserDatabase.GetUserData(userID, data) > 0)
	{
		IUser* user = g_UserManager.GetUserById(userID);
		if (user)
			g_UserManager.DisconnectUser(user);

		g_UserDatabase.UpdateIPBanList(data.lastIP);
	}
}

void CommandUnipban(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	g_UserDatabase.UpdateIPBanList(args[1], true);
}

void CommandToggleGameMaster(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	if (!g_UserDatabase.IsUserExists(userID))
	{
		Logger().Info(OBFUSCATE("[ToggleGM] User does not exist\n"));
		return;
	}

	CUserCharacterExtended character = {};
	character.flag = EXT_UFLAG_GAMEMASTER;
	g_UserDatabase.GetCharacterExtended(userID, character);

	character.gameMaster = character.gameMaster ? false : true;
	Logger().Info(OBFUSCATE("Updated '%d' gameMaster privilege to '%s'\n"), userID, character.gameMaster ? (const char*)OBFUSCATE("true") : (const char*)OBFUSCATE("false"));

	g_UserDatabase.UpdateCharacterExtended(userID, character);
}

void CommandShopReload(CCommand* cmd, const std::vector<std::string>& args)
{
	g_ShopManager.Init();
	// send shop update to users
	for (auto u : g_UserManager.GetUsers())
		g_PacketManager.SendShopUpdate(u->GetExtendedSocket(), g_ShopManager.GetProducts());

	Logger().Info("Sent shop update to: %d\n", g_UserManager.GetUsers().size());
}

void CommandDbReload(CCommand* cmd, const std::vector<std::string>& args)
{
	if (!g_pServerInstance->Reload())
	{
		Logger().Info("Failed to reload managers!\n");
		g_pServerInstance->SetServerActive(false);
		return;
	}

	// send userinfo to users
	for (auto u : g_UserManager.GetUsers())
	{
		CUserCharacter character = u->GetCharacter(UFLAG_ALL);
		g_PacketManager.SendUserUpdateInfo(u->GetExtendedSocket(), u, character);
	}

	Logger().Info("Sent user update info to: %d\n", g_UserManager.GetUsers().size());

	// send shop update to users
	for (auto u : g_UserManager.GetUsers())
		g_PacketManager.SendShopUpdate(u->GetExtendedSocket(), g_ShopManager.GetProducts());

	Logger().Info("Sent shop update to: %d\n", g_UserManager.GetUsers().size());

	Logger().Info("Managers reload successfull.\n");
}

void CommandBans(CCommand* cmd, const std::vector<std::string>& args)
{
	std::ostringstream log;
	log << va(OBFUSCATE("\n%-5s|%-5s|%-32s|%-8s|%-6s|%-7s\n"), ("UID"), ("Type"), ("Reason"), ("HWID Ban"), ("IP Ban"), ("Acc Ban"));

	std::map<int, UserBan> banList = g_UserDatabase.GetUserBanList();
	std::vector<std::string> ipBanList = g_UserDatabase.GetIPBanList();
	std::vector<std::vector<unsigned char>> hwidBanList = g_UserDatabase.GetHWIDBanList();

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
		g_UserDatabase.GetUserData(userID, data);

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

	Logger().Info(log.str().c_str());
}

void CommandGiveItem(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 3)
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int itemID = 0, userID, count = 1, duration = 0;
	if (isNumber(args[1]))
	{
		userID = stoi(args[1]);
		if (!g_UserDatabase.IsUserExists(userID))
			userID = 0;
	}
	else
	{
		userID = g_UserDatabase.IsUserExists(args[1], false);
	}

	if (!userID)
	{
		Logger().Info(OBFUSCATE("[GiveItem] User not found.\n"));
		return;
	}

	if (args.size() >= 3 && isNumber(args[2]))
		itemID = stoi(args[2]);

	if (itemID <= 0)
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	if (args.size() >= 4 && isNumber(args[3]))
		count = stoi(args[3]);

	if (args.size() >= 5 && isNumber(args[4]))
		duration = stoi(args[4]);

	IUser* user = g_UserManager.GetUserById(userID);
	int status = g_ItemManager.AddItem(userID, user, itemID, count, duration); // add permanent item by default
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
			g_PacketManager.SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice);
			g_PacketManager.SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice, "", "", true);
		}

		break;
	}
	};
}

void CommandStatus(CCommand* cmd, const std::vector<std::string>& args)
{
	Logger().Info("%s\n", g_pServerInstance->GetMainInfo());
}

void CommandSendEvent(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 3 || !isNumber(args[1]) || !isNumber(args[2]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	int num = stoi(args[2]);

	IUser* user = g_UserManager.GetUserById(userID);
	if (!user)
	{
		Logger().Info("User is offline");
		return;
	}

	g_PacketManager.SendEventAdd(user->GetExtendedSocket(), num);
}

void CommandSendEvent2(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	IUser* user = g_UserManager.GetUserById(userID);
	if (!user)
	{
		Logger().Info("User is offline");
		return;
	}

	std::vector<UserWeaponReleaseCharacter> characters;
	std::vector<UserWeaponReleaseRow> rows;
	int totalCharacterCount = 0;

	g_UserDatabase.GetWeaponReleaseCharacters(user->GetID(), characters, totalCharacterCount);
	g_UserDatabase.GetWeaponReleaseRows(user->GetID(), rows);

	g_PacketManager.SendMiniGameWeaponReleaseUpdate(user->GetExtendedSocket(), g_pServerConfig->weaponRelease, rows, characters, totalCharacterCount);
}

void CommandSendInventory(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 2 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	IUser* user = g_UserManager.GetUserById(userID);
	if (!user)
	{
		Logger().Info("User is offline");
		return;
	}

	std::vector<CUserInventoryItem> items;
	g_UserDatabase.GetInventoryItems(user->GetID(), items);

	g_PacketManager.SendDefaultItems(user->GetExtendedSocket(), g_UserManager.GetDefaultInventoryItems());
	g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);
}

void CommandSendPacket(CCommand* cmd, const std::vector<std::string>& args)
{
	if (args.size() < 3 || !isNumber(args[1]))
	{
		Logger().Info("%s\n", cmd->GetUsage().c_str());
		return;
	}

	int userID = stoi(args[1]);
	IUser* user = g_UserManager.GetUserById(userID);
	if (!user)
	{
		Logger().Info("User is offline");
		return;
	}

	std::string filename = args[2];

	g_PacketManager.SendPacketFromFile(user->GetExtendedSocket(), filename);
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
CCommand sendpacket("sendpacket", "Send packet from file to user by userID", "sendpacket <userID> <filename>", CommandSendPacket);