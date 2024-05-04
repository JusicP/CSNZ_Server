#pragma once

#include "interface/iclanmanager.h"
#include "manager.h"

class CClanManager : public CBaseManager<IClanManager>
{
public:
	CClanManager();
	~CClanManager();

	bool OnPacket(CReceivePacket* msg, IExtendedSocket* socket);
	bool OnClanListRequest(CReceivePacket* msg, IUser* user);
	bool OnClanInfoRequest(CReceivePacket* msg, IUser* user);
	bool OnClanCreateRequest(CReceivePacket* msg, IUser* user);
	bool OnClanJoinRequest(CReceivePacket* msg, IUser* user);
	bool OnClanCancelJoinRequest(CReceivePacket* msg, IUser* user);
	bool OnClanJoinApproveRequest(CReceivePacket* msg, IUser* user);
	bool OnClanJoinResultRequest(CReceivePacket* msg, IUser* user);
	bool OnClanLeaveRequest(CReceivePacket* msg, IUser* user);
	bool OnClanInviteRequest(CReceivePacket* msg, IUser* user);
	bool OnClanChangeMemberGradeRequest(CReceivePacket* msg, IUser* user);
	bool OnClanKickMemberRequest(CReceivePacket* msg, IUser* user);
	bool OnClanUpdateMarkRequest(CReceivePacket* msg, IUser* user);
	bool OnClanUpdateConfigRequest(CReceivePacket* msg, IUser* user);
	bool OnClanSetNoticeRequest(CReceivePacket* msg, IUser* user);
	bool OnClanStorageGiveItemRequest(CReceivePacket* msg, IUser* user);
	bool OnClanStorageGetItemRequest(CReceivePacket* msg, IUser* user);
	bool OnClanStorageRequest(CReceivePacket* msg, IUser* user);
	bool OnClanStorageDeleteItem(CReceivePacket* msg, IUser* user);
	bool OnClanDissolveRequest(CReceivePacket* msg, IUser* user);
	bool OnClanDelegateMasterRequest(CReceivePacket* msg, IUser* user);
	bool OnClanMemberUserListRequest(CReceivePacket* msg, IUser* user);
	bool OnClanJoinUserListRequest(CReceivePacket* msg, IUser* user);
	bool OnClanChatMessage(CReceivePacket* msg, IUser* user);

	void OnUserLogin(IUser* user);
};

extern CClanManager g_ClanManager;