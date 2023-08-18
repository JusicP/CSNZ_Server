#include "HostManager.h"
#include "PacketManager.h"
#include "ServerConfig.h"

using namespace std;

CHostManager::CHostManager()
{
}

bool CHostManager::OnPacket(CReceivePacket* msg, CExtendedSocket* socket)
{
	LOG_PACKET;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CUser* user = g_pUserManager->GetUserBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : (user != NULL ? user->GetCurrentRoom() : NULL);

	if (room == NULL)
		return false;

	if (server == NULL && room->GetHostUser() != user)
		return false;

	CGameMatch* gamematch = room->GetGameMatch();

	if (gamematch == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case HostPacketType::GameStart:
		room->OnGameStart();
	case HostPacketType::SaveData:
		return OnSaveData(msg, gamematch);
	case HostPacketType::SetInventory:
		return OnSetUserInventory(msg, socket);
	case HostPacketType::UseScenItem:
		return OnUseInGameItem(msg, socket);
	case HostPacketType::FlyerFlock:
		return OnFlyerFlockRequest(msg, socket);
	case HostPacketType::UpdateUserStatus:
		return OnUpdateUserStatus(msg, socket);
	case HostPacketType::OnKillEvent:
		return OnKillEvent(msg, socket);
	case HostPacketType::OnGameEnd:
		return OnGameEnd(socket);
	case HostPacketType::OnUpdateKillCounter:
		return OnUpdateKillCounter(msg, socket);
	case HostPacketType::OnUpdateDeathCounter:
		return OnUpdateDeathCounter(msg, socket);
	case HostPacketType::OnUpdateWinCounter:
		return OnUpdateWinCounter(msg, socket);
	case HostPacketType::OnUpdateScore:
		return OnUpdateScore(msg, socket);
	case HostPacketType::OnGameEvent:
		return OnGameEvent(msg, socket);
	case HostPacketType::OnUserWeapon:
		return OnUserWeapon(msg, socket);
	case HostPacketType::OnUserSpawn:
		return OnUserSpawn(msg, socket);
	case HostPacketType::OnUpdateClass:
		return OnUpdateClass(msg, socket);
	case 15:
	{
		int unk1 = msg->ReadUInt32();
		int unk2 = msg->ReadUInt8();

		g_pConsole->Warn("Packet_Host 15: %d, %d\n", unk1, unk2);

		break;
	}
	case 17:
	{
		int userID = msg->ReadUInt32();
		int arraySize = msg->ReadUInt8();
		g_pConsole->Warn("Packet_Host 17: %d, %d\n", userID, arraySize);
		for (int i = 0; i < arraySize; i++)
		{
			int unk1 = msg->ReadUInt32();
			int unk2 = msg->ReadUInt32();
			g_pConsole->Warn("Packet_Host 17 array: %d, %d\n", unk1, unk2);
		}

		break;
	}
	case 18:
	{
		/*int unk1 = msg->ReadUInt8();
		int arraySize = msg->ReadUInt8();
		// array????
		g_pConsole->Warn("Packet_Host 18: %d, %d\n", unk1, arraySize);
		for (int i = 0; i < arraySize; i++)
		{
			int unk1 = msg->ReadUInt32();
			int unk2 = msg->ReadUInt16();
			g_pConsole->Warn("Packet_Host 18 array: %d, %d\n", unk1, unk2);
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
		g_pConsole->Warn("Packet_Host type %d is not implemented, len: %d\n", type, msg->GetLength());
		break;
	}

	return false;
}


bool CHostManager::OnSaveData(CReceivePacket* msg, CGameMatch* gamematch)
{
	if (!msg->CanReadBytes(2))
	{
		// наебал систему :)
		return false;
	}

	int saveDataSize = msg->ReadUInt16();
	if (saveDataSize <= 0)
	{
		return false;
	}

	gamematch->SetSaveData(msg->ReadArray(saveDataSize));

	return true;
}

bool CHostManager::OnSetUserInventory(CReceivePacket* msg, CExtendedSocket* socket)
{
	int userID = msg->ReadUInt32();
	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	vector<CUserInventoryItem> inGameItems = g_pUserManager->GetDefaultInventoryItems();
	g_pUserDatabase->GetInventoryItems(userID, inGameItems);

	// remove non ingame, inactive items
	inGameItems.erase(
		remove_if(
			inGameItems.begin(),
			inGameItems.end(),
			[](CUserInventoryItem& item) -> bool {
				return !(item.m_nItemID && item.m_nStatus && item.m_nInUse && g_pItemTable->GetRowValueByItemID<int>((const char*)OBFUSCATE("InGameItem"), to_string(item.m_nItemID)));
			}
		),
		inGameItems.end()
	);

	if (room->GetGameMatch())
	{
		CGameMatchUserStat* stat = room->GetGameMatch()->GetGameUserStat(destUser);
		if (stat)
		{
			stat->m_Items = inGameItems;
		}
	}

	g_pPacketManager->SendHostUserInventory(socket, userID, inGameItems);

	return true;
}

bool CHostManager::OnUseInGameItem(CReceivePacket* msg, CExtendedSocket* socket)
{
	int userID = msg->ReadUInt32();
	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int itemID = msg->ReadUInt16();
	int count = msg->ReadUInt16();

	vector<CUserInventoryItem> items;
	g_pUserDatabase->GetInventoryItemsByID(destUser->GetID(), itemID, items);

	if (items.size() == 0)
		return false;

	g_pItemManager->UseItem(destUser, items[0].GetGameSlot());

	g_pPacketManager->SendHostOnItemUse(socket, userID, items[0].m_nItemID);

	return true;
}

bool CHostManager::OnFlyerFlockRequest(CReceivePacket* msg, CExtendedSocket* socket)
{
	// TODO: rewrite
	string type = msg->ReadString();

	g_pPacketManager->SendHostFlyerFlock(socket, g_pServerConfig->flockingFlyerType);

	return true;
}

bool CHostManager::OnUpdateUserStatus(CReceivePacket* msg, CExtendedSocket* socket)
{
	int userID = msg->ReadUInt32();
	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int status = msg->ReadUInt8();
	if (status == 1)
	{
		destRoom->GetGameMatch()->Connect(destUser);
	}
	else
	{
		g_pConsole->Log("CHostManager::OnUpdateUserStatus: got %d\n", status);
	}

	return true;
}

bool CHostManager::OnKillEvent(CReceivePacket* msg, CExtendedSocket* socket)
{
	int killerUserID = msg->ReadInt32();
	CUser* destUser = g_pUserManager->GetUserById(killerUserID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

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

	destRoom->GetGameMatch()->OnKillEvent(destUser, killEvent);

	return true;
}

bool CHostManager::OnUpdateKillCounter(CReceivePacket* msg, CExtendedSocket* socket)
{
	int userID = msg->ReadInt32();
	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int unk2 = msg->ReadInt8();
	int counter = msg->ReadInt32();
	int unk4 = msg->ReadInt8();

	destRoom->GetGameMatch()->OnUpdateKillCounter(destUser, counter);

	return true;
}

bool CHostManager::OnUpdateDeathCounter(CReceivePacket* msg, CExtendedSocket* socket)
{
	int userID = msg->ReadInt32();
	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int unk2 = msg->ReadInt8();
	int counter = msg->ReadInt32();
	int unk4 = msg->ReadInt8();

	destRoom->GetGameMatch()->OnUpdateDeathCounter(destUser, counter);

	return true;
}

bool CHostManager::OnUpdateWinCounter(CReceivePacket* msg, CExtendedSocket* socket)
{
	int ter = msg->ReadInt8();
	int ct = msg->ReadInt8();
	int unk1 = msg->ReadInt8();
	int unk2 = msg->ReadInt8();
	int unk3 = msg->ReadInt8();
	int unk4 = msg->ReadInt8();
	int unk5 = msg->ReadInt8();

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CGameMatch* gamematch = server != NULL ? server->GetRoom()->GetGameMatch() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom()->GetGameMatch();

	gamematch->OnUpdateWinCounter(ter, ct);

	return true;
}

bool CHostManager::OnUpdateScore(CReceivePacket* msg, CExtendedSocket* socket)
{
	int userID = msg->ReadInt32();
	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int score = msg->ReadInt8();
	int unk3 = msg->ReadInt8();

	destRoom->GetGameMatch()->OnUpdateScore(destUser, score);

	return true;
}

bool CHostManager::OnGameEvent(CReceivePacket* msg, CExtendedSocket* socket)
{
	int type = msg->ReadUInt8();
	int userID = msg->ReadUInt32();

	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	if (type == 0) // bomb explode
	{
		destUser->GetCurrentRoom()->GetGameMatch()->OnBombExplode(destUser);
	}
	else if (type == 1) // bomb defuse
	{
		destUser->GetCurrentRoom()->GetGameMatch()->OnBombDefuse(destUser);
	}
	else if (type == 2) // hostage escape
	{
		destUser->GetCurrentRoom()->GetGameMatch()->OnHostageEscape(destUser);
	}
	else if (type == 32) // monster kill
	{
		int monsterType = msg->ReadUInt32();

		// 1 - def hms 
		// 100 - def zbs zombie
		//g_pConsole->Log(LOG_INTERNAL, CON_WARNING, OBFUSCATE("[User '%s'] Game event - monster kill: userID: %d, monsterType: %d\n"), user->GetLogName(), userID, monsterType);

		destUser->GetCurrentRoom()->GetGameMatch()->OnMonsterKill(destUser, monsterType);
	}
	else if (type == 34) // dropbox pickup
	{
		int rewardID = msg->ReadUInt32();

		//g_pConsole->Log(LOG_INTERNAL, CON_WARNING, OBFUSCATE("[User '%s'] Game event - dropbox: userID: %d, rewardID: %d\n"), user->GetLogName(), userID, rewardID);
		destUser->GetCurrentRoom()->GetGameMatch()->OnDropBoxPickup(destUser, rewardID);
	}
	else if (type == 35) // round restart
	{
	}
	else if (type == 47) // mosquito kill event
	{
		destUser->GetCurrentRoom()->GetGameMatch()->OnMosquitoKill(destUser);
	}
	else if (type == 56) // kite kill event
	{
		destUser->GetCurrentRoom()->GetGameMatch()->OnKiteKill(destUser);
	}
	else if (type == 77)
	{
		int unk = msg->ReadUInt8();
	}
	else
	{
		// a lot of spam
		g_pConsole->Log(OBFUSCATE("Packet_Host game event: eventID: %d, userID: %d, len: %d\n"), type, userID, msg->GetLength());
	}


	return true;
}

bool CHostManager::OnUpdateClass(CReceivePacket* msg, CExtendedSocket* socket)
{
	int userID = msg->ReadInt32();
	CUser* destUser = g_pUserManager->GetUserById(userID);

	if (destUser == NULL)
		return false;

	CRoom* destRoom = destUser->GetCurrentRoom();

	if (destRoom == NULL)
		return false;

	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	if (destRoom != room)
		return false;

	int classItemID = msg->ReadInt16();
	int unk3 = msg->ReadInt8();

	destRoom->GetGameMatch()->OnUpdateClass(destUser, classItemID);

	// send zbs addon info
	if (destRoom->GetSettings()->gameMode == 15)
	{
		CRoomUser* roomUser = destUser->GetRoomData();
		if (!roomUser)
			g_pConsole->Error("CHostManager::OnUpdateClass: no roomUser for '%d'!\n", destUser->GetID());
		else
			g_pPacketManager->SendHostZBAddon(socket, destUser->GetID(), roomUser->m_Addons);
	}

	return true;
}

bool CHostManager::OnZbsResult(CReceivePacket* msg, CExtendedSocket* socket)
{
	g_pConsole->Warn("CHostManager::OnZbsResult\n");

	return true;
}

bool CHostManager::OnGameEnd(CExtendedSocket* socket)
{
	CDedicatedServer* server = g_pDedicatedServerManager->GetServerBySocket(socket);
	CRoom* room = server != NULL ? server->GetRoom() : g_pUserManager->GetUserBySocket(socket)->GetCurrentRoom();

	g_pConsole->Log("Room (RID: %d) ending game\n", room->GetID());

	room->EndGame();

	return true;
}

void CHostManager::OnHostChanged(CUser* gameMatchUser, CUser* newHost, CGameMatch* match)
{
	g_pPacketManager->SendHostRestart(gameMatchUser->GetExtendedSocket(), newHost->GetID(), gameMatchUser == newHost ? true : false, match);

	if (gameMatchUser != newHost)
		//g_pPacketManager->SendHostJoin(gameMatchUser->GetExtendedSocket(), newHost->GetID());
		g_pPacketManager->SendHostServerJoin(gameMatchUser->GetExtendedSocket(), ip_string_to_int(newHost->GetNetworkConfig().m_szExternalIpAddress), false, newHost->GetNetworkConfig().m_nExternalServerPort, gameMatchUser->GetID());
}

bool CHostManager::OnUserWeapon(CReceivePacket* msg, CExtendedSocket* socket)
{
	int id = msg->ReadUInt32();
	int pri = msg->ReadUInt16();
	int sec = msg->ReadUInt16();
	int kni = msg->ReadUInt16();
	int gre = msg->ReadUInt16();

	return true;
}

bool CHostManager::OnUserSpawn(CReceivePacket* msg, CExtendedSocket* socket)
{
	int id = msg->ReadUInt32();
	auto x = msg->ReadFloat();
	auto y = msg->ReadFloat();
	auto z = msg->ReadFloat();

	return true;
}

// everytime round will trigger(new round, round restart)
bool CHostManager::OnRoundStart(CReceivePacket* msg, CExtendedSocket* socket)
{
	// idk why, but something for logging i think

	return true;
}