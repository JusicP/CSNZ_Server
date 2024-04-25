#include "room.h"
#include "manager/channelmanager.h"
#include "manager/packetmanager.h"
#include "manager/dedicatedservermanager.h"
#include "manager/userdatabase.h"
#include "serverconfig.h"
#include "common/utils.h"

#include "user/userinventoryitem.h"

using namespace std;

CRoom::CRoom(int roomId, IUser* hostUser, CChannel* channel, CRoomSettings* settings)
{
	m_nID = roomId;
	m_pGameMatch = NULL;

	m_pParentChannel = channel;

	m_pSettings = settings;

	m_Status = RoomStatus::STATUS_WAITING;

	AddUser(hostUser);

	m_pServer = NULL;
}

CRoom::~CRoom()
{
	delete m_pSettings;
	delete m_pGameMatch;

	if (m_pServer)
	{
		m_pServer->SetRoom(NULL);
		g_PacketManager.SendHostStop(m_pServer->GetSocket());
	}
}

int CRoom::GetNumOfPlayers()
{
	// TODO: should we count bot players???
	int realPlayers = m_Users.size();
	//int botPlayers = m_pSettings->unk39 + m_pSettings->unk40;

	return realPlayers /*+ botPlayers*/;
}

int CRoom::GetFreeSlots()
{
	int availableSlots = m_pSettings->maxPlayers - GetNumOfPlayers();
	return availableSlots >= 0 ? availableSlots : 0;
}

bool CRoom::HasFreeSlots()
{
	return GetFreeSlots() != 0;
}

bool CRoom::HasPassword()
{
	return m_pSettings->password.empty() != 1;
}

bool CRoom::HasUser(IUser* user)
{
	vector<IUser*>::iterator it = find(m_Users.begin(), m_Users.end(), user);
	if (it != m_Users.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CRoom::AddUser(IUser* user)
{
	m_Users.push_back(user);
	if (m_Users.size() == 1)
	{
		UpdateHost(user); // call this to update event items room settings
	}

	user->SetRoomData(new CRoomUser(user, RoomTeamNum::CounterTerrorist, RoomReadyStatus::READY_STATUS_NO));
	user->SetStatus(UserStatus::STATUS_INROOM);
}

void CRoom::RemoveUser(IUser* targetUser)
{
	m_Users.erase(remove(begin(m_Users), end(m_Users), targetUser), end(m_Users));

	delete targetUser->GetRoomData();
	targetUser->SetRoomData(NULL);

	OnUserRemoved(targetUser);

	targetUser->SetCurrentRoom(NULL);
}

RoomTeamNum CRoom::FindDesirableTeamNum()
{
	return RoomTeamNum::Unassigned;
}

RoomTeamNum CRoom::GetUserTeam(IUser* user)
{
	if (!HasUser(user))
	{
		return RoomTeamNum::Unassigned;
	}

	return user->GetRoomData()->m_Team;
}

int CRoom::GetNumOfReadyRealPlayers()
{
	int readyPlayersCount = 0;
	for (auto user : m_Users)
	{
		if (IsUserReady(user))
		{
			readyPlayersCount++;
		}
	}

	return readyPlayersCount;
}

int CRoom::GetNumOfRealCts()
{
	return 0;
}

int CRoom::GetNumOfRealTerrorists()
{
	return 0;
}

int CRoom::GetNumOfReadyPlayers()
{
	return 0;
}

RoomReadyStatus CRoom::IsUserReady(IUser* user)
{
	if (!HasUser(user))
	{
		Logger().Warn("CRoom::IsUserReady: user '%d, %s' not found\n", user->GetID(), user->GetUsername().c_str());
		return RoomReadyStatus::READY_STATUS_NO;
	}

	return user->GetRoomData()->m_Ready;
}

bool CRoom::IsRoomReady()
{
	return false;
}

bool CRoom::IsUserIngame(IUser* user)
{
	return user->GetStatus() == STATUS_PLAYING;
}

void CRoom::SetUserIngame(IUser* user, bool inGame)
{
	user->SetStatus(inGame ? UserStatus::STATUS_PLAYING : UserStatus::STATUS_INROOM);
	user->GetRoomData()->m_bIsIngame = inGame;
	user->GetRoomData()->m_Ready = inGame ? RoomReadyStatus::READY_STATUS_INGAME : RoomReadyStatus::READY_STATUS_NO;
}

void CRoom::SetUserToTeam(IUser* user, RoomTeamNum newTeam)
{
	if (!HasUser(user))
	{
		Logger().Warn("CRoom::SetUserToTeam: user '%d, %s' not found\n", user->GetID(), user->GetUsername().c_str());
		return;
	}

	user->GetRoomData()->m_Team = newTeam;
}

RoomReadyStatus CRoom::ToggleUserReadyStatus(IUser* user)
{
	if (!HasUser(user))
	{
		Logger().Warn("CRoom::ToggleUserReadyStatus: user '%d, %s' not found\n", user->GetID(), user->GetUsername().c_str());
		return RoomReadyStatus::READY_STATUS_NO;
	}

	if (user == m_pHostUser)
	{
		Logger().Warn("CRoom::ToggleUserReadyStatus: host user '%d, %s' tried to toggle ready status\n", user->GetID(), user->GetUsername().c_str());
		return RoomReadyStatus::READY_STATUS_NO;
	}

	RoomReadyStatus oldStatus = user->GetRoomData()->m_Ready;
	RoomReadyStatus newStatus = oldStatus == RoomReadyStatus::READY_STATUS_NO ? RoomReadyStatus::READY_STATUS_YES : RoomReadyStatus::READY_STATUS_NO;
	user->GetRoomData()->m_Ready = newStatus;

	return newStatus;
}

void CRoom::ResetStatusIngameUsers()
{
	for (auto u : m_Users)
	{
		if (u->GetRoomData() && u->GetRoomData()->m_Ready == RoomReadyStatus::READY_STATUS_INGAME)
		{
			u->GetRoomData()->m_Ready = RoomReadyStatus::READY_STATUS_NO;
		}
	}
}

RoomStatus CRoom::GetStatus()
{
	return (RoomStatus)m_pSettings->status;
}

void CRoom::SetStatus(RoomStatus newStatus)
{
	m_pSettings->status = newStatus;
	m_pSettings->statusSymbol = newStatus == RoomStatus::STATUS_INGAME ? 3 : 0;
}

void CRoom::SendJoinNewRoom(IUser* user)
{
	g_PacketManager.SendRoomCreateAndJoin(user->GetExtendedSocket(), this);
}

void CRoom::UpdateSettings(CRoomSettings& newSettings)
{
	m_pSettings->lowFlag |= newSettings.lowFlag;
	m_pSettings->lowMidFlag |= newSettings.lowMidFlag;
	m_pSettings->highMidFlag |= newSettings.highMidFlag;
	m_pSettings->highFlag |= newSettings.highFlag;

	if (newSettings.lowFlag & ROOM_LOW_ROOMNAME) {
		m_pSettings->roomName = newSettings.roomName;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK) {
		m_pSettings->unk00 = newSettings.unk00;
	}
	if (newSettings.lowFlag & ROOM_LOW_CLANBATTLE) {
		m_pSettings->unk01 = newSettings.unk01;
		m_pSettings->unk02 = newSettings.unk02;
		m_pSettings->unk03 = newSettings.unk03;
		m_pSettings->unk04 = newSettings.unk04;
	}
	if (newSettings.lowFlag & ROOM_LOW_PASSWORD) {
		m_pSettings->password = newSettings.password;
	}
	if (newSettings.lowFlag & ROOM_LOW_LEVELLIMIT) {
		m_pSettings->levelLimit = newSettings.levelLimit;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK7) {
		m_pSettings->unk07 = newSettings.unk07;
	}
	if (newSettings.lowFlag & ROOM_LOW_GAMEMODEID) {
		m_pSettings->gameModeId = newSettings.gameModeId;
	}
	if (newSettings.lowFlag & ROOM_LOW_MAPID) {
		m_pSettings->mapId = newSettings.mapId;
	}
	if (newSettings.lowFlag & ROOM_LOW_MAXPLAYERS) {
		m_pSettings->maxPlayers = newSettings.maxPlayers;
	}
	if (newSettings.lowFlag & ROOM_LOW_WINLIMIT) {
		m_pSettings->winLimit = newSettings.winLimit;
	}
	if (newSettings.lowFlag & ROOM_LOW_KILLLIMIT) {
		m_pSettings->killLimit = newSettings.killLimit;
	}
	if (newSettings.lowFlag & ROOM_LOW_GAMETIME) {
		m_pSettings->gameTime = newSettings.gameTime;
	}
	if (newSettings.lowFlag & ROOM_LOW_ROUNDTIME) {
		m_pSettings->roundTime = newSettings.roundTime;
	}
	if (newSettings.lowFlag & ROOM_LOW_WEAPONLIMIT) {
		m_pSettings->weaponLimit = newSettings.weaponLimit;
	}
	if (newSettings.lowFlag & ROOM_LOW_HOSTAGEKILLLIMIT) {
		m_pSettings->hostageKillLimit = newSettings.hostageKillLimit;
	}
	if (newSettings.lowFlag & ROOM_LOW_FREEZETIME) {
		m_pSettings->freezeTime = newSettings.freezeTime;
	}
	if (newSettings.lowFlag & ROOM_LOW_BUYTIME) {
		m_pSettings->buyTime = newSettings.buyTime;
	}
	if (newSettings.lowFlag & ROOM_LOW_DISPLAYNICKNAME) {
		m_pSettings->displayNickname = newSettings.displayNickname;
	}
	if (newSettings.lowFlag & ROOM_LOW_TEAMBALANCE) {
		m_pSettings->teamBalance = newSettings.teamBalance;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK21) {
		m_pSettings->unk21 = newSettings.unk21;
	}
	if (newSettings.lowFlag & ROOM_LOW_FRIENDLYFIRE) {
		m_pSettings->friendlyFire = newSettings.friendlyFire;
	}
	if (newSettings.lowFlag & ROOM_LOW_FLASHLIGHT) {
		m_pSettings->flashlight = newSettings.flashlight;
	}
	if (newSettings.lowFlag & ROOM_LOW_FOOTSTEPS) {
		m_pSettings->footsteps = newSettings.footsteps;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK25) {
		m_pSettings->unk25 = newSettings.unk25;
	}
	if (newSettings.lowFlag & ROOM_LOW_TKPUNISH) {
		m_pSettings->tkPunish = newSettings.tkPunish;
	}
	if (newSettings.lowFlag & ROOM_LOW_AUTOKICK) {
		m_pSettings->autoKick = newSettings.autoKick;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK28) {
		m_pSettings->unk28 = newSettings.unk28;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK29) {
		m_pSettings->unk29 = newSettings.unk29;
	}
	if (newSettings.lowFlag & ROOM_LOW_VIEWFLAG) {
		m_pSettings->viewFlag = newSettings.viewFlag;
	}
	if (newSettings.lowFlag & ROOM_LOW_VOICECHAT) {
		m_pSettings->voiceChat = newSettings.voiceChat;
	}
	if (newSettings.lowFlag & ROOM_LOW_STATUS) {
		m_pSettings->status = newSettings.status;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK33) {
		m_pSettings->unk33 = newSettings.unk33;
		m_pSettings->unk33_vec = newSettings.unk33_vec;
	}

	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK34) {
		m_pSettings->unk34 = newSettings.unk34;
		m_pSettings->unk35 = newSettings.unk35;
		m_pSettings->unk36 = newSettings.unk36;
		m_pSettings->unk37 = newSettings.unk37;
		m_pSettings->unk38 = newSettings.unk38;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_C4TIMER) {
		m_pSettings->c4Timer = newSettings.c4Timer;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_BOT) {
		m_pSettings->botDifficulty = newSettings.botDifficulty;
		m_pSettings->friendlyBots = newSettings.friendlyBots;
		m_pSettings->enemyBots = newSettings.enemyBots;
		m_pSettings->botBalance = newSettings.botBalance;
		m_pSettings->botAdd = newSettings.botAdd;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_KDRULE) {
		m_pSettings->kdRule = newSettings.kdRule;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_STARTINGCASH) {
		m_pSettings->startingCash = newSettings.startingCash;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MOVINGSHOT) {
		m_pSettings->movingShot = newSettings.movingShot;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_BALLNUMBER) {
		m_pSettings->ballNumber = newSettings.ballNumber;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_STATUSSYMBOL) {
		m_pSettings->statusSymbol = newSettings.statusSymbol;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_RANDOMMAP) {
		m_pSettings->randomMap = newSettings.randomMap;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MAPPLAYLIST) {
		m_pSettings->mapPlaylistSize = newSettings.mapPlaylistSize;
		m_pSettings->mapPlaylist = newSettings.mapPlaylist;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MAPPLAYLISTINDEX) {
		m_pSettings->mapPlaylistIndex = newSettings.mapPlaylistIndex;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ENHANCERESTRICT) {
		m_pSettings->enhanceRestrict = newSettings.enhanceRestrict;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_SD) {
		m_pSettings->sd = newSettings.sd;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ZSDIFFICULTY) {
		m_pSettings->zsDifficulty = newSettings.zsDifficulty;
		m_pSettings->unk56 = newSettings.unk56;
		m_pSettings->unk57 = newSettings.unk57;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_LEAGUERULE) {
		m_pSettings->leagueRule = newSettings.leagueRule;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MANNERLIMIT) {
		m_pSettings->mannerLimit = newSettings.mannerLimit;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MAPID2) {
		m_pSettings->mapId2 = newSettings.mapId2;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ZBLIMIT) {
		m_pSettings->zbLimitFlag = newSettings.zbLimitFlag;
		m_pSettings->zbLimit = newSettings.zbLimit;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK62) {
		m_pSettings->unk62 = newSettings.unk62;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK63) {
		m_pSettings->unk63 = newSettings.unk63;
		m_pSettings->unk63_vec = newSettings.unk63_vec;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK64) {
		m_pSettings->unk64 = newSettings.unk64;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_TEAMSWITCH) {
		m_pSettings->teamSwitch = newSettings.teamSwitch;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ZBRESPAWN) {
		m_pSettings->zbRespawn = newSettings.zbRespawn;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ZBBALANCE) {
		m_pSettings->zbBalance = newSettings.zbBalance;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_GAMERULE) {
		m_pSettings->gameRule = newSettings.gameRule;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_SUPERROOM) {
		m_pSettings->superRoom = newSettings.superRoom;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ISZBCOMPETITIVE) {
		m_pSettings->isZbCompetitive = newSettings.isZbCompetitive;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ZBAUTOHUNTING) {
		m_pSettings->zbAutoHunting = newSettings.zbAutoHunting;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_INTEGRATEDTEAM) {
		m_pSettings->integratedTeam = newSettings.integratedTeam;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK73) {
		m_pSettings->unk73 = newSettings.unk73;
	}

	if (newSettings.highMidFlag & ROOM_HIGHMID_FIREBOMB) {
		m_pSettings->fireBomb = newSettings.fireBomb;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_MUTATIONRESTRICT) {
		m_pSettings->mutationRestrict = newSettings.mutationRestrict;
		m_pSettings->mutationRestrictList = newSettings.mutationRestrictList;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_MUTATIONLIMIT) {
		m_pSettings->mutationLimit = newSettings.mutationLimit;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_FLOATINGDAMAGESKIN) {
		m_pSettings->floatingDamageSkin = newSettings.floatingDamageSkin;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_PLAYERONETEAM) {
		m_pSettings->playerOneTeam = newSettings.playerOneTeam;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_WEAPONRESTRICT) {
		m_pSettings->weaponRestrict = newSettings.weaponRestrict;
	}
	if (newSettings.highFlag & ROOM_HIGH_UNK77) {
		m_pSettings->unk77 = newSettings.unk77;
	}
}

void CRoom::OnUserMessage(CReceivePacket* msg, IUser* user)
{
	string message = msg->ReadString();

	CUserCharacter character = user->GetCharacter(UFLAG_GAMENAME);
	string senderName = character.gameName;

	Logger().Info("User '%s' write to room chat: '%s'\n", senderName.c_str(), message.c_str());

	if (!message.find("/changegm") && m_pHostUser == user)
	{
		istringstream iss(message);
		vector<string> results((istream_iterator<string>(iss)),
			istream_iterator<string>());

		if (results.size() == 2)
		{
			if (isNumber(results[1]))
			{
				m_pSettings->gameModeId = stoi(results[1]);

				for (auto u : m_Users) // send gamemode update to all users
				{
					g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, ROOM_LOW_GAMEMODEID);
				}
			}
			else
			{
				g_PacketManager.SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changegm usage: /changegm <gameMode>");
			}
		}
		else
		{
			g_PacketManager.SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changegm usage: /changegm <gameMode>");
		}

		return;
	}
	else if (!message.find("/changemap") && m_pHostUser == user)
	{
		istringstream iss(message);
		vector<string> results((istream_iterator<string>(iss)),
			istream_iterator<string>());

		if (results.size() == 2)
		{
			if (isNumber(results[1]))
			{
				m_pSettings->mapId = stoi(results[1]);
				m_pSettings->mapId2 = m_pSettings->mapId;

				for (auto u : m_Users)
				{
					g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, ROOM_LOW_MAPID, ROOM_LOWMID_MAPID2);
				}
			}
			else
			{
				g_PacketManager.SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changemap usage: /changemap <mapId>");
			}
		}
		else
		{
			g_PacketManager.SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changemap usage: /changemap <mapId>");
		}

		return;
	}
	else if (!message.find("/changebotscount") && m_pHostUser == user)
	{
		istringstream iss(message);
		vector<string> results((istream_iterator<string>(iss)),
			istream_iterator<string>());

		if (results.size() == 3)
		{
			if (isNumber(results[1]) && isNumber(results[2]))
			{
				m_pSettings->enemyBots = stoi(results[1]);
				m_pSettings->friendlyBots = stoi(results[2]);
				m_pSettings->botAdd = 1;

				for (auto u : m_Users)
				{
					g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, 0, ROOM_LOWMID_BOT);
				}
			}
		}
		else
		{
			g_PacketManager.SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changebotscount usage: /changebotscount <enemy bots> <friendly bots>");
		}

		return;
	}

	for (auto userDest : m_Users)
	{
		CUserCharacterExtended characterExtendedDest = userDest->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);

		// you can't send room message if dest is blocking your chat
		if (~characterExtendedDest.banSettings & 2 || characterExtendedDest.banSettings & 2 && !g_UserDatabase.IsInBanList(userDest->GetID(), user->GetID()))
			g_PacketManager.SendUMsgUserMessage(userDest->GetExtendedSocket(), UMsgPacketType::RoomUserMessage, senderName, message);
	}
}

void CRoom::OnUserTeamMessage(CReceivePacket* msg, IUser* user)
{
	string message = msg->ReadString();
	int userTeam = GetUserTeam(user);

	CUserCharacter character = user->GetCharacter(UFLAG_GAMENAME);

	for (auto userDest : m_Users)
	{
		if (userTeam == GetUserTeam(userDest))
		{
			CUserCharacterExtended characterExtendedDest = userDest->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);

			// you can't send team message if dest is blocking your chat
			if (~characterExtendedDest.banSettings & 2 || characterExtendedDest.banSettings & 2 && !g_UserDatabase.IsInBanList(userDest->GetID(), user->GetID()))
				g_PacketManager.SendUMsgUserMessage(userDest->GetExtendedSocket(), UMsgPacketType::RoomTeamUserMessage, character.gameName, message);
		}
	}
}

void CRoom::OnGameStart()
{
	SetStatus(RoomStatus::STATUS_INGAME);
	SetUserIngame(m_pHostUser, true);

	for (auto u : m_Users)
	{
		SendRoomStatus(u);

		if (IsUserReady(u))
		{
			if (m_pServer != NULL)
				UserGameJoin(u);
			else if (u != m_pHostUser)
				UserGameJoin(u);
		}
	}
}

void CRoom::KickUser(IUser* user)
{
	m_KickedUsers.push_back(user->GetID());

	for (auto u : m_Users)
	{
		g_PacketManager.SendRoomKick(u->GetExtendedSocket(), user->GetID());
	}
}

void CRoom::VoteKick(IUser* user, bool kick)
{
	Logger().Warn("CRoom::VoteKick: not implemented!\n");
}

void CRoom::SendRoomSettings(IUser* user)
{
	g_PacketManager.SendRoomUpdateSettings(user->GetExtendedSocket(), m_pSettings);
}

void CRoom::SendUpdateRoomSettings(IUser* user, CRoomSettings* settings, int lowFlag, int lowMidFlag, int highMidFlag, int highFlag)
{
	g_PacketManager.SendRoomUpdateSettings(user->GetExtendedSocket(), settings, lowFlag, lowMidFlag, highMidFlag, highFlag);
}

void CRoom::SendRoomUsersReadyStatus(IUser* user)
{
	for (auto u : m_Users)
	{
		SendUserReadyStatus(user, u);
	}
}

void CRoom::SendReadyStatusToAll()
{
	for (auto u : m_Users)
	{
		SendRoomUsersReadyStatus(u);
	}
}

void CRoom::SendReadyStatusToAll(IUser* user)
{
	for (auto u : m_Users)
	{
		SendUserReadyStatus(u, user);
	}
}

void CRoom::SendNewUser(IUser* user, IUser* newUser)
{
	g_PacketManager.SendRoomPlayerJoin(user->GetExtendedSocket(), newUser, RoomTeamNum::CounterTerrorist);
}

void CRoom::SendUserReadyStatus(IUser* user, IUser* player)
{
	if (player->GetRoomData() == NULL)
	{
		Logger().Error("CRoom::SendUserReadyStatus: GetRoomData() == NULL, users count: %d\n", m_Users.size());
		return;
	}
	g_PacketManager.SendRoomSetPlayerReady(user->GetExtendedSocket(), player, player->GetRoomData()->m_Ready);
}

void CRoom::SendConnectHost(IUser* user, IUser* host)
{
	//g_PacketManager.SendUDPHostData(user->GetExtendedSocket(), true, host->GetData()->userId, host->GetNetworkConfig().m_szExternalIpAddress, host->GetNetworkConfig().m_nExternalServerPort);
	if (g_pServerConfig->room.connectingMethod)
	{
		g_PacketManager.SendHostJoin(user->GetExtendedSocket(), host);
		//g_PacketManager.SendHostServerJoin(user->GetExtendedSocket(), ip_string_to_int(host->GetNetworkConfig().m_szExternalIpAddress), false, host->GetNetworkConfig().m_nExternalServerPort, user->GetID());
	}
	else
	{
		if (m_pServer)
			g_PacketManager.SendHostServerJoin(user->GetExtendedSocket(), m_pServer->GetIP(), true, m_pServer->GetPort(), user->GetID());
		else
			g_PacketManager.SendHostJoin(user->GetExtendedSocket(), host);
			//g_PacketManager.SendHostServerJoin(user->GetExtendedSocket(), ip_string_to_int(host->GetNetworkConfig().m_szExternalIpAddress), false, host->GetNetworkConfig().m_nExternalServerPort, user->GetID());
	}
}

void CRoom::SendGuestData(IUser* host, IUser* guest)
{
	//CPacket_UDP hostData(host->m_pSocket);
	//hostData.Send(hostData.Build(0, guest->m_pData->userId, guest->externalIpAddress, 27015));
}

void CRoom::SendStartMatch(IUser* host)
{
	if (g_pServerConfig->room.connectingMethod)
	{
		g_PacketManager.SendHostGameStart(host->GetExtendedSocket(), host->GetID());
	}
	else
	{
		m_pServer = g_DedicatedServerManager.GetAvailableServerFromPools(this);

		if (m_pServer)
		{
			g_PacketManager.SendRoomCreateAndJoin(m_pServer->GetSocket(), this);
			g_PacketManager.SendHostGameStart(m_pServer->GetSocket(), m_pServer->GetPort());
		}
		else
		{
			g_PacketManager.SendHostGameStart(host->GetExtendedSocket(), host->GetID());
		}
	}

}

void CRoom::SendCloseResultWindow(IUser* user)
{
	g_PacketManager.SendHostLeaveResultWindow(user->GetExtendedSocket());
}

void CRoom::SendTeamChange(IUser* user, IUser* player, RoomTeamNum newTeamNum)
{
	g_PacketManager.SendRoomSetUserTeam(user->GetExtendedSocket(), player, newTeamNum);
}

void CRoom::SendGameEnd(IUser* user)
{
	g_PacketManager.SendHostStop(user->GetExtendedSocket());
	g_PacketManager.SendRoomGameResult(user->GetExtendedSocket(), this, m_pGameMatch);

	CGameMatchUserStat* stat = m_pGameMatch->GetGameUserStat(user);
	if (stat)
	{
		g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), va("m_nKills: %d\nm_nDeaths: %d\nm_nScore: %d", stat->m_nKills, stat->m_nDeaths, stat->m_nScore));
	}
}

void CRoom::SendRoomStatus(IUser* user)
{
	g_PacketManager.SendRoomUpdateSettings(user->GetExtendedSocket(), m_pSettings, ROOM_LOW_STATUS, ROOM_LOWMID_STATUSSYMBOL);
}

void CRoom::SendPlayerLeaveIngame(IUser* user)
{
	g_PacketManager.SendRoomPlayerLeaveIngame(user->GetExtendedSocket());
}

void CRoom::CheckForHostItems()
{
	if (!m_pHostUser)
		return;

	CUserInventoryItem item;
	m_pSettings->superRoom = g_UserDatabase.GetFirstActiveItemByItemID(m_pHostUser->GetID(), 8357 /* superRoom */, item);

	for (auto u : m_Users)
	{
		g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, 0, ROOM_LOWMID_SUPERROOM);
	}

	CUserInventoryItem item2;
	m_pSettings->c4Timer = g_UserDatabase.GetFirstActiveItemByItemID(m_pHostUser->GetID(), 112 /* c4Timer */, item2);

	for (auto u : m_Users)
	{
		g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, 0, ROOM_LOWMID_C4TIMER);
	}

	if (m_pSettings->gameModeId == 3 || m_pSettings->gameModeId == 4 || m_pSettings->gameModeId == 5 || m_pSettings->gameModeId == 15 || m_pSettings->gameModeId == 24)
	{
		CUserInventoryItem item;
		m_pSettings->sd = g_UserDatabase.GetFirstActiveItemByItemID(m_pHostUser->GetID(), 439 /* BigHeadEvent */, item);

		for (auto u : m_Users)
		{
			g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, 0, ROOM_LOWMID_SD);
		}
	}
}

void CRoom::OnUserRemoved(IUser* user)
{
	if (m_pGameMatch)
	{
		m_pGameMatch->Disconnect(user); // remove from game match user list
	}

	if (m_Users.size() > 0)
	{
		SendRemovedUser(user);
		if (user == m_pHostUser)
		{
			FindAndUpdateNewHost();
		}
	}
	else
	{
		m_pParentChannel->RemoveRoomById(m_nID);
		//m_pParentChannel->OnEmptyRoomCallback(this);
	}

	user->SetStatus(UserStatus::STATUS_MENU);
}

void CRoom::SendRemovedUser(IUser* deletedUser)
{
	for (auto u : m_Users)
	{
		g_PacketManager.SendRoomPlayerLeave(u->GetExtendedSocket(), deletedUser->GetID());
	}
}

// TODO: remove unnecessary send calls or group it into one
void CRoom::UpdateHost(IUser* newHost)
{
	m_pHostUser = newHost;

	for (auto u : m_Users)
	{
		g_PacketManager.SendRoomSetHost(u->GetExtendedSocket(), newHost);
	}

	if (m_pServer == NULL && m_pGameMatch != NULL)
	{
		if (m_pGameMatch->m_UserStats.empty())
		{
			EndGame(true);
		}
		else
		{
			m_pGameMatch->OnHostChanged(newHost);
		}
	}

	CheckForHostItems();
}

bool CRoom::FindAndUpdateNewHost()
{
	if (m_Users.size() == 0)
	{
		return false;
	}

	if (m_pServer == NULL && m_pGameMatch != NULL)
	{
		vector<CGameMatchUserStat*> userStats = m_pGameMatch->m_UserStats;

		if (userStats.empty())
			UpdateHost(m_Users[0]);
		else
			UpdateHost(userStats[0]->m_pUser);
	}
	else
	{
		UpdateHost(m_Users[0]);
	}

	return true;
}

void CRoom::HostStartGame()
{
	// set random map for zb competitive
	if (m_pSettings->isZbCompetitive)
	{
		vector<int> zbCompetitiveMaps;
		vector<int> column = g_pMapListTable->GetColumn<int>("map_name");
		for (auto mapID : column)
		{
			if (g_pMapListTable->GetCell<int>("zb_competitive", std::to_string(mapID)))
			{
				zbCompetitiveMaps.push_back(mapID);
			}
		}

		Randomer randomMap(zbCompetitiveMaps.size() - 1);
		m_pSettings->mapId = zbCompetitiveMaps[randomMap()];
		m_pSettings->mapId2 = m_pSettings->mapId;

		for (auto u : m_Users)
		{
			g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, ROOM_LOW_MAPID, ROOM_LOWMID_MAPID2);
		}
	}

	if (!m_pGameMatch)
	{
		m_pGameMatch = new CGameMatch(this, m_pSettings->gameModeId, m_pSettings->mapId);
	}
	else
	{
		Logger().Error("CRoom::HostStartGame: m_pGameMatch != NULL\n");
		return;
	}

	SendStartMatch(m_pHostUser);

	SendReadyStatusToAll();

	m_pHostUser->GetCurrentChannel()->SendUpdateRoomList(this);

	if (m_pServer)
	{
		string ip = ip_to_string(m_pServer->GetIP());

		Logger().Info("Host '%s' started room match (RID: %d, IP: %s, port: %d)\n", m_pHostUser->GetUsername().c_str(), m_nID, ip.c_str(), m_pServer->GetPort());
	}
	else
	{
		Logger().Info("Host '%s' started room match (RID: %d, IP: %s, port: %d)\n", m_pHostUser->GetUsername().c_str(), m_nID, m_pHostUser->GetNetworkConfig().m_szExternalIpAddress.c_str(), m_pHostUser->GetNetworkConfig().m_nExternalClientPort);
	}
}

void CRoom::UserGameJoin(IUser* user)
{
	SetUserIngame(user, true);
	SendGuestData(m_pHostUser, user);
	SendConnectHost(user, m_pHostUser);
	SendReadyStatusToAll(user);

	//m_pGameMatch->Connect(user);

	Logger().Info("User '%d, %s' joining room match (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), m_nID);
}

void CRoom::EndGame(bool forcedEnd)
{
	m_pGameMatch->OnGameMatchEnd();

	SetStatus(RoomStatus::STATUS_WAITING);
	ResetStatusIngameUsers();

	if (m_pServer)
		g_PacketManager.SendHostStop(m_pServer->GetSocket());

	for (auto u : m_Users)
	{
		SendRoomStatus(u);
		if (u->GetRoomData()->m_bIsIngame)
		{
			SetUserIngame(u, false);
			SendGameEnd(u);
		}
	}

	SendReadyStatusToAll();

	if (!forcedEnd && m_pSettings->mapPlaylistSize)
	{
		for (int i = 0; i <= m_pSettings->mapPlaylistSize - 1; i++)
		{
			if (m_pSettings->mapId == m_pSettings->mapPlaylist[i].mapId)
			{
				int nextMap = i + 1;
				if (nextMap == m_pSettings->mapPlaylistSize)
					nextMap = 0;

				m_pSettings->mapId = m_pSettings->mapPlaylist[nextMap].mapId;
				m_pSettings->mapId2 = m_pSettings->mapId;
				m_pSettings->mapPlaylistIndex = nextMap + 1;

				for (auto u : m_Users)
				{
					g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, ROOM_LOW_MAPID, ROOM_LOWMID_MAPID2 | ROOM_LOWMID_MAPPLAYLISTINDEX);
				}
				break;
			}
		}
	}

	delete m_pGameMatch;
	m_pGameMatch = NULL;
}

int CRoom::GetID()
{
	return m_nID;
}

IUser* CRoom::GetHostUser()
{
	return m_pHostUser;
}

vector<IUser*> CRoom::GetUsers()
{
	return m_Users;
}

CRoomSettings* CRoom::GetSettings()
{
	return m_pSettings;
}

CGameMatch* CRoom::GetGameMatch()
{
	return m_pGameMatch;
}

CChannel* CRoom::GetParentChannel()
{
	return m_pParentChannel;
}

bool CRoom::IsUserKicked(int userID)
{
	return find(m_KickedUsers.begin(), m_KickedUsers.end(), userID) != m_KickedUsers.end();
}

CDedicatedServer* CRoom::GetServer()
{
	return m_pServer;
}

void CRoom::SetServer(CDedicatedServer* server)
{
	m_pServer = server;
}

void CRoom::ChangeMap(int mapId)
{
	m_pSettings->mapId = mapId;
	m_pSettings->mapId2 = m_pSettings->mapId;

	if (m_pServer != NULL)
	{
		g_PacketManager.SendRoomUpdateSettings(m_pServer->GetSocket(), m_pSettings, ROOM_LOW_MAPID, ROOM_LOWMID_MAPID2);
	}

	for (auto u : m_Users)
	{
		g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), m_pSettings, ROOM_LOW_MAPID, ROOM_LOWMID_MAPID2);
	}
}