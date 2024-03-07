#include "usermanager.h"
#include "channelmanager.h"
#include "userdatabase.h"
#include "itemmanager.h"
#include "packetmanager.h"
#include "shopmanager.h"
#include "minigamemanager.h"
#include "luckyitemmanager.h"
#include "clanmanager.h"
#include "questmanager.h"

#include "user/userinventoryitem.h"

#include "csvtable.h"
#include "serverinstance.h"
#include "serverconfig.h"

using namespace std;

#define SUPPORTED_CLIENT_BUILD "24.12.23"

CUserManager::CUserManager(int maxPlayers) : CBaseManager("UserManager", true)
{
	m_Users.reserve(maxPlayers);

	for (size_t i = 0; i < g_pServerConfig->defUser.defaultItems.size(); i++)
		m_DefaultItems.push_back(CUserInventoryItem(i, g_pServerConfig->defUser.defaultItems[i], 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, {}, 0, 0, 0));
}

bool CUserManager::OnLoginPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	Console().Log("Client (%s) sent login packet\n", socket->GetIP().c_str());

	string steamID = msg->ReadString();
	int size = msg->ReadUInt16();
	vector<unsigned char> authSessionTicket = msg->ReadArray(size); // AuthSessionTicket - Reference: https://partner.steamgames.com/doc/features/auth
	vector<unsigned char> hwid = msg->ReadArray(16);
	int pcBang = msg->ReadUInt32(); // PCBang (PC Cafe) identification by running executable
	int ip = msg->ReadUInt32();
	string locale = msg->ReadString();

	socket->SetHWID(hwid);

	if (g_pUserDatabase->IsHWIDBanned(hwid))
	{
		Console().Log("Client (%s) disconnected from the server due to banned HWID\n", socket->GetIP().c_str());

		g_pServerInstance->DisconnectClient(socket);

		return true;
	}

	if (g_pServerConfig->crypt)
	{
		if (!socket->SetupCrypt())
		{
			Console().Log("Client (%s) disconnected from the server (Crypt failed)\n", socket->GetIP().c_str());

			g_pServerInstance->DisconnectClient(socket);

			return true;
		}

		unsigned char* key = socket->GetCryptKey();
		unsigned char* iv = socket->GetCryptIV();

		g_pPacketManager->SendCrypt(socket, 0, key, iv);

		socket->SetCryptOutput(true);

		g_pPacketManager->SendCrypt(socket, 1, key, iv);
	}

	return true;
}

bool CUserManager::OnUdpPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int unk = msg->ReadUInt8();
	if (unk == 0)
	{
		// nothing to read
		Console().Log(OBFUSCATE("CUserManager::OnUdpPacket: received type 0\n"));
	}
	else if (unk == 1)
	{
		int unk = msg->ReadUInt32();
		Console().Log(OBFUSCATE("CUserManager::OnUdpPacket: received type 1: %d\n"), unk);
	}
	else if (unk == 2)
	{
		int unk2 = msg->ReadUInt8();
		if (unk2 == 1)
		{
			int ip = msg->ReadUInt32();
			int unk3 = msg->ReadUInt16();
			int unk4 = msg->ReadUInt16();

			//user->GetNetworkConfig().m_szLocalIpAddress = ip_to_string(ip);

			Console().Log(OBFUSCATE("CUserManager::OnUdpPacket: received type 2-1: ip: %d, unk: %d, unk: %d\n"), ip, unk3, unk4);
		}
		else if (unk2 == 2)
		{
			int userID = msg->ReadUInt32();
			int unk3 = msg->ReadUInt16();

			Console().Log(OBFUSCATE("CUserManager::OnUdpPacket: received type 2-2: userID: %d, unk: %d\n"), userID, unk3);
		}
	}

	return true;
}

bool CUserManager::OnOptionPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case 0: // save cfg
	{
		CUserCharacterExtended character(EXT_UFLAG_CONFIG);
		character.config = msg->ReadArray(msg->ReadUInt16());

		g_pUserDatabase->UpdateCharacterExtended(user->GetID(), character);
		break;
	}
	case 2: // called when joining the game
		break;
	default:
		Console().Log(OBFUSCATE("[User '%s'] CUserManager::OnOptionPacket: unknown request %d\n"), user->GetLogName(), type);
		break;
	}

	return true;
}

bool CUserManager::OnVersionPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	if (!GetUserBySocket(socket))
	{
		int launcherVersion = msg->ReadUInt8();  // const 67
		int gameVersion = msg->ReadUInt16(); // const 26
		int clientBuildTimestamp = msg->ReadUInt32();
		int clientNARCRC = msg->ReadUInt32();

		if (g_pServerConfig->checkClientBuild && ((clientBuildTimestamp != g_pServerConfig->allowedClientTimestamp) || (launcherVersion != g_pServerConfig->allowedLauncherVersion)))
		{
			Console().Log(OBFUSCATE("CUserManager::OnVersionPacket: user joined with outdated client build, rejecting...\n"));

			g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("You cannot log on due to invalid client version.\nPatch the client and try it again."));

#ifndef PUBLIC_RELEASE
			Console().Warn("[SuspectNotice] detected suspect user '%s', reason: 1, %s\n", socket->GetIP().c_str(), launcherVersion != g_pServerConfig->allowedLauncherVersion ? "launcher version mismatch" : "client libraries timestamp mismatch");

			/// @todo get HWID by ip
			//g_pUserDatabase->SuspectAddAction(socket->GetIP(), 1);
#endif
			g_pServerInstance->DisconnectClient(socket);
		}

		GuestData_s& data = socket->GetGuestData();
		if (!data.isGuest)
			g_pPacketManager->SendVersion(socket, 0);
		else
			SendGuestUserPacket(socket);

		data.isGuest = true;
		data.launcherVersion = launcherVersion;

		static char dateStr[30];
		time_t t = clientBuildTimestamp;
		tm* unk3_date = localtime(&t);
		strftime(dateStr, sizeof(dateStr), "%d.%m.%y", unk3_date);

		if (strcmp(dateStr, SUPPORTED_CLIENT_BUILD))
			Console().Warn("CUserManager::OnVersionPacket: the server may not support the client build: %s (supported build: %s)\n", dateStr, SUPPORTED_CLIENT_BUILD);
	}

	return true;
}

bool CUserManager::OnFavoritePacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case FavoritePacketType::SetBuyMenu:
		return OnFavoriteSetBuyMenu(msg, user);
	case FavoritePacketType::SetFastBuy:
		return OnFavoriteSetFastBuy(msg, user); // obsolete
	case FavoritePacketType::SetLoadout:
		return OnFavoriteSetLoadout(msg, user);
	case FavoritePacketType::SetBookmark:
		return OnFavoriteSetBookmark(msg, user);
	default:
		Console().Warn("CUserManager::OnFavoritePacket: unknown request %d\n", type);
		break;
	};

	return false;
}

bool CUserManager::OnFavoriteSetFastBuy(CReceivePacket* msg, IUser* user)
{
	vector<int> fastBuyItems;
	int fastBuySlot = msg->ReadInt8();
	string fastBuyName = msg->ReadString();

	if (fastBuySlot > 4)
		return false;

	for (int i = 0; i < 11; i++)
		fastBuyItems.push_back(msg->ReadUInt16());

	g_pUserDatabase->UpdateFastBuy(user->GetID(), fastBuySlot, fastBuyName, fastBuyItems);

	return true;
}

bool CUserManager::OnFavoriteSetBookmark(CReceivePacket* msg, IUser* user)
{
	int bookmarkSlot = msg->ReadUInt8();
	int itemID = msg->ReadUInt16();

	if (bookmarkSlot > 8)
	{
		Console().Warn("OnFavoriteSetBookmark: bookmarkSlot(%d) > 8\n", bookmarkSlot);
		return false;
	}

	g_pUserDatabase->UpdateBookmark(user->GetID(), bookmarkSlot, itemID);

	return true;
}

void CUserManager::SendUserInventory(IUser* user)
{
	vector<CUserInventoryItem> items;
	g_pUserDatabase->GetInventoryItems(user->GetID(), items);

	g_pPacketManager->SendDefaultItems(user->GetExtendedSocket(), m_DefaultItems);
	g_pPacketManager->SendInventoryAdd(user->GetExtendedSocket(), items);
}

bool CUserManager::OnFavoriteSetLoadout(CReceivePacket* msg, IUser* user)
{
	int loadoutID = 0;
	int itemID = 0;

	int loadoutType = msg->ReadUInt8(); // нада как-то узнать что там
	if (loadoutType == 1) // switch current loadout
		loadoutID = msg->ReadUInt8();
	else
		itemID = msg->ReadUInt16();

	CUserCharacterExtended character = user->GetCharacterExtended(EXT_UFLAG_CURLOADOUT);

	if (loadoutType == 1)
	{
		if (loadoutID > LOADOUT_COUNT)
		{
			Console().Log(OBFUSCATE("CUserManager::OnFavoriteSetLoadout: invalid loadout %d\n"), loadoutID);
			return false;
		}

		// change current loadout
		character.flag = EXT_UFLAG_CURLOADOUT;
		character.curLoadout = loadoutID;
		g_pUserDatabase->UpdateCharacterExtended(user->GetID(), character);
		return true;
	}
	else if (loadoutType == (character.curLoadout + 1) * 10 ||
		loadoutType == (character.curLoadout + 1) * 10 + 1 ||
		loadoutType == (character.curLoadout + 1) * 10 + 2 ||
		loadoutType == (character.curLoadout + 1) * 10 + 3)
	{
		int slot = loadoutType - (character.curLoadout + 1) * 10;
		
		if (character.curLoadout > LOADOUT_COUNT)
		{
			Console().Log(OBFUSCATE("CUserManager::OnFavoriteSetLoadout: invalid loadout %d\n"), character.curLoadout);
			return false;
		}

		if (slot > LOADOUT_SLOT_COUNT)
		{
			Console().Log(OBFUSCATE("CUserManager::OnFavoriteSetLoadout: invalid slot %d\n"), slot);
			return false;
		}

		vector<CUserInventoryItem> items;
		g_pUserDatabase->GetInventoryItemsByID(user->GetID(), itemID, items);

		if (items.empty())
			return false;

		string className = g_pItemTable->GetCell<string>("ClassName", to_string(itemID));

		if (className != "Equipment")
			return false;

		g_pUserDatabase->UpdateLoadout(user->GetID(), character.curLoadout, slot, itemID);
		return true;
	}
	else if (loadoutType == 0)
	{
		vector<CUserInventoryItem> items;
		g_pUserDatabase->GetInventoryItemsByID(user->GetID(), itemID, items);

		if (items.empty())
			return false;

		string className = g_pItemTable->GetCell<string>("ClassName", to_string(itemID));

		if (className != "Class")
			return false;

		// change bg character...
		character.flag = EXT_UFLAG_CHARACTERID;
		character.characterID = itemID;
		g_pUserDatabase->UpdateCharacterExtended(user->GetID(), character);
	}
	else
	{
		Console().Warn("CUserManager::OnFavoriteSetLoadout: unknown loadout type: %d\n", loadoutType);
	}

	return true;
}

bool CUserManager::OnFavoriteSetBuyMenu(CReceivePacket* msg, IUser* user)
{
	int subMenuID = msg->ReadUInt8();
	int subMenuSlot = msg->ReadUInt8();
	int itemID = msg->ReadUInt16();

	if (subMenuID > 17)
	{
		Console().Log(OBFUSCATE("CUserManager::OnFavoriteSetBuyMenu: invalid subMenuId %d\n"), subMenuID);
		return false;
	}

	if (subMenuSlot > 9)
	{
		Console().Log(OBFUSCATE("CUserManager::OnFavoriteSetBuyMenu: invalid subMenuSlot %d\n"), subMenuSlot);
		return false;
	}

	g_pUserDatabase->UpdateBuyMenu(user->GetID(), subMenuID, subMenuSlot, itemID);

	Console().Log(OBFUSCATE("User '%d' updated buy menu, %d, %d, %d\n"), user->GetID(), subMenuID, subMenuSlot, itemID);

	return true;
}

int CUserManager::ChangeUserNickname(IUser* user, const string& newNickname, bool createCharacter)
{
	if (newNickname.size() < 4)
		return -1;
	else if (newNickname.size() > 16)
		return -2;
	else if (g_pUserDatabase->IsUserExists(newNickname, false))
		return -3;
	else if (newNickname.find_first_not_of((const char*)OBFUSCATE("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890")) != string::npos)
		return -4;
	else if (findCaseInsensitive(newNickname, g_pServerConfig->nameBlacklist))
		return -4;

	if (createCharacter)
	{
		if (!user->CreateCharacter(newNickname))
			return 0;

		// give	a user pseudo default items
		for (auto itemID : g_pServerConfig->defUser.pseudoDefaultItems)
			g_pItemManager->AddItem(user->GetID(), user, itemID, 1, 0);

		g_pItemManager->GiveReward(user->GetID(), user, 1001); // lvl box 1
	}
	else
	{
		// set new nickname
		user->UpdateGameName(newNickname);
	}

	return 1;
}

vector<CUserInventoryItem>& CUserManager::GetDefaultInventoryItems()
{
	return m_DefaultItems;
}

void CUserManager::SendGuestUserPacket(IExtendedSocket* socket)
{
	g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Welcome to the CSN:S server. Enter /login <username> <password> to login to your account."));
	g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("If you don't have an account enter /register <username> <password>"));
	g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Server developers: Jusic, Hardee, NekoMeow, Smilex_Gamer, xRiseless. Our Discord: https://discord.gg/EvUAY6D"));
}

void CUserManager::SendLoginPacket(IUser* user, const CUserCharacter& character)
{
	IExtendedSocket* socket = user->GetExtendedSocket();

	g_pPacketManager->SendUserStart(socket, user->GetID(), user->GetUsername(), character.gameName, true);
	g_pPacketManager->SendUserUpdateInfo(socket, user, character);

	CUserCharacterExtended characterExtended = user->GetCharacterExtended(EXT_UFLAG_CONFIG | EXT_UFLAG_BANSETTINGS);
	if (characterExtended.config.size())
		g_pPacketManager->SendOption(socket, characterExtended.config);

	vector<string> banList;
	g_pUserDatabase->GetBanList(user->GetID(), banList);
	g_pPacketManager->SendBanList(socket, banList);

	g_pPacketManager->SendBanSettings(socket, characterExtended.banSettings);
	g_pPacketManager->SendBanMaxSize(socket, BANLIST_MAX_SIZE);

	SendMetadata(socket);

	g_pPacketManager->SendGameMatchInfo(socket);
	g_pPacketManager->SendGameMatchUnk(socket);
	g_pPacketManager->SendGameMatchUnk9(socket);

	//g_pPacketManager->SendQuestUnk1(socket);
	//g_pPacketManager->SendQuestUnk11(socket);

	//g_pPacketManager->SendItemUnk1(socket);
	//g_pPacketManager->SendItemUnk3(socket);

	if (g_pServerConfig->mainMenuSkinEvent > 0)
		g_pPacketManager->SendEventMainMenuSkin(socket, g_pServerConfig->mainMenuSkinEvent);

	g_pPacketManager->SendEventUnk(socket);
	g_pPacketManager->SendEventAdd(socket, g_pServerConfig->activeMiniGamesFlag);

	if (g_pServerConfig->activeMiniGamesFlag & kEventFlag_WeaponRelease)
		g_pMiniGameManager->SendWeaponReleaseUpdate(user);

	SendUserInventory(user);
	SendUserLoadout(user);
	SendUserNotices(user);

	g_pPacketManager->SendShopUpdate(socket, g_pShopManager->GetProducts());
	g_pPacketManager->SendShopRecommendedProducts(socket, g_pShopManager->GetRecommendedProducts());
	g_pPacketManager->SendShopPopularProducts(socket, g_pShopManager->GetPopularProducts());

	// CN: 欢迎来到CSN:S服务器! 我们的服务器是非商业性的, 不要相信任何人说的售卖CSOL私服的信息.\n官方Discord: https://discord.gg/EvUAY6D \n
	const char* text = OBFUSCATE("EN: Welcome to the CSN:S server! The project is non-commercial. Don't trust people trying to sell you a server.\nServer developer Discord: https://discord.gg/EvUAY6D \n");
	g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, text);

	if (!g_pServerConfig->welcomeMessage.empty())
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, g_pServerConfig->welcomeMessage);

	g_pChannelManager->JoinChannel(user, g_pChannelManager->channelServers[0]->GetID(), g_pChannelManager->channelServers[0]->GetChannels()[0]->GetID(), false);

	for (auto& survey : g_pServerConfig->surveys)
	{
		if (!g_pUserDatabase->IsSurveyAnswered(user->GetID(), survey.id))
		{
			g_pPacketManager->SendUserSurvey(socket, survey);
		}
	}

	g_pPacketManager->SendLeaguePacket(socket);
}

void CUserManager::SendMetadata(IExtendedSocket* socket)
{
	int flag = g_pServerConfig->metadataToSend;
	if (flag & kMetadataFlag_MapList)
		g_pPacketManager->SendMetadataMaplist(socket);
	if (flag & kMetadataFlag_ClientTable)
		g_pPacketManager->SendMetadataClientTable(socket);
	if (flag & kMetadataFlag_ModeList)
		g_pPacketManager->SendMetadataModelist(socket);
	if (flag & kMetadataFlag_Unk3)
		g_pPacketManager->SendMetadataUnk3(socket);
	if (flag & kMetadataFlag_ItemBox)
		g_pPacketManager->SendMetadataItemBox(socket, g_pLuckyItemManager->GetItems());
	if (flag & kMetadataFlag_WeaponPaint)
		g_pPacketManager->SendMetadataWeaponPaint(socket);
	if (flag & kMetadataFlag_Unk8)
		g_pPacketManager->SendMetadataUnk8(socket);
	if (flag & kMetadataFlag_MatchOption)
		g_pPacketManager->SendMetadataMatchOption(socket);
	if (flag & kMetadataFlag_Unk15)
		g_pPacketManager->SendMetadataUnk15(socket);
	if (flag & kMetadataFlag_WeaponParts)
		g_pPacketManager->SendMetadataWeaponParts(socket);
	if (flag & kMetadataFlag_Unk20)
		g_pPacketManager->SendMetadataUnk20(socket);
	if (flag & kMetadataFlag_Encyclopedia)
		g_pPacketManager->SendMetadataEncyclopedia(socket);
	if (flag & kMetadataFlag_GameModeList)
		g_pPacketManager->SendMetadataGameModeList(socket);
	if (flag & kMetadataFlag_ProgressUnlock)
		g_pPacketManager->SendMetadataProgressUnlock(socket);
	if (flag & kMetadataFlag_ReinforceMaxLvl)
		g_pPacketManager->SendMetadataReinforceMaxLvl(socket);
	if (flag & kMetadataFlag_ReinforceMaxEXP)
		g_pPacketManager->SendMetadataReinforceMaxEXP(socket);
	if (flag & kMetadataFlag_ReinforceItemsExp)
		g_pPacketManager->SendMetadataReinforceItemsExp(socket);
	if (flag & kMetadataFlag_Unk31)
		g_pPacketManager->SendMetadataUnk31(socket); // client crash without this
	if (flag & kMetadataFlag_HonorMoneyShop)
		g_pPacketManager->SendMetadataHonorMoneyShop(socket);
	if (flag & kMetadataFlag_ItemExpireTime)
		g_pPacketManager->SendMetadataItemExpireTime(socket);
	if (flag & kMetadataFlag_ScenarioTX_Common)
		g_pPacketManager->SendMetadataScenarioTX_Common(socket);
	if (flag & kMetadataFlag_ScenarioTX_Dedi)
		g_pPacketManager->SendMetadataScenarioTX_Dedi(socket);
	if (flag & kMetadataFlag_ShopItemList_Dedi)
		g_pPacketManager->SendMetadataShopItemList_Dedi(socket);
	if (flag & kMetadataFlag_ZBCompetitive)
		g_pPacketManager->SendMetadataZBCompetitive(socket);
	if (flag & kMetadataFlag_Unk43)
		g_pPacketManager->SendMetadataUnk43(socket);
	if (flag & kMetadataFlag_Unk49)
		g_pPacketManager->SendMetadataUnk49(socket);
	if (flag & kMetadataFlag_PPSystem)
		g_pPacketManager->SendMetadataPPSystem(socket);
	if (flag & kMetadataFlag_Item)
		g_pPacketManager->SendMetadataItem(socket);
	if (flag & kMetadataFlag_CodisData)
		g_pPacketManager->SendMetadataCodisData(socket);
	if (flag & kMetadataFlag_WeaponProp)
		g_pPacketManager->SendMetadataWeaponProp(socket);
	if (flag & kMetadataFlag_Hash)
		g_pPacketManager->SendMetadataHash(socket);
	if (flag & kMetadataFlag_RandomWeaponList)
		g_pPacketManager->SendMetadataRandomWeaponList(socket);
}

void CUserManager::SendUserLoadout(IUser* user)
{
	CUserLoadout loadout = {};
	g_pUserDatabase->GetLoadouts(user->GetID(), loadout);

	// unknown size error
	//vector<CUserFastBuy> fastBuy;
	//g_pUserDatabase->GetFastBuy(user->GetID(), fastBuy);

	vector<CUserBuyMenu> buyMenu;
	g_pUserDatabase->GetBuyMenu(user->GetID(), buyMenu);

	CUserCharacterExtended character(EXT_UFLAG_CURLOADOUT | EXT_UFLAG_CHARACTERID);
	g_pUserDatabase->GetCharacterExtended(user->GetID(), character);

	vector<int> bookmark;
	g_pUserDatabase->GetBookmark(user->GetID(), bookmark);

	g_pPacketManager->SendFavoriteLoadout(user->GetExtendedSocket(), character.characterID, character.curLoadout, loadout);
	//g_pPacketManager->SendFavoriteFastBuy(user->GetExtendedSocket(), fastBuy);
	g_pPacketManager->SendFavoriteBuyMenu(user->GetExtendedSocket(), buyMenu);
	g_pPacketManager->SendFavoriteBookmark(user->GetExtendedSocket(), bookmark);
}

void CUserManager::SendUserNotices(IUser* user)
{
	for (auto& notice : g_pServerConfig->notices)
	{
		g_pPacketManager->SendUMsgNotice(user->GetExtendedSocket(), notice);
	}
}

bool CUserManager::OnCharacterPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	string name = msg->ReadString();

	IUser* user = GetUserBySocket(socket);

	int replyCode = ChangeUserNickname(user, name, true);
	switch (replyCode)
	{
	case 0:
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("DB_QUERY_FAILED"));
		DisconnectUser(user);
		return false;
	case 1:
		g_pPacketManager->SendReply(user->GetExtendedSocket(), ServerReply::S_REPLY_CREATEOK);
		break;
	case -1:
		g_pPacketManager->SendReply(user->GetExtendedSocket(), ServerReply::S_REPLY_CREATE_ID_TOO_SHORT);
		return false;
	case -2:
		g_pPacketManager->SendReply(user->GetExtendedSocket(), ServerReply::S_REPLY_CREATE_ID_TOO_LONG);
		return false;
	case -3:
		g_pPacketManager->SendReply(user->GetExtendedSocket(), ServerReply::S_REPLY_CREATE_ID_ALREADY_EXIST);
		return false;
	case -4:
		g_pPacketManager->SendReply(user->GetExtendedSocket(), 26); // 26? name blacklist
		return false;
	}

	CUserCharacter character = user->GetCharacter(0xFFFFFFFF);

	g_pItemManager->OnUserLogin(user);
	g_pClanManager->OnUserLogin(user);
	g_pQuestManager->OnUserLogin(user);

	SendLoginPacket(user, character);

	return true;
}

bool CUserManager::OnUserMessage(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);

	int type = msg->ReadUInt8();
	switch (type)
	{
	case UMsgReceiveType::LobbyWhisperChat:
		g_pChannelManager->OnWhisperMessage(msg, user);
		break;
	case UMsgReceiveType::LobbyChat:
		g_pChannelManager->OnLobbyMessage(msg, socket, user);
		break;
	case UMsgReceiveType::ClanChat:
		Console().Warn("CUserManager::OnUserMessage: ClanChat!\n");
		break;
	case UMsgReceiveType::RoomChat:
		g_pChannelManager->OnRoomUserMessage(msg, user);
		break;
	case UMsgReceiveType::RoomTeamChat:
		g_pChannelManager->OnRoomTeamUserMessage(msg, user);
		break;
	case UMsgReceiveType::RewardSelect:
		g_pItemManager->OnRewardSelect(msg, user);
		break;
	default:
		Console().Warn("CUserManager::OnUserMessage: unknown request %d\n", type);
		break;
	}

	return true;
}

bool CUserManager::OnUpdateInfoPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int msgType = msg->ReadUInt8();
	switch (msgType)
	{
	case UpdateInfoPacketType::RequestUpdateNickname:
		g_pItemManager->OnNicknameChangeUse(user, msg->ReadString());
		break;
	case UpdateInfoPacketType::RequestUpdateLocation:
	{
		int nation = msg->ReadUInt16();
		int city = msg->ReadUInt16();
		int town = msg->ReadUInt16();

		user->UpdateLocation(nation, city, town);
		break;
	}
	case UpdateInfoPacketType::RequestUpdateTutorial:
	{
		int tutorial = msg->ReadUInt8();
		Console().Log("RequestTutorial: %d\n", tutorial);

		break;
	}
	case 12: // called when click on inventory button
		break;
	default:
		Console().Warn("CUserManager::OnUpdateInfoPacket: unknown request %d\n", msgType);
		break;
	}

	return true;
}

void CUserManager::OnSecondTick(time_t curTime)
{
	for (auto u : m_Users)
		u->OnTick();
}

void CUserManager::SendNoticeMessageToAll(const string& msg)
{
	for (auto u : m_Users)
		g_pPacketManager->SendUMsgNoticeMessageInChat(u->GetExtendedSocket(), msg);
}

void CUserManager::SendNoticeMsgBoxToAll(const string& msg)
{
	for (auto u : m_Users)
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(u->GetExtendedSocket(), msg);
}

int CUserManager::LoginUser(IExtendedSocket* socket, const string& userName, const string& password)
{
	UserBan ban = {};
	int userID = g_pUserDatabase->Login(userName, password, socket, ban, NULL);
	if (userID <= 0)
	{
		switch (userID)
		{
		case LOGIN_USER_BANNED:
			if (ban.banType == 1) // ban with msg
				g_pPacketManager->SendUMsgSystemReply(socket, UMsgPacketType::SystemReply_MsgBox, "GM_CUT", vector<string>{ban.reason});
			break;
		}
		Console().Log(OBFUSCATE("Login failed (code: %d)\n"), userID);
		return userID;
	}

	if (GetUserById(userID)) // if user with this uid is already on server
	{
		Console().Log("Login failed (code: %d)\n", LOGIN_USER_ALREADY_LOGGED_IN_UID);

		g_pPacketManager->SendReply(socket, ServerReply::S_REPLY_PLAYING);

		return LOGIN_USER_ALREADY_LOGGED_IN_UID;
	}

	if (GetUserBySocket(socket)) // if user with the same socket object is already on server
	{
		Console().Log("Login failed (code: %d)\n", LOGIN_USER_ALREADY_LOGGED_IN_UUID);

		g_pPacketManager->SendReply(socket, ServerReply::S_REPLY_PLAYING);

		return LOGIN_USER_ALREADY_LOGGED_IN_UUID;
	}

	IUser* newUser = AddUser(socket, userID, userName);
	if (!newUser)
	{
		Console().Log("Login failed (code: %d)\n", LOGIN_SERVER_IS_FULL); // -5 (user limit)

		g_pPacketManager->SendReply(socket, ServerReply::S_REPLY_EXCEED_MAX_CONNECTION);

		return LOGIN_SERVER_IS_FULL;
	}

	Console().Log(OBFUSCATE("User logged in (IP: %s, UID: %d, Username: %s)\n"), newUser->GetNetworkConfig().m_szExternalIpAddress.c_str(), userID, userName.c_str());

	// Update last login time, last IP and last HWID
	CUserData data = newUser->GetUser(UDATA_FLAG_FIRSTLOGONTIME);
	data.flag = 0;
	if (data.firstLogonTime == 0)
	{
		data.firstLogonTime = g_pServerInstance->GetCurrentTime();
		data.flag |= UDATA_FLAG_FIRSTLOGONTIME;
	}

	data.flag |= UDATA_FLAG_LASTLOGONTIME | UDATA_FLAG_LASTIP | UDATA_FLAG_LASTHWID;
	data.lastLogonTime = g_pServerInstance->GetCurrentTime();
	data.lastIP = socket->GetIP();
	data.lastHWID = socket->GetHWID();

	g_pUserDatabase->UpdateUserData(userID, data);
		
	g_pPacketManager->SendReply(socket, ServerReply::S_REPLY_YES);

	if (!newUser->IsCharacterExists())
	{
		g_pPacketManager->SendCharacter(socket);
	}
	else
	{
		// continue login proccess
		CUserCharacter character = newUser->GetCharacter(0xFFFFFFFF);

		g_pItemManager->OnUserLogin(newUser);
		g_pClanManager->OnUserLogin(newUser);
		g_pQuestManager->OnUserLogin(newUser);

		SendLoginPacket(newUser, character);
	}

	return LOGIN_OK;
}

int CUserManager::RegisterUser(IExtendedSocket* socket, const string& userName, const string& password)
{
	if (password.size() < 5 || password.size() > 15 || password.find_first_not_of("0123456789") == string::npos)
		return REGISTER_PASSWORD_WRONG;
	if (userName.size() < 5 || userName.size() > 15 || userName.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != string::npos)
		return REGISTER_USERNAME_WRONG;

	int regResult = g_pUserDatabase->Register(userName, password, socket->GetIP());
	if (regResult < 0)
	{
		Console().Log("Register failed (code: %d)\n", regResult);
		return regResult;
	}

	Console().Log("Register ok (code: %d)\n", regResult);

	return regResult;
}

void CUserManager::DisconnectUser(IUser* user)
{
	g_pServerInstance->DisconnectClient(user->GetExtendedSocket());
}

void CUserManager::DisconnectAllFromServer()
{
	for (auto u : m_Users)
	{
		DisconnectUser(u);
	}
}

IUser* CUserManager::AddUser(IExtendedSocket* socket, int userID, const string& userName)
{
	if ((int)m_Users.size() >= g_pServerConfig->maxPlayers)
		return NULL;

	CUser* newUser = new CUser(socket, userID, userName);
	m_Users.push_back(newUser);

	return newUser;
}

IUser* CUserManager::GetUserById(int userId)
{
	for (auto u : m_Users)
	{
		if (u->GetID() == userId)
			return u;
	}

	return NULL;
}

IUser* CUserManager::GetUserBySocket(IExtendedSocket* socket)
{
	for (auto u : m_Users)
	{
		if (u->GetExtendedSocket() == socket)
			return u;
	}

	return NULL;
}

IUser* CUserManager::GetUserByUsername(const string& username)
{
	for (auto u : m_Users)
	{
		if (u->GetUsername() == username)
		{
			return u;
		}
	}

	return NULL;
}

IUser* CUserManager::GetUserByNickname(const string& nickname)
{
	int userID = g_pUserDatabase->IsUserExists(nickname, false);

	return GetUserById(userID);
}

void CUserManager::RemoveUser(IUser* user)
{
	for (auto u : m_Users)
	{
		if (u == user)
		{
			m_Users.erase(remove(begin(m_Users), end(m_Users), user), end(m_Users));

			CleanUpUser(u);
			delete u;
		}
	}
}

void CUserManager::RemoveUserById(int userId)
{
	for (auto u : m_Users)
	{
		if (u->GetID() == userId)
		{
			m_Users.erase(remove(begin(m_Users), end(m_Users), u), end(m_Users));

			CleanUpUser(u);
			delete u;
		}
	}
}

void CUserManager::RemoveUserBySocket(IExtendedSocket* socket)
{
	for (auto u : m_Users)
	{
		if (u->GetExtendedSocket() == socket)
		{
			m_Users.erase(remove(begin(m_Users), end(m_Users), u), end(m_Users));

			CleanUpUser(u);
			delete u;
		}
	}
}

void CUserManager::CleanUpUser(IUser* user)
{
	IRoom* room = user->GetCurrentRoom();
	if (room)
		room->RemoveUser(user);

	CChannel* channel = user->GetCurrentChannel();
	if (channel)
		channel->UserLeft(user);

	user->SetCurrentChannel(NULL);
}

std::vector<IUser*> CUserManager::GetUsers()
{
	return m_Users;
}

bool CUserManager::OnReportPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;
#if 0
	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int reason = msg->ReadUInt8();
	int userID = msg->ReadUInt32();
	string classificator = msg->ReadString();
	// cfg shit...., (5)ClsNotOpen, (6)BoxItem, (7)Spectator, (8)BoxItem2, (50)SPEED, Wall, W2lal, (1)Memory, (2)Class, (3)WpnCmd, (4)ClsCmd, (7)Table, spdcl, ChaHack, MOVTYP, OBS, OBSGUEST, SVDEX, QBARREL, SOCC(last char? a1 + 8), SOLID
	string object = msg->ReadString();

	Console().Log("[SuspectNotice] detected suspect user '%s', userID dest: %d, reason: %d, %s, %s\n", user->GetLogName(), userID, reason, classificator.empty() ? "NULL" : classificator.c_str(), object.empty() ? "NULL" : object.c_str());

	// E0000002 - section(ce detect)
	// E0000005 - ogl hook?
	// E0000006 - ogl hook?
	// E0000008 - ogl hook?
	// UI001, UI002, UI003 - oid/uid manipulations
	if (classificator == "spdcl" || classificator == "E0000002" || classificator == "E0000005" || classificator == "E0000006" || classificator == "UI002" || classificator == "UI003" || classificator == "UI001" || classificator == "UI000"
		|| strstr(object.c_str(), "NG.dll") /*|| (classificator == "E0000008" && !strstr(object.c_str(), "Data = 0000000000.0000000000.0000000000") && !strstr(object.c_str(), "File = "))*/)
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), "The game has detected suspicious activity on client side. Try to close apps and join connect the game again.");

		DisconnectUser(user);
	}

	/// @todo:
	g_pUserDatabase->SuspectAddAction(user->GetExtendedSocket()->GetHWID(), 0);
#endif
	return true;
}

bool CUserManager::OnAlarmPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
	{
		return false;
	}

	//vector<Notice_s> notices;
	//g_pPacketManager->SendAlarm(socket, g_pServerConfig->notices);

	//for (auto& notice : g_pServerConfig->notices)
	//{
	//	g_pPacketManager->SendUMsgNotice(socket, notice, 0);
	//}

	return true;
}

bool CUserManager::OnUserSurveyPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case 11:
		OnUserSurveyAnswerRequest(msg, user);
		break;
	case 12:
	{
		int unk = msg->ReadUInt8();
		string unk2 = msg->ReadString();
		break;
	}
	default:
		Console().Warn(OBFUSCATE("[User '%s'] CUserManager::OnUserSurveyPacket: unknown request %d (len: %d)\n"), user->GetLogName(), type, msg->GetLength());
		break;
	}

	return true;
}

bool CUserManager::OnBanPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case BanPacketType::RequestBanAddNickname:
		OnBanAddNicknameRequest(msg, user);
		break;
	case BanPacketType::RequestBanRemoveNickname:
		OnBanRemoveNicknameRequest(msg, user);
		break;
	case BanPacketType::RequestBanSettings:
		OnBanSettingsRequest(msg, user);
		break;
	default:
		Console().Warn(OBFUSCATE("[User '%s'] Unknown Packet_Ban type %d (len: %d)\n"), user->GetLogName(), type, msg->GetLength());
		break;
	}

	return true;
}

bool CUserManager::OnMessengerPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case 1: // send user info
	{
		/// @todo handle errors
		string gameName = msg->ReadString();

		int userID = g_pUserDatabase->IsUserExists(gameName, false);

		CUserCharacter character = user->GetCharacter(0xFFFFFFFF);

		g_pPacketManager->SendMessengerUserInfo(socket, userID, character);
		break;
	}
	default:
		Console().Warn(OBFUSCATE("[User '%s'] CUserManager::OnMessengerPacket: unknown request %d\n"), user->GetLogName(), type);
	}

	return true;
}

bool CUserManager::OnAddonPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	vector<int> addons;
	g_pUserDatabase->GetAddons(user->GetID(), addons);

	// update addon list on client side
	if (!addons.empty())
		g_pPacketManager->SendAddonPacket(socket, addons);

	return true;
}

bool CUserManager::OnLeaguePacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case 0:
		g_pPacketManager->SendLeaguePacket(socket);
		break;
	default:
		Console().Warn(OBFUSCATE("[User '%s'] Unknown Packet_League type %d (len: %d)\n"), user->GetLogName(), type, msg->GetLength());
		break;
	}

	return true;
}

bool CUserManager::OnCryptPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	if (g_pServerConfig->crypt)
		socket->SetCryptInput(true);

	return true;
}

void CUserManager::OnUserSurveyAnswerRequest(CReceivePacket* msg, IUser* user)
{
	UserSurveyAnswer answer;
	int surveyID = msg->ReadUInt32();
	answer.surveyID = surveyID;

	vector<Survey>::iterator surveyIt = find_if(g_pServerConfig->surveys.begin(), g_pServerConfig->surveys.end(),
		[surveyID](Survey& survey) { return survey.id == surveyID; });
	if (surveyIt == g_pServerConfig->surveys.end())
	{
		g_pPacketManager->SendUserSurveyReply(user->GetExtendedSocket(), ANSWER_INVALID);
		return;
	}

	int questionsCount = msg->ReadUInt8();
	for (int i = 0; i < questionsCount; i++)
	{
		UserSurveyQuestionAnswer questionAnswer = {};
		int questionID = msg->ReadUInt8();
		questionAnswer.questionID = questionID;

		vector<SurveyQuestion>::iterator surveyQuestion = find_if(surveyIt->questions.begin(), surveyIt->questions.end(),
			[questionID](SurveyQuestion& question) { return question.id == questionID; });
		if (surveyQuestion == surveyIt->questions.end())
		{
			g_pPacketManager->SendUserSurveyReply(user->GetExtendedSocket(), ANSWER_INVALID);
			return;
		}

		int answerType = msg->ReadUInt8();
		if (answerType == 1 || answerType == 2)
		{
			questionAnswer.answers.push_back(msg->ReadString());
		}
		else
		{
			questionAnswer.checkBox = true;
			int checkBoxAnswersCount = msg->ReadUInt8();
			if (!checkBoxAnswersCount)
			{
				g_pPacketManager->SendUserSurveyReply(user->GetExtendedSocket(), ANSWER_INVALID);
				return;
			}

			for (int k = 0; k < checkBoxAnswersCount; k++)
			{
				int answerID = msg->ReadUInt8();

				vector<SurveyQuestionAnswerCheckBox>::iterator checkBoxAnswer = find_if(surveyQuestion->answersCheckBox.begin(), surveyQuestion->answersCheckBox.end(),
					[answerID](SurveyQuestionAnswerCheckBox& answer) { return answer.id == answerID; });
				if (checkBoxAnswer == surveyQuestion->answersCheckBox.end())
				{
					g_pPacketManager->SendUserSurveyReply(user->GetExtendedSocket(), ANSWER_INVALID);
					return;
				}

				questionAnswer.answers.push_back(checkBoxAnswer->answer);
			}
		}

		answer.questionsAnswers.push_back(questionAnswer);
	}

	if (g_pUserDatabase->SurveyAnswer(user->GetID(), answer) <= 0)
	{
		g_pPacketManager->SendUserSurveyReply(user->GetExtendedSocket(), ANSWER_DB_ERROR);
	}

	g_pPacketManager->SendUserSurveyReply(user->GetExtendedSocket(), ANSWER_OK);
}

void CUserManager::OnBanAddNicknameRequest(CReceivePacket* msg, IUser* user)
{
	string gameName = msg->ReadString();

	int result = user->UpdateBanList(gameName);
	switch (result)
	{
	case 0:
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("DB_QUERY_FAILED")); // db error
		break;
	case 1:
		g_pPacketManager->SendBanUpdateList(user->GetExtendedSocket(), gameName);
		break;
	case -1:
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("BAN_ADD_FAIL_NICKNAME_NOT_EXIST"));
		break;
	case -2:
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("BAN_ADD_FAIL_ID_EXIST"));
		break;
	case -3:
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("BAN_ADD_FAIL_MAX_NUMBER_EXCESS"));
		break;
	}
}

void CUserManager::OnBanRemoveNicknameRequest(CReceivePacket* msg, IUser* user)
{
	string gameName = msg->ReadString();

	int result = user->UpdateBanList(gameName, true);
	switch (result)
	{
	case 0:
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("DB_QUERY_FAILED")); // db error
		break;
	case 1:
		g_pPacketManager->SendBanUpdateList(user->GetExtendedSocket(), gameName, true);
		break;
	case -1:
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("BAN_REMOVE_FAIL_NICKNAME_NOT_EXIST"));
		break;
	}
}

void CUserManager::OnBanSettingsRequest(CReceivePacket* msg, IUser* user)
{
	int settings = msg->ReadUInt8();

	user->UpdateBanSettings(settings);

	g_pPacketManager->SendBanSettings(user->GetExtendedSocket(), settings);
}
