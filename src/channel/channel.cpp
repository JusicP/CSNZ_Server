#include "channel.h"
#include "manager/usermanager.h"
#include "manager/packetmanager.h"
#include "manager/userdatabase.h"

using namespace std;

CChannel::CChannel(CChannelServer* server, int id, const std::string& channelName, int maxPlayers, const std::string& loginMsg)
{
	m_nID = id;
	m_szName = channelName;
	m_nNextRoomID = 1;
	m_pParentChannelServer = server;
	m_nMaxPlayers = maxPlayers;
	m_LoginMsg = loginMsg;

	m_Users.reserve(maxPlayers);
}

bool CChannel::UserJoin(IUser* user, bool unhide)
{
	if (unhide)
	{
		for (auto u : m_Users)
		{
			if (u == user || u->GetCurrentRoom())
				continue;

			g_PacketManager.SendLobbyUserJoin(u->GetExtendedSocket(), user);
		}

		return true;
	}

	if ((int)m_Users.size() >= m_nMaxPlayers)
	{
		g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("SERVER_SELECT_FAIL_LOBBY_FULL"));

		return false;
	}

	m_Users.push_back(user);

	for (auto u : m_Users) // send lobby update packet for all users in channel
	{
		if (u == user)
			continue;

		g_PacketManager.SendLobbyUserJoin(u->GetExtendedSocket(), user);
	}

	g_PacketManager.SendLobbyJoin(user->GetExtendedSocket(), this);
	g_PacketManager.SendRoomListFull(user->GetExtendedSocket(), m_Rooms);

	if (!m_LoginMsg.empty())
	{
		g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), m_LoginMsg);
	}

	return true;
}

void CChannel::UserLeft(IUser* user, bool hide)
{
	if (user->GetCurrentRoom() && !hide)
	{
		Logger().Info(OBFUSCATE("User '%d' tried to leave channel with active room\n"), user->GetID());
		return;
	}

	if (!hide && !RemoveUserById(user->GetID()))
	{
		Logger().Info(OBFUSCATE("CChannel::UserLeft: couldn't find user with %d ID. User will remain in channel user list\n"), user->GetID());
	}

	for (auto u : m_Users) // send lobby update packet for all users in channel
	{
		if (/*u == user || */u->GetCurrentRoom())
			continue;

		g_PacketManager.SendLobbyUserLeft(u->GetExtendedSocket(), user);
	}
}

void CChannel::SendFullUpdateRoomList()
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_PacketManager.SendRoomListFull(u->GetExtendedSocket(), m_Rooms);
	}
}

void CChannel::SendFullUpdateRoomList(IUser* user)
{
	g_PacketManager.SendRoomListFull(user->GetExtendedSocket(), m_Rooms);
}

void CChannel::SendUpdateRoomList(IRoom* room)
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_PacketManager.SendRoomListUpdate(u->GetExtendedSocket(), room);
	}
}

void CChannel::SendAddRoomToRoomList(IRoom* room)
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_PacketManager.SendRoomListAdd(u->GetExtendedSocket(), room);
	}
}

void CChannel::SendRemoveFromRoomList(int roomId)
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_PacketManager.SendRoomListRemove(u->GetExtendedSocket(), roomId);
	}
}

void CChannel::SendUserMessageToAllUser(int type, int senderUserID, const std::string& senderName, const std::string& msg)
{
	for (auto userDest : m_Users)
	{
		CUserCharacterExtended characterExtendedDest = userDest->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);

		// you can't send lobby message if dest is blocking your chat
		if (~characterExtendedDest.banSettings & 2 || characterExtendedDest.banSettings & 2 && !g_UserDatabase.IsInBanList(userDest->GetID(), senderUserID))
			g_PacketManager.SendUMsgUserMessage(userDest->GetExtendedSocket(), type, senderName, msg);
	}
}

void CChannel::UpdateUserInfo(IUser* user, const CUserCharacter& character)
{
	for (auto u : m_Users)
	{
		g_PacketManager.SendUserUpdateInfo(u->GetExtendedSocket(), user, character);
	}
}

void CChannel::OnEmptyRoomCallback(IRoom* room)
{
	if (!RemoveRoomById(room->GetID()))
		Logger().Info(OBFUSCATE("CChannel::OnEmptyRoomCallback: couldn't find room with %d ID. Room will remain in channel room list\n"), room->GetID());

	delete room;
}

IRoom* CChannel::GetRoomById(int id)
{
	for (auto room : m_Rooms)
	{
		if (room->GetID() == id)
			return room;
	}
	return NULL;
}

IUser* CChannel::GetUserById(int userId)
{
	for (auto user : m_Users)
	{
		if (user->GetID() == userId)
			return user;
	}
	return NULL;
}

IRoom* CChannel::CreateRoom(IUser* host, CRoomSettings* settings)
{
	m_Rooms.push_back(new CRoom(m_nNextRoomID++, host, this, settings));
	return m_Rooms[m_Rooms.size() - 1];
}

bool CChannel::RemoveRoomById(int roomID)
{
	for (auto room : m_Rooms)
	{
		if (room->GetID() == roomID)
		{
			m_Rooms.erase(remove(begin(m_Rooms), end(m_Rooms), room), end(m_Rooms));
			return true;
		}
	}

	return false;
}

bool CChannel::RemoveUserById(int userID)
{
	for (auto user : m_Users)
	{
		if (user->GetID() == userID)
		{
			m_Users.erase(remove(begin(m_Users), end(m_Users), user), end(m_Users));
			return true;
		}
	}

	return false;
}

int CChannel::GetID()
{
	return m_nID;
}

std::string CChannel::GetName()
{
	return m_szName;
}

std::vector<IRoom*> CChannel::GetRooms()
{
	return m_Rooms;
}

std::vector<IUser*> CChannel::GetUsers()
{
	return m_Users;
}

std::vector<IUser*> CChannel::GetOutsideUsers()
{
	std::vector<IUser*> temp;
	for (auto& u : m_Users)
		if (u->GetCurrentRoom() == nullptr)
			temp.push_back(u);
	return temp;
}

CChannelServer* CChannel::GetParentChannelServer()
{
	return m_pParentChannelServer;
}