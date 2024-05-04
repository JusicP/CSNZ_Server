#pragma once

#include "interface/ihostmanager.h"
#include "manager.h"

#include "user/user.h"

class CHostManager : public CBaseManager<IHostManager>
{
public:
	CHostManager();
	~CHostManager();

	bool OnPacket(CReceivePacket* msg, IExtendedSocket* socket);

	void OnHostChanged(IUser* gameMatchUser, IUser* newHost, CGameMatch* match);
private:
	bool OnSaveData(CReceivePacket* msg, CGameMatch* gameMatch);
	bool OnSetUserInventory(CReceivePacket* msg, IExtendedSocket* socket, IRoom* room);
	bool OnUseInGameItem(CReceivePacket* msg, IExtendedSocket* socket, IRoom* room);
	bool OnFlyerFlockRequest(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnUpdateUserStatus(CReceivePacket* msg, IExtendedSocket* socket, IRoom* room, CGameMatch* gameMatch);
	bool OnKillEvent(CReceivePacket* msg, IRoom* room);
	bool OnUpdateKillCounter(CReceivePacket* msg, IRoom* room);
	bool OnUpdateDeathCounter(CReceivePacket* msg, IRoom* room);
	bool OnUpdateWinCounter(CReceivePacket* msg, CGameMatch* gameMatch);
	bool OnUpdateScore(CReceivePacket* msg, IRoom* room);
	bool OnGameEvent(CReceivePacket* msg, IRoom* room);
	bool OnUpdateClass(CReceivePacket* msg, IRoom* room);
	bool OnZbsResult(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnGameEnd(IExtendedSocket* socket);
	bool OnUserWeapon(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnUserSpawn(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnRoundStart(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnChangeMap(CReceivePacket* msg, IRoom* room);
};

extern CHostManager g_HostManager;