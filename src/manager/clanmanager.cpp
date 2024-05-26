#include "clanmanager.h"
#include "manager/usermanager.h"
#include "manager/packetmanager.h"
#include "manager/userdatabase.h"

#include "user/userinventoryitem.h"

#include "common/utils.h"

#include "serverconfig.h"

using namespace std;

CClanManager g_ClanManager;

CClanManager::CClanManager() : CBaseManager("ClanManager")
{
}

CClanManager::~CClanManager()
{
}

// fuck, it has tons of flags
// 0 list
// 1 ClanInfo
// 2 create
// 3 join
// 4 cancel join
// 26 clan inventory
// 161 achievement

/*
* Clan Packet Type 26
* 1 byte (some switch)
* if (byte)
*	if (byte == 1)
*	{
*		2 byte (v30)
*		for (int i=0; i<v36; i++) // well fuck v36=v30
*		{
*			4 byte (v27)
*			maybe string?
*			1 byte (v20)
*			2 byte (v18)
*			2 byte (v16)
*			2 byte (v14)
*		}
*	}
*	else if (byte == 2)
*	{
*		sub_373BB4F5(shits)
*			1 byte (v20)
* 	}
*	else if (byte == 3)
*	{
*		for (int k=0; k < 5; k++)
*		{
*			1 byte (v12) // k + v38 + 1080
*		}
*	}
* else (byte = 0)
* {
*	1 byte (v31)
* 
*	sub_373BC62A(shit)
*	2 byte (v20) // size
*	for ( i = 0; i < v23; ++i ) // well v23 = v20, FUCK IDA
*	{
*		2 byte (v16)
*		1 byte (v14)
*		if (v14) // FUCK IDA AGAIN, why GENERATE USELESS SAME DUMMY
*		{
*			2 byte (v8)
*			2 byte (v6)
*			4 byte (v4)
*		}
*		2 byte (v16)
*		2 byte (v16)
*		2 byte (v16)
*		2 byte (v16)
*	}
* }
*/

bool CClanManager::OnPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = g_UserManager.GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case ClanPacketType::RequestClanList:
		OnClanListRequest(msg, user);
		break;
	case ClanPacketType::RequestClanInfo:
		OnClanInfoRequest(msg, user);
		break;
	case ClanPacketType::RequestClanCreate:
		OnClanCreateRequest(msg, user);
		break;
	case ClanPacketType::RequestClanJoin:
		OnClanJoinRequest(msg, user);
		break;
	case ClanPacketType::RequestClanCancelJoin:
		OnClanCancelJoinRequest(msg, user);
		break;
	case ClanPacketType::RequestClanJoinApprove:
		OnClanJoinApproveRequest(msg, user);
		break;
	case ClanPacketType::RequestClanJoinResult:
		OnClanJoinResultRequest(msg, user);
		break;
	case ClanPacketType::RequestClanLeave:
		OnClanLeaveRequest(msg, user);
		break;
	case ClanPacketType::RequestClanInvite:
		OnClanInviteRequest(msg, user);
		break;
	case ClanPacketType::RequestClanChangeMemberGrade:
		OnClanChangeMemberGradeRequest(msg, user);
		break;
	case ClanPacketType::RequestClanKickMember:
		OnClanKickMemberRequest(msg, user);
		break;
	case ClanPacketType::RequestClanUpdateMark:
		OnClanUpdateMarkRequest(msg, user);
		break;
	case ClanPacketType::RequestClanUpdateConfig:
		OnClanUpdateConfigRequest(msg, user);
		break;
	case ClanPacketType::RequestClanSetNotice:
		OnClanSetNoticeRequest(msg, user);
		break;
	case ClanPacketType::RequestClanGiveItem:
		OnClanStorageGiveItemRequest(msg, user);
		break;
	case ClanPacketType::RequestClanGetItem:
		OnClanStorageGetItemRequest(msg, user);
		break;
	case ClanPacketType::RequestClanStorage:
		OnClanStorageRequest(msg, user);
		break;
	case ClanPacketType::RequestClanDissolve:
		OnClanDissolveRequest(msg, user);
		break;
	case ClanPacketType::RequestClanDelegateMaster:
		OnClanDelegateMasterRequest(msg, user);
		break;
	case ClanPacketType::RequestClanMemberUserList:
		OnClanMemberUserListRequest(msg, user);
		break;
	case ClanPacketType::RequestClanJoinUserList:
		OnClanJoinUserListRequest(msg, user);
		break;
	case ClanPacketType::RequestClanDeleteItem:
		OnClanStorageDeleteItem(msg, user);
		break;
	case ClanPacketType::ClanChatMessage:
		OnClanChatMessage(msg, user);
		break;
	case ClanPacketType::RequestClanInactiveAccounts:
		break;
	default:
		Logger().Warn(OBFUSCATE("[User '%s'] Unknown clan type %d\n"), user->GetLogName(), type);
		break;
	}

	return true;
}

bool CClanManager::OnClanListRequest(CReceivePacket* msg, IUser* user)
{
	int pageID = msg->ReadUInt16();
	int flag = msg->ReadUInt8(); // 0 - all play time and gamemodes, 1 - all play time, 2 - all gamemodes, 3 - play time
	int gameModeID = msg->ReadUInt8();
	int unk3 = msg->ReadUInt8(); // always 0
	int playTime = msg->ReadUInt8();
	string clanName = msg->ReadString();

	// TODO: handle GetClanList error?
	vector<ClanList_s> clans;
	int pageMax = 0;
	g_UserDatabase.GetClanList(clans, clanName, flag, gameModeID, playTime, pageID, pageMax);

	g_PacketManager.SendClanList(user->GetExtendedSocket(), clans, pageID, pageMax);

	if (unk3 != 0)
	{
		Logger().Warn(OBFUSCATE("CClanManager::OnClanListRequest: pageID: %d, unk2: %d, gameModeID: %d, unk3: %d, unk4: %d... unk3 is not NULL!!!\n"), pageID, flag, gameModeID, unk3, playTime);
	}

	return true;
}

bool CClanManager::OnClanInfoRequest(CReceivePacket* msg, IUser* user)
{
	int clanID = msg->ReadUInt32();
	Clan_s clan = {};
	if (g_UserDatabase.GetClanInfo(clanID, clan) <= 0)
	{
		// TODO: send failed reply
		return false;
	}

	g_PacketManager.SendClanInfo(user->GetExtendedSocket(), clan);

	return true;
}

bool CClanManager::OnClanCreateRequest(CReceivePacket* msg, IUser* user)
{
	// TODO: Approved but requires some packet sent to client to display lol, kinda weird
	string name = msg->ReadString();
	if (name.size() < 3)
	{
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_CREATE_NAME_TOO_SHORT"));
		return false;
	}
	else if (name.size() > 20)
	{
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_CREATE_NAME_TOO_LONG"));
		return false;
	}
	else if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") != string::npos)
	{
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_CREATE_NAME_BAD_RESERVE"));
		return false;
	}
	else if (findCaseInsensitive(name, g_pServerConfig->nameBlacklist))
	{
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_CREATE_NAME_INVALID_CHAR"));
		return false;
	}

	CUserCharacter character = user->GetCharacter(UFLAG_LOCATION);

	// default config for clan
	ClanCreateConfig clanCfg = {};
	clanCfg.masterUserID = user->GetID();
	clanCfg.name = name;
	clanCfg.points = 0; // score
	clanCfg.time = 0; // null or 0-3am?
	clanCfg.gameModeID = 0;
	clanCfg.mapID = 0;
	clanCfg.region = character.nation; // well depends on translation
	clanCfg.joinMethod = 3; // join method 0-3
	clanCfg.pointBoost = 5;
	clanCfg.expBoost = 5;
	clanCfg.markChangeCount = 1;

	int clanID = g_UserDatabase.CreateClan(clanCfg);
	switch (clanID)
	{
	case -1:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_CREATE_DUP_NAME"));
		return false;
	case -2:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_CREATE_ALREADY_IN_CLAN"));
		return false;
	case -3:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_CREATE_MORE_POINT"));
		return false;
	case 0:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	}

	user->UpdateClan(clanID);

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCreate, 1, NULL);

	OnUserLogin(user);

	return true;
}

bool CClanManager::OnClanJoinRequest(CReceivePacket* msg, IUser* user)
{
	// TODO: reverse packet vars
	int clanID = msg->ReadUInt32();

	string clanName;
	int result = g_UserDatabase.JoinClan(user->GetID(), clanID, clanName);
	switch (result)
	{
	case 0: // db error
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 4, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	case -1: // user already in clan
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 4, OBFUSCATE("CSO_CLAN_JOIN_EXIST_MEMBER"));
		return false;
	case -2: // already sent join request
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 4, OBFUSCATE("CSO_CLAN_JOIN_SAME_CLAN"));
		return false;
	case -3: // not recruiting
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 4, OBFUSCATE("CSO_CLAN_JOIN_NOT_ALLOWED"));
		return false;
	case -4: // invite required
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 4, OBFUSCATE("CSO_CLAN_JOIN_NEED_INVITE"));
		return false;
	case -5: // approve required
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 4, OBFUSCATE("CSO_CLAN_JOIN_NO_AUTH"));
		return false;
	case -6:
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 4, OBFUSCATE("CSO_CLAN_JOIN_MAX_MEMBER"));
		return false;
	case -7:
		g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 2, clanName.c_str());
		return false;
	}

	g_PacketManager.SendClanJoinReply(user->GetExtendedSocket(), 0, NULL);

	user->UpdateClan(clanID);

	OnUserLogin(user);

	return true;
}

bool CClanManager::OnClanCancelJoinRequest(CReceivePacket* msg, IUser* user)
{
	int clanID = msg->ReadUInt32();

	int result = g_UserDatabase.CancelJoin(user->GetID(), clanID);
	switch (result)
	{
	case 0: // db error
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCancelJoin, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	case -1: // user already in clan
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCancelJoin, 0, OBFUSCATE("CSO_CLAN_JOIN_CANCEL_NOT_APPLIED"));
		return false;
	}

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanCancelJoin, 1, NULL);

	return true;
}

bool CClanManager::OnClanJoinApproveRequest(CReceivePacket* msg, IUser* user)
{
	string userName = msg->ReadString();

	int result = g_UserDatabase.ClanApprove(user->GetID(), userName);
	switch (result)
	{
	case 0: // db error
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinApprove, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	case -1:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinApprove, 0, OBFUSCATE("CSO_CLAN_NOT_IN_APPLICANTS"));
		return false;
	}

	IUser* targetUser = g_UserManager.GetUserByUsername(userName);
	if (targetUser)
	{
		OnUserLogin(targetUser);
	}
	else
	{
		ClanUser targetMember{};
		vector<ClanUser> users;
		g_UserDatabase.GetClanUserList(user->GetID(), true, users);
		for (auto member : users)
		{
			if (member.userName == userName)
			{
				targetMember = member;
			}
		}

		for (auto member : users)
		{
			if (!member.user)
				continue;

			g_PacketManager.SendClanUpdateUserList(member.user->GetExtendedSocket(), targetMember);
		}
	}

	ClanUserJoinRequest targetMemberJr{};
	targetMemberJr.userName = userName;
	g_PacketManager.SendClanUpdateJoinUserList(user->GetExtendedSocket(), targetMemberJr, true);

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinApprove, 1, NULL);

	return true;
}

bool CClanManager::OnClanJoinResultRequest(CReceivePacket* msg, IUser* user)
{
	int type = msg->ReadUInt8();
	if (type == 0)
	{
		string userName = msg->ReadString();

		int result = g_UserDatabase.ClanReject(user->GetID(), userName);
		switch (result)
		{
		case 0: // db error
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinResult, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		case -1:
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinResult, 0, OBFUSCATE("CSO_CLAN_NOT_IN_APPLICANTS"));
			return false;
		}

		ClanUserJoinRequest clanUser;
		clanUser.userName = userName;
		g_PacketManager.SendClanUpdateJoinUserList(user->GetExtendedSocket(), clanUser, true);
	}
	else if (type == 1)
	{
		int result = g_UserDatabase.ClanRejectAll(user->GetID());
		switch (result)
		{
		case 0: // db error
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinResult, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		case -1:
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinResult, 0, OBFUSCATE("CSO_CLAN_NOT_IN_APPLICANTS"));
			return false;
		}

		g_PacketManager.SendClanDeleteJoinUserList(user->GetExtendedSocket());
	}

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanJoinResult, 1, NULL);

	return true;
}

bool CClanManager::OnClanLeaveRequest(CReceivePacket* msg, IUser* user)
{
	int result = g_UserDatabase.LeaveClan(user->GetID());
	switch (result)
	{
	case -1:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanLeave, 0, OBFUSCATE("CSO_CLAN_NO_AUTH"));
		return false;
	case 0:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanLeave, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	}

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanLeave, 1, NULL);

	// send update to clan members
	ClanUser clanUser = {};
	clanUser.userName = user->GetUsername(); // TODO: rewrite

	std::vector<ClanUser> users;
	g_UserDatabase.GetClanUserList(user->GetID(), true, users);
	for (auto member : users)
	{
		if (member.user)
		{
			g_PacketManager.SendClanUpdateUserList(member.user->GetExtendedSocket(), clanUser, true);
			g_PacketManager.SendClanUpdateMemberUserList(member.user->GetExtendedSocket(), clanUser, true);
		}
	}

	user->UpdateClan(0);

	return true;
}

bool CClanManager::OnClanInviteRequest(CReceivePacket* msg, IUser* user)
{
	int type = msg->ReadUInt8();
	if (type == 1)
	{
		Logger().Warn(OBFUSCATE("CClanManager::OnClanInviteRequest: type 1 received\n"));
		return true;
	}

	IUser* destUser = NULL;
	int clanID = 0;
	string gameName = msg->ReadString();
	int result = g_UserDatabase.ClanInvite(user->GetID(), gameName, destUser, clanID);
	switch (result)
	{
	case 0:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanInvite, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	case -1:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanInvite, 0, OBFUSCATE("CSO_CLAN_INVITE_NO_AUTH"));
		return false;
	case -2:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanInvite, 0, OBFUSCATE("CSO_CLAN_INVITE_FAILED"));
		return false;
	case -3:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanInvite, 0, OBFUSCATE("CSO_CLAN_INVITE_FAIL_NOT_EXIST"));
		return false;
	case -4:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanInvite, 0, OBFUSCATE("CSO_CLAN_CANT_INVITE_MASTER"));
		return false;
	case -5:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanInvite, 0, OBFUSCATE("CSO_CLAN_CANT_INVITE_SAME_CLAN"));
		return false;
	}

	Clan_s clan = {};
	g_UserDatabase.GetClan(user->GetID(), CFLAG_NAME, clan);
	g_PacketManager.SendClanInvite(destUser->GetExtendedSocket(), clan.name, clanID);

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanInvite, 1, NULL);

	return true;
}

bool CClanManager::OnClanChangeMemberGradeRequest(CReceivePacket* msg, IUser* user)
{
	string userName = msg->ReadString();
	int newGrade = msg->ReadUInt8();
	if (newGrade > 3)
		return false;

	ClanUser targetMember {};
	int result = g_UserDatabase.UpdateClanMemberGrade(user->GetID(), userName, newGrade, targetMember);
	switch (result)
	{
	case -1: // not clan master
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanChangeMemberGrade, 0, OBFUSCATE("CSO_CLAN_GRADE_NO_AUTH"));
		return false;
	case -2:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanChangeMemberGrade, 0, OBFUSCATE("CSO_CLAN_GRADE_OFFICER_NO_AUTH"));
		return false;
	case 0:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanChangeMemberGrade, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	}

	// TODO: test this
	if (targetMember.user)
	{
		Clan_s clan = {};
		if (g_UserDatabase.GetClan(user->GetID(), CFLAG_ID | CFLAG_NAME | CFLAG_CLANMASTER | CFLAG_JOINMETHOD, clan) > 0)
		{
			g_PacketManager.SendClanUpdate(targetMember.user->GetExtendedSocket(), 1, newGrade, clan);
		}
	}

	g_PacketManager.SendClanUpdateMemberUserList(user->GetExtendedSocket(), targetMember);

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanChangeMemberGrade, 1, NULL);

	return true;
}

bool CClanManager::OnClanKickMemberRequest(CReceivePacket* msg, IUser* user)
{
	string userName = msg->ReadString();

	int result = g_UserDatabase.ClanKick(user->GetID(), userName);
	switch (result)
	{
	case 0:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanKickMember, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	case -1:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanKickMember, 0, OBFUSCATE("CSO_CLAN_NO_AUTH"));
		return false;
	}

	ClanUser kickedMember {};
	kickedMember.userName = userName;

	IUser* targetUser = g_UserManager.GetUserByUsername(userName);
	if (targetUser)
	{
		g_PacketManager.SendClanKick(targetUser->GetExtendedSocket());

		targetUser->UpdateClan(0);
	}

	vector<ClanUser> users;
	g_UserDatabase.GetClanMemberList(user->GetID(), users);
	for (auto member : users)
	{
		if (!member.user)
			continue;

		g_PacketManager.SendClanUpdateUserList(member.user->GetExtendedSocket(), kickedMember, true);
	}

	g_PacketManager.SendClanUpdateMemberUserList(user->GetExtendedSocket(), kickedMember, true);

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanKickMember, 1, NULL);

	return true;
}

bool CClanManager::OnClanUpdateMarkRequest(CReceivePacket* msg, IUser* user)
{
	int type = msg->ReadUInt8();
	if (type != 0)
	{
		Logger().Warn(OBFUSCATE("CClanManager::OnClanUpdateMarkRequest: unk type: %d\n"), type);
		return false;
	}
	int subType = msg->ReadUInt8();
	if (subType == 0)
	{
		int unk = msg->ReadUInt16();
		int unk2 = msg->ReadUInt8();

		// TODO: display used marks
		g_PacketManager.SendClanMarkColor(user->GetExtendedSocket());
	}
	else if (subType == 1)
	{
		Clan_s clan = {};
		g_UserDatabase.GetClan(user->GetID(), CFLAG_ID | CFLAG_NAME | CFLAG_MARKCHANGECOUNT, clan);

		if (!clan.markChangeCount)
		{
			g_PacketManager.SendClanMarkReply(user->GetExtendedSocket(), 0, OBFUSCATE("CSO_CLAN_NO_MARK_ITEM"));
			return false;
		}

		clan.markID = msg->ReadUInt32();
		clan.markChangeCount--;

		int result = g_UserDatabase.IsClanWithMarkExists(clan.markID);
		switch (result)
		{
		case 0:
			g_PacketManager.SendClanMarkReply(user->GetExtendedSocket(), 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		case 1:
			g_PacketManager.SendClanMarkReply(user->GetExtendedSocket(), 0, OBFUSCATE("CSO_CLAN_MARK_DUP"));
			return false;
		}

		// CSO_CLAN_MARK_INVALID_SHAPE, CSO_CLAN_MARK_INVALID_COLOR
		result = g_UserDatabase.UpdateClan(user->GetID(), CFLAG_MARKID | CFLAG_MARKCHANGECOUNT, clan);
		switch (result)
		{
		case -1: // not clan master
			g_PacketManager.SendClanMarkReply(user->GetExtendedSocket(), 0, OBFUSCATE("CSO_CLAN_GRADE_NO_AUTH"));
			return false;
		case 0:
			g_PacketManager.SendClanMarkReply(user->GetExtendedSocket(), 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		}

		g_PacketManager.SendClanMarkReply(user->GetExtendedSocket(), 1, NULL);
		
		CUserCharacter character = {};
		character.clanID = clan.id;
		character.clanName = clan.name;
		character.clanMarkID = clan.markID;
		user->UpdateClientUserInfo(UFLAG_CLAN, character);

		g_PacketManager.SendClanUpdate(user->GetExtendedSocket(), 7, 0, clan);
	}
	else
	{
		Logger().Warn(OBFUSCATE("CClanManager::OnClanUpdateMarkRequest: unk subType: %d\n"), type);
	}

	return true;
}

bool CClanManager::OnClanUpdateConfigRequest(CReceivePacket* msg, IUser* user)
{
	int type = msg->ReadUInt8();
	if (type == 0)
	{
		int joinMethod = msg->ReadUInt8();
		if (joinMethod > 3)
		{
			return false;
		}

		Clan_s clan = {};
		clan.joinMethod = joinMethod;
		int result = g_UserDatabase.UpdateClan(user->GetID(), CFLAG_JOINMETHOD, clan);
		switch (result)
		{
		case -1: // not clan master
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanUpdateConfig, 0, OBFUSCATE("CSO_CLAN_SETTING_NO_AUTH"));
			return false;
		case 0:
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanUpdateConfig, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		}

		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanUpdateConfig, 1, NULL);

		// send this to all members..?
		if (g_UserDatabase.GetClan(user->GetID(), CFLAG_ID | CFLAG_NAME | CFLAG_CLANMASTER, clan) > 0)
		{
			ClanUser clanUser = {};
			if (g_UserDatabase.GetClanMember(user->GetID(), clanUser) > 0)
			{
				g_PacketManager.SendClanUpdate(user->GetExtendedSocket(), 1, clanUser.memberGrade, clan);
			}
		}
	}
	else if (type == 3) // update region(2 bytes)
	{
		int region = msg->ReadUInt16();

		Clan_s clan = {};
		clan.region = region;
		int result = g_UserDatabase.UpdateClan(user->GetID(), CFLAG_REGION, clan);
		switch (result)
		{
		case -1: // not clan master
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanUpdateConfig, 0, OBFUSCATE("CSO_CLAN_SETTING_NO_AUTH"));
			return false;
		case 0:
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanUpdateConfig, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		}

		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanUpdateConfig, 1, NULL);
	}
	else
	{
		Logger().Warn(OBFUSCATE("CClanManager::OnClanUpdateConfigRequest: unknown type: %d\n"), type);
	}

	return true;
}

bool CClanManager::OnClanSetNoticeRequest(CReceivePacket* msg, IUser* user)
{
	int unk = msg->ReadUInt8();
	if (unk == 1)
	{
		string notice = msg->ReadString();

		Clan_s clan = {};
		clan.noticeMsg = notice;
		int result = g_UserDatabase.UpdateClan(user->GetID(), CFLAG_NOTICEMSG, clan);
		switch (result)
		{
		case -1: // not clan master
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanSetNotice, 0, OBFUSCATE("CSO_CLAN_NOTICE_NO_AUTH"));
			return false;
		case 0:
			g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanSetNotice, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		}

		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanSetNotice, 1, NULL);

		// TODO: test this
		vector<ClanUser> users;
		g_UserDatabase.GetClanUserList(user->GetID(), true, users);
		for (auto member : users)
		{
			if (member.user)
			{
				g_PacketManager.SendClanUpdateNotice(member.user->GetExtendedSocket(), clan);
			}
		}
	}

	return true;
}

bool CClanManager::OnClanStorageGiveItemRequest(CReceivePacket* msg, IUser* user)
{
	int storagePageID = msg->ReadUInt8();
	int unk2 = msg->ReadUInt8();
	int slot = msg->ReadUInt16();
	
	CUserInventoryItem item = {};
	item.m_nSlot = slot;

	int result = g_UserDatabase.AddClanStorageItem(user->GetID(), storagePageID, item);
	switch (result)
	{
	case -3: // storage page is full
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGiveItem, 0, OBFUSCATE("CSO_CLAN_GIVE_ITEM_EXCEED_LIMIT"));
		return false;
	case -1: // user not in clan or access grade is lower than user's member grade
	case -2: // user inventory item does not exist
	case 0: // db error
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGiveItem, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	}

	g_PacketManager.SendInventoryRemove(user->GetExtendedSocket(), vector<CUserInventoryItem> {item}, false);

	//g_PacketManager.SendClanStoragePage(user->GetExtendedSocket());

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGiveItem, 1, NULL);

	return true;
}

bool CClanManager::OnClanStorageGetItemRequest(CReceivePacket* msg, IUser* user)
{
	int pageID = msg->ReadUInt8();
	int slot = msg->ReadUInt16();

	CUserInventoryItem item = {};
	int result = g_UserDatabase.GetClanStorageItem(user->GetID(), pageID, slot, item);
	switch (result)
	{
	case -1: // user not in clan or access grade is lower than user's member grade
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGetItem, 0, OBFUSCATE("CSO_CLAN_SYSTEM_ERROR"));
		return false;
	case -2: // user inventory is full
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGetItem, 0, OBFUSCATE("CSO_CLAN_INVENTORY_FULL"));
		return false;
	case -3: // user already has the same permanent item
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGetItem, 0, OBFUSCATE("CSO_CLAN_GET_ITEM_HAS_INFINITY_ITEM"));
		return false;
	case -4: // user has exceed storage item limit
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGetItem, 0, OBFUSCATE("CSO_CLAN_GET_ITEM_EXCEED_LIMIT"));
		return false;
	case -5: // user already has the same clan item
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGetItem, 0, OBFUSCATE("CSO_CLAN_GET_ITEM_HAS_CLAN_ITEM"));
		return false;
	case 0: // db error
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGetItem, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	}

	g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), vector<CUserInventoryItem> {item});

	// TODO: send update?

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanGetItem, 1, NULL);

	return true;
}

bool CClanManager::OnClanStorageRequest(CReceivePacket* msg, IUser* user)
{
	int type = msg->ReadUInt8();
	if (type == 0) // request storage page
	{
		int storagePageID = msg->ReadUInt8();
		ClanStoragePage storagePage = {};
		storagePage.pageID = storagePageID;
		if (g_UserDatabase.GetClanStoragePage(user->GetID(), storagePage) <= 0)
		{
			g_PacketManager.SendClanStorageReply(user->GetExtendedSocket(), 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		}

		g_PacketManager.SendClanStoragePage(user->GetExtendedSocket(), storagePage);
	}
	else if (type == 1) // request storage usage history
	{
		ClanStorageHistory storageHistory = {};
		if (g_UserDatabase.GetClanStorageHistory(user->GetID(), storageHistory) <= 0)
		{
			g_PacketManager.SendClanStorageReply(user->GetExtendedSocket(), 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
			return false;
		}

		g_PacketManager.SendClanStorageHistory(user->GetExtendedSocket()); // client crashes
	}
	else if (type == 2) // request storage set access grade 
	{
		int storagePageID = msg->ReadUInt8();
		int accessGrade = msg->ReadUInt8();

		g_PacketManager.SendClanStorageReply(user->GetExtendedSocket(), 1, NULL);

		g_UserDatabase.UpdateClanStorageAccessGrade(user->GetID(), storagePageID, accessGrade);

		vector<int> storageAccessGrade;
		if (g_UserDatabase.GetClanStorageAccessGrade(user->GetID(), storageAccessGrade) > 0)
		{
			vector<ClanUser> userList;
			if (g_UserDatabase.GetClanUserList(user->GetID(), true, userList) > 0)
			{
				for (auto clanUser : userList)
				{
					if (clanUser.user)
						g_PacketManager.SendClanStorageAccessGrade(clanUser.user->GetExtendedSocket(), storageAccessGrade);
				}
			}
		}
	}
	else
	{
		Logger().Warn(OBFUSCATE("CClanManager::OnClanStorageRequest: unknown type: %d\n"), type);
	}

	return true;
}

bool CClanManager::OnClanStorageDeleteItem(CReceivePacket* msg, IUser* user)
{
	int storagePageID = msg->ReadUInt8();
	int slot = msg->ReadUInt16();

	if (g_UserDatabase.DeleteClanStorageItem(user->GetID(), storagePageID, slot) <= 0)
	{
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanDeleteItem, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	}

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanDeleteItem, 1, NULL);

	return true;
}

bool CClanManager::OnClanDissolveRequest(CReceivePacket* msg, IUser* user)
{
	int result = g_UserDatabase.DissolveClan(user->GetID());
	switch (result)
	{
	case -1:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanDissolve, 0, OBFUSCATE("CSO_CLAN_NO_AUTH"));
		return false;
	case 0:
		g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanDissolve, 0, OBFUSCATE("CSO_CLAN_DB_SYSTEM_ERROR"));
		return false;
	}

	g_PacketManager.SendClanReply(user->GetExtendedSocket(), RequestClanDissolve, 1, NULL);

	user->UpdateClan(0);

	// TODO: send CSO_CLAN_DISBANDED to all clan member?

	return true;
}

bool CClanManager::OnClanDelegateMasterRequest(CReceivePacket* msg, IUser* user)
{
	string userName = msg->ReadString();

	int result = g_UserDatabase.ClanMasterDelegate(user->GetID(), userName);
	switch (result)
	{
	case -1:
		g_PacketManager.SendClanMasterDelegate(user->GetExtendedSocket());
		return false;
	case 0:
		g_PacketManager.SendClanMasterDelegate(user->GetExtendedSocket());
		return false;
	}

	g_PacketManager.SendClanMasterDelegate(user->GetExtendedSocket());

	return true;
}

bool CClanManager::OnClanMemberUserListRequest(CReceivePacket* msg, IUser* user)
{
	vector<ClanUser> users;
	int result = g_UserDatabase.GetClanMemberList(user->GetID(), users);
	switch (result)
	{
	case 0:
		return false;
	}

	if (users.size())
		g_PacketManager.SendClanCreateMemberUserList(user->GetExtendedSocket(), users);

	return true;
}

bool CClanManager::OnClanJoinUserListRequest(CReceivePacket* msg, IUser* user)
{
	vector<ClanUserJoinRequest> users;
	int result = g_UserDatabase.GetClanMemberJoinUserList(user->GetID(), users);
	switch (result)
	{
	case 0:
		return false;
	}

	if (users.size())
		g_PacketManager.SendClanCreateJoinUserList(user->GetExtendedSocket(), users);

	return true;
}

bool CClanManager::OnClanChatMessage(CReceivePacket* msg, IUser* user)
{
	string message = msg->ReadString();

	vector<ClanUser> userList;
	if (g_UserDatabase.GetClanUserList(user->GetID(), true, userList) <= 0 || userList.size() <= 0)
	{
		// failed
		return false;
	}

	// send message to all clan users
	for (auto clanUser : userList)
	{
		if (clanUser.user)
		{
			CUserCharacterExtended characterExtendedDest = clanUser.user->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);

			// you can't send clan message if dest is blocking your chat
			if (~characterExtendedDest.banSettings & 2 || characterExtendedDest.banSettings & 2 && !g_UserDatabase.IsInBanList(clanUser.userID, user->GetID()))
			{
				CUserCharacter character = user->GetCharacter(UFLAG_GAMENAME);

				g_PacketManager.SendClanChatMessage(clanUser.user->GetExtendedSocket(), character.gameName, message);
			}
		}
	}

	return true;
}

void CClanManager::OnUserLogin(IUser* user)
{
	vector<ClanUser> userList;
	if (g_UserDatabase.GetClanUserList(user->GetID(), true, userList) <= 0 || userList.size() <= 0)
	{
		// failed
		return;
	}

	g_PacketManager.SendClanCreateUserList(user->GetExtendedSocket(), userList);

	// TODO: move it to new func?
	ClanUser loggedInUser = {};
	for (auto clanUser : userList)
	{
		if (user->GetID() == clanUser.userID)
		{
			loggedInUser = clanUser;
			break;
		}
	}

	// send user list update to all clan members
	for (auto clanUser : userList)
	{
		if (clanUser.user && clanUser.user->GetID() != user->GetID())
		{
			g_PacketManager.SendClanUpdateUserList(clanUser.user->GetExtendedSocket(), loggedInUser);
		}
	}

	vector<ClanUser> users;
	if (g_UserDatabase.GetClanMemberList(user->GetID(), users) > 0 && users.size())
	{
		g_PacketManager.SendClanCreateMemberUserList(user->GetExtendedSocket(), users);
	}

	vector<ClanUserJoinRequest> usersJoin;
	if (g_UserDatabase.GetClanMemberJoinUserList(user->GetID(), usersJoin) > 0 && usersJoin.size())
	{
		g_PacketManager.SendClanCreateJoinUserList(user->GetExtendedSocket(), usersJoin);
	}

	vector<int> storageAccessGrade;
	if (g_UserDatabase.GetClanStorageAccessGrade(user->GetID(), storageAccessGrade) > 0)
	{
		g_PacketManager.SendClanStorageAccessGrade(user->GetExtendedSocket(), storageAccessGrade);
	}

	Clan_s clan = {};
	g_UserDatabase.GetClan(user->GetID(), CFLAG_ID | CFLAG_NAME | CFLAG_TIME | CFLAG_GAMEMODEID | CFLAG_MAPID | CFLAG_REGION | CFLAG_JOINMETHOD | CFLAG_EXPBOOST | CFLAG_POINTBOOST | CFLAG_NOTICEMSG
		| CFLAG_SCORE | CFLAG_MARKID | CFLAG_MARKCOLOR | CFLAG_CLANMASTER | CFLAG_MARKCHANGECOUNT | CFLAG_MAXMEMBERCOUNT | CFLAG_CHRONICLE, clan);

	//ClanStoragePage storagePage;
	//g_UserDatabase.GetClanStoragePage(user->GetID(), );
	g_PacketManager.SendClanUpdate(user->GetExtendedSocket(), 0, loggedInUser.memberGrade, clan);
}