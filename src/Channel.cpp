#include "Channel.h"
#include "UserManager.h"
#include "PacketManager.h"

using namespace std;

CChannel::CChannel(CChannelServer* server, int idx, string channelName, int maxPlayers, string loginMsg)
{
	m_nIndex = idx;
	m_szName = channelName;
	m_nNextRoomID = 1;
	m_pParentChannelServer = server;
	m_nMaxPlayers = maxPlayers;
	m_LoginMsg = loginMsg;

	m_Users.reserve(maxPlayers);
}

bool CChannel::UserJoin(CUser* user, bool unhide)
{
	if (unhide)
	{
		for (auto u : m_Users)
		{
			if (u == user || u->GetCurrentRoom())
				continue;

			g_pPacketManager->SendLobbyUserJoin(u->GetExtendedSocket(), user);
		}

		return true;
	}

	if ((int)m_Users.size() >= m_nMaxPlayers)
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("SERVER_SELECT_FAIL_LOBBY_FULL"));

		return false;
	}

	m_Users.push_back(user);

	for (auto u : m_Users) // send lobby update packet for all users in channel
	{
		if (u == user)
			continue;

		g_pPacketManager->SendLobbyUserJoin(u->GetExtendedSocket(), user);
	}

	g_pPacketManager->SendLobbyJoin(user->GetExtendedSocket(), this);
	g_pPacketManager->SendRoomListFull(user->GetExtendedSocket(), m_Rooms);

	if (!m_LoginMsg.empty())
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), m_LoginMsg);
	}

	return true;
}

void CChannel::UserLeft(CUser* user, bool hide)
{
	if (user->GetCurrentRoom() && !hide)
	{
		g_pConsole->Log(OBFUSCATE("User '%d' tried to leave channel with active room\n"), user->GetID());
		return;
	}

	if (!hide && !RemoveUserById(user->GetID()))
	{
		g_pConsole->Log(OBFUSCATE("CChannel::UserLeft: couldn't find user with %d ID. User will remain in channel user list\n"), user->GetID());
	}

	for (auto u : m_Users) // send lobby update packet for all users in channel
	{
		if (/*u == user || */u->GetCurrentRoom())
			continue;

		g_pPacketManager->SendLobbyUserLeft(u->GetExtendedSocket(), user);
	}
}

void CChannel::SendFullUpdateRoomList()
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_pPacketManager->SendRoomListFull(u->GetExtendedSocket(), m_Rooms);
	}
}

void CChannel::SendFullUpdateRoomList(CUser* user)
{
	g_pPacketManager->SendRoomListFull(user->GetExtendedSocket(), m_Rooms);
}

void CChannel::SendUpdateRoomList(CRoom* room)
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_pPacketManager->SendRoomListUpdate(u->GetExtendedSocket(), room);
	}
}

void CChannel::SendAddRoomToRoomList(CRoom* room)
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_pPacketManager->SendRoomListAdd(u->GetExtendedSocket(), room);
	}
}

void CChannel::SendRemoveFromRoomList(int roomId)
{
	for (auto u : m_Users)
	{
		if (u->GetCurrentRoom())
			continue;

		g_pPacketManager->SendRoomListRemove(u->GetExtendedSocket(), roomId);
	}
}

void CChannel::SendLobbyMessageToAllUser(string senderName, string msg)
{
	for (auto u : m_Users)
	{
		g_pPacketManager->SendUMsgLobbyMessage(u->GetExtendedSocket(), senderName, msg);
	}
}

void CChannel::UpdateUserInfo(CUser* user, const CUserCharacter& character)
{
	for (auto u : m_Users)
	{
		g_pPacketManager->SendUserUpdateInfo(u->GetExtendedSocket(), user, character);
	}
}

void CChannel::OnEmptyRoomCallback(CRoom* room)
{
	if (!RemoveRoomById(room->GetID()))
		g_pConsole->Log(OBFUSCATE("CChannel::OnEmptyRoomCallback: couldn't find room with %d ID. Room will remain in channel room list\n"), room->GetID());

	delete room;
}

CRoom* CChannel::GetRoomById(int id)
{
	for (auto room : m_Rooms)
	{
		if (room->GetID() == id)
			return room;
	}
	return NULL;
}

CUser* CChannel::GetUserById(int userId)
{
	for (auto user : m_Users)
	{
		if (user->GetID() == userId)
			return user;
	}
	return NULL;
}

CRoom* CChannel::CreateRoom(CUser* host, CRoomSettings* settings)
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

CChannelServer* CChannel::GetParentChannelServer()
{
	return m_pParentChannelServer;
}