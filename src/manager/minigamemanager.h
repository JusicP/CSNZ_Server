#pragma once

#include "interface/iminigamemanager.h"

#include "usermanager.h"

class CMiniGameManager : public CBaseManager<IMiniGameManager>
{
public:
	CMiniGameManager();
	~CMiniGameManager();

	void OnPacket(CReceivePacket* msg, IExtendedSocket* socket);

	void WeaponReleaseAddCharacter(IUser* user, char charID, int count);

	void SendWeaponReleaseUpdate(IUser* user);

	void OnBingoUpdateRequest(IUser* user);

private:
	// bingo
	void OnBingoRequest(CReceivePacket* msg, IUser* user);
	void OnBingoResetRequest(IUser* user);
	void OnBingoShuffleRequest(IUser* user);

	bool BingoInitDesk(IUser* user, UserBingo& bingo);
	bool BingoOpenRandomNumber(IUser* user, UserBingo& bingo);

	// weapon release
	void OnWeaponReleaseRequest(CReceivePacket* msg, IUser* user);
	void OnWeaponReleaseSetCharacterRequest(CReceivePacket* msg, IUser* user);
	void OnWeaponReleaseGetJokerRequest(IUser* user);
};

extern CMiniGameManager g_MiniGameManager;