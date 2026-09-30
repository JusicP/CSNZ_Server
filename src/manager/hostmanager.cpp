#include "hostmanager.h"
#include "packetmanager.h"
#include "dedicatedservermanager.h"
#include "usermanager.h"
#include "userdatabase.h"
#include "itemmanager.h"
#include "classmodmanager.h"
#include "serverconfig.h"

#include "common/utils.h"

#include "user/userinventoryitem.h"

using namespace std;

CHostManager g_HostManager;

CHostManager::CHostManager() : CBaseManager("HostManager")
{
}

CHostManager::~CHostManager()
{
}

bool CHostManager::OnPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	CDedicatedServer* server = g_DedicatedServerManager.GetServerBySocket(socket);
	IUser* user = g_UserManager.GetUserBySocket(socket);
	IRoom* room = server != NULL ? server->GetRoom() : (user != NULL ? user->GetCurrentRoom() : NULL);

	if (room == NULL)
		return false;

	if (server == NULL && room->GetHostUser() != user)
		return false;

	CGameMatch* gameMatch = room->GetGameMatch();

	if (gameMatch == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case HostPacketType::GameStart:
		break;
	case HostPacketType::SaveData:
		return OnSaveData(msg, gameMatch);
	case HostPacketType::SetInventory:
		return OnSetUserInventory(msg, socket, room, gameMatch);
	case HostPacketType::UseScenItem:
		return OnUseInGameItem(msg, socket, room);
	case HostPacketType::FlyerFlock:
		return OnFlyerFlockRequest(msg, socket);
	case HostPacketType::UpdateUserStatus:
		return OnUpdateUserStatus(msg, socket, room, gameMatch);
	case HostPacketType::OnKillEvent:
		return OnKillEvent(msg, room, gameMatch);
	case HostPacketType::OnGameEnd:
		return OnGameEnd(socket);
	case HostPacketType::OnUpdateKillCounter:
		return OnUpdateKillCounter(msg, room, gameMatch);
	case HostPacketType::OnUpdateDeathCounter:
		return OnUpdateDeathCounter(msg, room, gameMatch);
	case HostPacketType::OnUpdateWinCounter:
		return OnUpdateWinCounter(msg, gameMatch);
	case HostPacketType::OnUpdateScore:
		return OnUpdateScore(msg, room, gameMatch);
	case HostPacketType::OnGameEvent:
		return OnGameEvent(msg, room, gameMatch);
	case HostPacketType::OnUserWeapon:
		return OnUserWeapon(msg, socket);
	case HostPacketType::OnUserSpawn:
		return OnUserSpawn(msg, socket);
	case HostPacketType::OnUpdateClass:
		return OnUpdateClass(msg, room, gameMatch);
	case HostPacketType::OnChangeMap:
		return OnChangeMap(msg, room);
	case 15:
	{
		int unk1 = msg->ReadUInt32();
		int unk2 = msg->ReadUInt8();

		Logger().Warn("Packet_Host 15: %d, %d\n", unk1, unk2);

		break;
	}
	case 17:
	{
		int userID = msg->ReadUInt32();
		int arraySize = msg->ReadUInt8();
		Logger().Warn("Packet_Host 17: %d, %d\n", userID, arraySize);
		for (int i = 0; i < arraySize; i++)
		{
			int unk1 = msg->ReadUInt32();
			int unk2 = msg->ReadUInt32();
			Logger().Warn("Packet_Host 17 array: %d, %d\n", unk1, unk2);
		}

		break;
	}
	case 18:
	{
		/*int unk1 = msg->ReadUInt8();
		int arraySize = msg->ReadUInt8();
		// array????
		Logger().Warn("Packet_Host 18: %d, %d\n", unk1, arraySize);
		for (int i = 0; i < arraySize; i++)
		{
			int unk1 = msg->ReadUInt32();
			int unk2 = msg->ReadUInt16();
			Logger().Warn("Packet_Host 18 array: %d, %d\n", unk1, unk2);
		}*/

		break;
	}
	case 19:
	{
		OnZbsResult(msg, socket);
		break;
	}
	case 21:
	{
		int userID = msg->ReadUInt32();
		int type = msg->ReadUInt8();
		if (type == 1)
		{
			int unk = msg->ReadUInt8();
		}
		break;
	}
	default:
		Logger().Warn("Packet_Host type %d is not implemented, len: %d\n", type, msg->GetLength());
		break;
	}

	return false;
}

bool CHostManager::OnSaveData(CReceivePacket* msg, CGameMatch* gameMatch)
{
	int saveDataSize = msg->ReadUInt16();
	if (saveDataSize <= 0)
	{
		return false;
	}

	gameMatch->SetSaveData(msg->ReadArray(saveDataSize));

	return true;
}

bool CHostManager::OnSetUserInventory(CReceivePacket* msg, IExtendedSocket* socket, IRoom* room, CGameMatch* gameMatch)
{
	int userID = msg->ReadUInt32();
	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	gameMatch->Connect(destUser);

	vector<CUserInventoryItem> inGameItems = g_UserManager.GetDefaultInventoryItems();
	g_UserDatabase.GetInventoryItems(userID, inGameItems);

	// remove slots with itemID 0, not in use and non in-game items
	inGameItems.erase(
		remove_if(
			inGameItems.begin(),
			inGameItems.end(),
			[](const CUserInventoryItem& item) -> bool {
				return !(item.m_nItemID && item.m_nInUse && g_pItemTable->GetCell<int>("InGameItem", to_string(item.m_nItemID)));
			}
		),
		inGameItems.end()
	);

	// remove weapons with "activated" status
	inGameItems.erase(
		remove_if(
			inGameItems.begin(),
			inGameItems.end(),
			[](const CUserInventoryItem& item) -> bool {
				int category = g_pItemTable->GetCell<int>("Category", to_string(item.m_nItemID));
				return ((category >= 1 && category <= 6 || category == 11) ? !item.m_nStatus : false);
			}
		),
		inGameItems.end()
	);

	CGameMatchUserStat* stat = gameMatch->GetGameUserStat(destUser);
	if (stat)
	{
		stat->UpdateItems(inGameItems);
	}

	g_PacketManager.SendHostUserInventory(socket, userID, inGameItems);


	// retain only classes
	inGameItems.erase(
		remove_if(
			inGameItems.begin(),
			inGameItems.end(),
			[](const CUserInventoryItem& item) -> bool {
				int category = g_pItemTable->GetCell<int>("Category", to_string(item.m_nItemID));
				return category != 7;
			}
		),
		inGameItems.end()
	);

	std::vector<ClassModInfo_t> infos;
	infos.reserve(inGameItems.size());

	for (auto& item : inGameItems)
	{
		if (g_ClassModManager.HasClassMod(destUser, item.m_nSlot))
		{
			ClassModInfo_t info = g_ClassModManager.GetClassModBySlot(destUser, item.m_nSlot);
			info.slotId = item.m_nItemID;
			infos.push_back(info);
			continue;
		}
		ClassModInfo_t info = g_ClassModManager.GetClassModPresetById(item.m_nItemID);
		info.slotId = item.m_nItemID;
		infos.push_back(info);
	}
	g_PacketManager.SendUserClassModInventory(socket, userID, infos);

	return true;
}

bool CHostManager::OnUseInGameItem(CReceivePacket* msg, IExtendedSocket* socket, IRoom* room)
{
	int userID = msg->ReadUInt32();
	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int itemID = msg->ReadUInt16();
	int count = msg->ReadUInt16();

	vector<CUserInventoryItem> items;
	if (!g_UserDatabase.GetInventoryItemsByID(destUser->GetID(), itemID, items))
		return false;

	g_ItemManager.UseItem(destUser, items[0].GetGameSlot());

	g_PacketManager.SendHostOnItemUse(socket, userID, items[0].m_nItemID);

	return true;
}

bool CHostManager::OnFlyerFlockRequest(CReceivePacket* msg, IExtendedSocket* socket)
{
	// TODO: rewrite
	string type = msg->ReadString();

	g_PacketManager.SendHostFlyerFlock(socket, g_pServerConfig->flockingFlyerType);

	return true;
}

bool CHostManager::OnUpdateUserStatus(CReceivePacket* msg, IExtendedSocket* socket, IRoom* room, CGameMatch* gameMatch)
{
	int userID = msg->ReadUInt32();
	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int status = msg->ReadUInt8();
	if (status == 1)
	{
		// send zbs addon info
		if (destRoom->GetSettings()->gameModeId == 15)
		{
			vector<int> addons;
			g_UserDatabase.GetAddons(userID, addons);

			if (!addons.empty())
			{
				g_PacketManager.SendHostZBAddon(socket, userID, addons);

				vector<int> newAddons;
				for (auto itemID : addons)
				{
					vector<CUserInventoryItem> items;
					if (g_UserDatabase.GetInventoryItemsByID(userID, itemID, items))
					{
						newAddons.push_back(itemID);
					}
				}

				if (newAddons.size() != addons.size())
					g_UserDatabase.SetAddons(userID, newAddons);
			}
		}
	}
	else
	{
		Logger().Info("CHostManager::OnUpdateUserStatus: got %d\n", status);
	}

	return true;
}

bool CHostManager::OnKillEvent(CReceivePacket* msg, IRoom* room, CGameMatch* gameMatch)
{
	int killerUserID = msg->ReadInt32();
	IUser* destUser = g_UserManager.GetUserById(killerUserID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	GameMatch_KillEvent killEvent;
	killEvent.killerUserID = killerUserID;
	killEvent.gunID = msg->ReadInt16();
	killEvent.killerTeam = msg->ReadInt8();
	killEvent.victimUserID = msg->ReadInt32();
	killEvent.victimTeam = msg->ReadInt8();
	killEvent.victimKillType = msg->ReadInt16();
	killEvent.killerPos[0] = msg->ReadFloat();
	killEvent.killerPos[1] = msg->ReadFloat();
	killEvent.killerPos[2] = msg->ReadFloat();
	killEvent.victimPos[0] = msg->ReadFloat();
	killEvent.victimPos[1] = msg->ReadFloat();
	killEvent.victimPos[2] = msg->ReadFloat();

	gameMatch->OnKillEvent(destUser, killEvent);

	return true;
}

bool CHostManager::OnUpdateKillCounter(CReceivePacket* msg, IRoom* room, CGameMatch* gameMatch)
{
	int userID = msg->ReadInt32();
	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int unk2 = msg->ReadInt8();
	int counter = msg->ReadInt32();
	int unk4 = msg->ReadInt8();

	gameMatch->OnUpdateKillCounter(destUser, counter);

	return true;
}

bool CHostManager::OnUpdateDeathCounter(CReceivePacket* msg, IRoom* room, CGameMatch* gameMatch)
{
	int userID = msg->ReadInt32();
	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int unk2 = msg->ReadInt8();
	int counter = msg->ReadInt32();
	int unk4 = msg->ReadInt8();

	gameMatch->OnUpdateDeathCounter(destUser, counter);

	return true;
}

bool CHostManager::OnUpdateWinCounter(CReceivePacket* msg, CGameMatch* gameMatch)
{
	int ter = msg->ReadInt8();
	int ct = msg->ReadInt8();
	int unk1 = msg->ReadInt8();
	int unk2 = msg->ReadInt8();
	int unk3 = msg->ReadInt8();
	int unk4 = msg->ReadInt8();
	int unk5 = msg->ReadInt8();

	gameMatch->OnUpdateWinCounter(ter, ct);

	return true;
}

bool CHostManager::OnUpdateScore(CReceivePacket* msg, IRoom* room, CGameMatch* gameMatch)
{
	int userID = msg->ReadInt32();
	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int score = msg->ReadInt8();
	int unk3 = msg->ReadInt8();

	gameMatch->OnUpdateScore(destUser, score);

	return true;
}

bool CHostManager::OnGameEvent(CReceivePacket* msg, IRoom* room, CGameMatch* gameMatch)
{
	int type = msg->ReadUInt8();
	int userID = msg->ReadUInt32();

	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	if (type == 0) // bomb explode
	{
		gameMatch->OnBombExplode(destUser);
	}
	else if (type == 1) // bomb defuse
	{
		gameMatch->OnBombDefuse(destUser);
	}
	else if (type == 2) // hostage escape
	{
		gameMatch->OnHostageEscape(destUser);
	}
	else if (type == 32) // monster kill
	{
		int monsterType = msg->ReadUInt32();

		// 1 - def hms 
		// 100 - def zbs zombie
		//Logger().Info(LOG_INTERNAL, CON_WARNING, OBFUSCATE("[User '%s'] Game event - monster kill: userID: %d, monsterType: %d\n"), user->GetLogName(), userID, monsterType);

		gameMatch->OnMonsterKill(destUser, monsterType);
	}
	else if (type == 34) // dropbox pickup
	{
		int rewardID = msg->ReadUInt32();

		//Logger().Info(LOG_INTERNAL, CON_WARNING, OBFUSCATE("[User '%s'] Game event - dropbox: userID: %d, rewardID: %d\n"), user->GetLogName(), userID, rewardID);
		gameMatch->OnDropBoxPickup(destUser, rewardID);
	}
	else if (type == 35) // round restart
	{
	}
	else if (type == 47) // mosquito kill event
	{
		gameMatch->OnMosquitoKill(destUser);
	}
	else if (type == 56) // kite kill event
	{
		gameMatch->OnKiteKill(destUser);
	}
	else if (type == 77)
	{
		int unk = msg->ReadUInt8();
	}
	else
	{
		// a lot of spam
		Logger().Info(OBFUSCATE("Packet_Host game event: eventID: %d, userID: %d, len: %d\n"), type, userID, msg->GetLength());
	}


	return true;
}

bool CHostManager::OnUpdateClass(CReceivePacket* msg, IRoom* room, CGameMatch* gameMatch)
{
	int userID = msg->ReadInt32();
	IUser* destUser = g_UserManager.GetUserById(userID);

	if (destUser == NULL)
		return false;

	IRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int classItemID = msg->ReadInt16();
	int unk3 = msg->ReadInt8();

	gameMatch->OnUpdateClass(destUser, classItemID);

	return true;
}

bool CHostManager::OnZbsResult(CReceivePacket* msg, IExtendedSocket* socket)
{
	Logger().Warn("CHostManager::OnZbsResult\n");

	return true;
}

bool CHostManager::OnGameEnd(IExtendedSocket* socket)
{
	CDedicatedServer* server = g_DedicatedServerManager.GetServerBySocket(socket);
	IRoom* room = server != NULL ? server->GetRoom() : g_UserManager.GetUserBySocket(socket)->GetCurrentRoom();

	Logger().Info("Room (RID: %d) ending game\n", room->GetID());

	room->EndGame(false);

	return true;
}

void CHostManager::OnHostChanged(IUser* gameMatchUser, IUser* newHost, CGameMatch* match)
{
	g_PacketManager.SendHostRestart(gameMatchUser->GetExtendedSocket(), newHost->GetID(), gameMatchUser == newHost, match);

	if (gameMatchUser != newHost)
		g_PacketManager.SendHostJoin(gameMatchUser->GetExtendedSocket(), newHost);
}

bool CHostManager::OnUserWeapon(CReceivePacket* msg, IExtendedSocket* socket)
{
	int id = msg->ReadUInt32();
	int pri = msg->ReadUInt16();
	int sec = msg->ReadUInt16();
	int kni = msg->ReadUInt16();
	int gre = msg->ReadUInt16();

	return true;
}

bool CHostManager::OnUserSpawn(CReceivePacket* msg, IExtendedSocket* socket)
{
	int id = msg->ReadUInt32();
	auto x = msg->ReadFloat();
	auto y = msg->ReadFloat();
	auto z = msg->ReadFloat();

	return true;
}

// everytime round will trigger(new round, round restart)
bool CHostManager::OnRoundStart(CReceivePacket* msg, IExtendedSocket* socket)
{
	// idk why, but something for logging i think

	return true;
}

bool CHostManager::OnChangeMap(CReceivePacket* msg, IRoom* room)
{
	auto unk1 = msg->ReadUInt64();
	auto unk2 = msg->ReadUInt64();
	int mapId = msg->ReadUInt16();

	room->ChangeMap(mapId);

	return true;
}