#pragma once

#include "interface/iusermanager.h"

#include "user/user.h"
#include "manager/manager.h"

class CUserManager : public CBaseManager<IUserManager>
{
public:
	CUserManager();
	~CUserManager();

	virtual bool Init();
	virtual void Shutdown();
	virtual void OnSecondTick(time_t curTime);

	bool OnLoginPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnUdpPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnOptionPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnVersionPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnFavoritePacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnCharacterPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnUserMessage(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnUpdateInfoPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnReportPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnAlarmPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnUserSurveyPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnBanPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnMessengerPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnAddonPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnLeaguePacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnCryptPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnKickPacket(CReceivePacket* msg, IExtendedSocket* socket);

	void SendNoticeMessageToAll(const std::string& msg);
	void SendNoticeMsgBoxToAll(const std::string& msg);

	int LoginUser(IExtendedSocket* socket, const std::string& userName, const std::string& password);
	int RegisterUser(IExtendedSocket* socket, const std::string& userName, const std::string& password);
	void DisconnectUser(IUser* user);
	void DisconnectAllFromServer();
	IUser* AddUser(IExtendedSocket* socket, int userID, const std::string& userName);
	IUser* GetUserById(int userId);
	IUser* GetUserBySocket(IExtendedSocket* socket);
	IUser* GetUserByUsername(const std::string& username);
	IUser* GetUserByNickname(const std::string& nickname);
	void RemoveUser(IUser* user);
	void RemoveUserById(int userId);
	void RemoveUserBySocket(IExtendedSocket* socket);
	void CleanUpUser(IUser* user);

	std::vector<IUser*> GetUsers();

	int ChangeUserNickname(IUser* user, const std::string& newNickname, bool createCharacter = false);

	std::vector<CUserInventoryItem>& GetDefaultInventoryItems();

	void SendMetadata(IExtendedSocket* socket);

private:
	void SendGuestUserPacket(IExtendedSocket* socket);
	void SendLoginPacket(IUser* user, const CUserCharacter& character);
	void SendUserInventory(IUser* user);
	void SendUserLoadout(IUser* user);
	void SendUserNotices(IUser* user);
	bool OnFavoriteSetLoadout(CReceivePacket* msg, IUser* user);
	bool OnFavoriteSetBuyMenu(CReceivePacket* msg, IUser* user);
	bool OnFavoriteSetFastBuy(CReceivePacket* msg, IUser* user);
	bool OnFavoriteSetBookmark(CReceivePacket* msg, IUser* user);

	void OnUserSurveyAnswerRequest(CReceivePacket* msg, IUser* user);

	void OnBanAddNicknameRequest(CReceivePacket* msg, IUser* user);
	void OnBanRemoveNicknameRequest(CReceivePacket* msg, IUser* user);
	void OnBanSettingsRequest(CReceivePacket* msg, IUser* user);

	std::vector<IUser*> m_Users;
	std::vector<CUserInventoryItem> m_DefaultItems;
};

extern CUserManager g_UserManager;