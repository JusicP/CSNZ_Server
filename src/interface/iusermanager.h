#pragma once

#include "imanager.h"
#include "iuser.h"

#include <string>
#include <vector>

class CUserInventoryItem;
class CReceivePacket;
class IExtendedSocket;
class IUser;

class IUserManager : public IBaseManager
{
public:
	virtual bool OnLoginPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnUdpPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnOptionPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnVersionPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnFavoritePacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnCharacterPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnUserMessage(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnUpdateInfoPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnReportPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnAlarmPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnUserSurveyPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnBanPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnMessengerPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnAddonPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnLeaguePacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnCryptPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual bool OnKickPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;

	virtual void SendNoticeMessageToAll(const std::string& msg) = 0;
	virtual void SendNoticeMsgBoxToAll(const std::string& msg) = 0;

	virtual int LoginUser(IExtendedSocket* socket, const std::string& userName, const std::string& password) = 0;
	virtual int RegisterUser(IExtendedSocket* socket, const std::string& userName, const std::string& password) = 0;
	virtual void DisconnectUser(IUser* user) = 0;
	virtual void DisconnectAllFromServer() = 0;
	virtual IUser* AddUser(IExtendedSocket* socket, int userID, const std::string& userName) = 0;
	virtual IUser* GetUserById(int userID) = 0;
	virtual IUser* GetUserBySocket(IExtendedSocket* socket) = 0;
	virtual IUser* GetUserByUsername(const std::string& userName) = 0;
	virtual IUser* GetUserByNickname(const std::string& nickname) = 0;
	virtual void RemoveUser(IUser* user) = 0;
	virtual void RemoveUserById(int userID) = 0;
	virtual void RemoveUserBySocket(IExtendedSocket* socket) = 0;
	virtual void CleanUpUser(IUser* user) = 0;

	virtual std::vector<IUser*> GetUsers() = 0;

	virtual int ChangeUserNickname(IUser* user, const std::string& newNickname, bool createCharacter = false) = 0;

	virtual std::vector<CUserInventoryItem>& GetDefaultInventoryItems() = 0;

	virtual void SendMetadata(IExtendedSocket* socket) = 0;
};
