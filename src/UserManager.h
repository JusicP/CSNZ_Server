#pragma once

#include "User.h"

class CUserManager
{
public:
	CUserManager(int maxPlayers);

	bool OnLoginPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnUdpPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnOptionPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnVersionPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnFavoritePacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnCharacterPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnUserMessage(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnUpdateInfoPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnReportPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnAlarmPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnUserSurveyPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnBanPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnMessengerPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnAddonPacket(CReceivePacket* msg, CExtendedSocket* socket);
	bool OnLeaguePacket(CReceivePacket* msg, CExtendedSocket* socket);

	void OnSecondTick();

	void SendNoticeMessageToAll(std::string msg);
	void SendNoticeMsgBoxToAll(std::string msg);

	int LoginUser(CExtendedSocket* socket, std::string userName, std::string password);
	int RegisterUser(CExtendedSocket* socket, std::string userName, std::string password);
	void DisconnectUser(CUser* user);
	void DisconnectAllFromServer();
	CUser* AddUser(CExtendedSocket* socket, int userID, std::string userName);
	CUser* GetUserById(int userId);
	CUser* GetUserBySocket(CExtendedSocket* socket);
	CUser* GetUserByUsername(std::string username);
	CUser* GetUserByNickname(std::string userName);
	void RemoveUser(CUser* user);
	void RemoveUserById(int userId);
	void RemoveUserBySocket(CExtendedSocket* socket);
	void CleanUpUser(CUser* user);
	std::vector<CUser*> users;

	int ChangeUserNickname(CUser* user, std::string newNickname, bool createCharacter = false);

	std::vector<CUserInventoryItem>& GetDefaultInventoryItems();

	void SendMetadata(CExtendedSocket* socket);

private:
	void SendGuestUserPacket(CExtendedSocket* socket);
	void SendLoginPacket(CUser* user, const CUserCharacter& character);
	void SendUserInventory(CUser* user);
	void SendUserLoadout(CUser* user);
	void SendUserNotices(CUser* user);
	bool OnFavoriteSetLoadout(CReceivePacket* msg, CUser* user);
	bool OnFavoriteSetBuyMenu(CReceivePacket* msg, CUser* user);
	bool OnFavoriteSetFastBuy(CReceivePacket* msg, CUser* user);
	bool OnFavoriteSetBookmark(CReceivePacket* msg, CUser* user);

	void OnUserSurveyAnswerRequest(CReceivePacket* msg, CUser* user);

	void OnBanAddNicknameRequest(CReceivePacket* msg, CUser* user);
	void OnBanRemoveNicknameRequest(CReceivePacket* msg, CUser* user);
	void OnBanSettingsRequest(CReceivePacket* msg, CUser* user);

	std::vector<CUserInventoryItem> m_DefaultItems;
};
