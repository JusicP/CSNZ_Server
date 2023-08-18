#include "Room.h"
#include "ChannelManager.h"
#include "PacketManager.h"
#include "ServerConfig.h"
#include "Utils.h"

using namespace std;

#define DEFAULT_COUNTDOWN 6
#define INVALID_ENT -1

CRoom::CRoom(int roomId, CUser* hostUser, CChannel* channel, IRoomOptions_s optionsRoomCallback)
{
	m_nID = roomId;
	m_pHostUser = hostUser;
	m_pGameMatch = NULL;

	m_pParentChannel = channel;

	m_pSettings = new CRoomSettings();

	m_Status = RoomStatus::STATUS_WAITING;

	m_bCountingDown = false;
	m_Countdown = DEFAULT_COUNTDOWN;

	AddUser(m_pHostUser);

	m_pServer = NULL;
}

CRoom::~CRoom()
{
	delete m_pSettings;
	delete m_pGameMatch;

	if (m_pServer)
	{
		m_pServer->SetRoom(NULL);
		g_pPacketManager->SendHostStop(m_pServer->GetSocket());
	}
}

int CRoom::GetNumOfPlayers()
{
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

bool CRoom::HasUser(CUser* user)
{
	vector<CUser*>::iterator it = find(m_Users.begin(), m_Users.end(), user);
	if (it != m_Users.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CRoom::AddUser(CUser* user)
{
	if (m_Users.size() <= 0) // make new user host if there are no users in room
	{
		m_pHostUser = user;
	}

	m_Users.push_back(user);

	user->SetRoomData(new CRoomUser(user, RoomTeamNum::CounterTerrorist, RoomReadyStatus::READY_STATUS_NO));
	user->SetStatus(UserStatus::STATUS_INROOM);
}

void CRoom::RemoveUser(CUser* targetUser)
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

RoomTeamNum CRoom::GetUserTeam(CUser* user)
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

RoomReadyStatus CRoom::GetUserReadyStatus(CUser* user)
{
	return RoomReadyStatus::READY_STATUS_NO;
}

RoomReadyStatus CRoom::IsUserReady(CUser* user)
{
	if (!HasUser(user))
	{
		g_pConsole->Warn("CRoom::IsUserReady: user '%d, %s' not found\n", user->GetID(), user->GetUsername().c_str());
		return RoomReadyStatus::READY_STATUS_NO;
	}

	return user->GetRoomData()->m_Ready;
}

bool CRoom::IsRoomReady()
{
	return false;
}

bool CRoom::IsUserIngame(CUser* user)
{
	return user->GetStatus() == STATUS_PLAYING;
}

void CRoom::SetUserIngame(CUser* user, bool inGame)
{
	user->SetStatus(inGame ? UserStatus::STATUS_PLAYING : UserStatus::STATUS_INROOM);
	user->GetRoomData()->m_bIsIngame = inGame;
	user->GetRoomData()->m_Ready = inGame ? RoomReadyStatus::READY_STATUS_INGAME : RoomReadyStatus::READY_STATUS_NO;
}

void CRoom::SetUserToTeam(CUser* user, RoomTeamNum newTeam)
{
	if (!HasUser(user))
	{
		g_pConsole->Warn("CRoom::SetUserToTeam: user '%d, %s' not found\n", user->GetID(), user->GetUsername().c_str());
		return;
	}

	user->GetRoomData()->m_Team = newTeam;
}

RoomReadyStatus CRoom::ToggleUserReadyStatus(CUser* user)
{
	if (!HasUser(user))
	{
		g_pConsole->Warn("CRoom::ToggleUserReadyStatus: user '%d, %s' not found\n", user->GetID(), user->GetUsername().c_str());
		return RoomReadyStatus::READY_STATUS_NO;
	}

	if (user == m_pHostUser)
	{
		g_pConsole->Warn("CRoom::ToggleUserReadyStatus: host user '%d, %s' tried to toggle ready status\n", user->GetID(), user->GetUsername().c_str());
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
	m_pSettings->unk47 = newStatus == RoomStatus::STATUS_INGAME ? 3 : 0;
}

bool CRoom::CanCountdown(CUser* user)
{
	return user == m_pHostUser && m_Status == RoomStatus::STATUS_WAITING;
}

void CRoom::ProgressCountdown(int hostNextNum)
{
	if (m_Countdown > DEFAULT_COUNTDOWN || m_Countdown < 0)
	{
		g_pConsole->Warn("CRoom::ProgressCountdown: invalid countdown\n");
		m_Countdown = 0;
	}

	if (m_bCountingDown == false)
	{
		m_bCountingDown = true;
	}

	if (m_Countdown != hostNextNum)
	{
		g_pConsole->Warn("CRoom::ProgressCountdown: client's countdown does not match server's\n");
	}

	m_Countdown--;
}

int CRoom::GetCountdown()
{
	if (m_bCountingDown == false)
	{
		g_pConsole->Warn("CRoom::GetCountdown: tried to get countdown without counting down\n");
		return 0;
	}

	if (m_Countdown > DEFAULT_COUNTDOWN || m_Countdown < 0)
	{
		g_pConsole->Warn("CRoom::GetCountdown: server's countdown is out of bounds %d\n", m_Countdown);
		m_Countdown = 0;
	}

	return m_Countdown;
}

bool CRoom::IsGlobalCountdownInProgress()
{
	return m_bCountingDown;
}

void CRoom::StopCountdown()
{
	m_bCountingDown = false;
	m_Countdown = DEFAULT_COUNTDOWN;
}

void CRoom::SendJoinNewRoom(CUser* user)
{
	g_pPacketManager->SendRoomCreateAndJoin(user->GetExtendedSocket(), this);
}

CRoomSettings* CRoom::UpdateSettings(CPacketHelper_RoomUpdateSettings newSettings)
{
	CRoomSettings* updatedSet = new CRoomSettings();

	if (newSettings.lowFlag & ROOM_LOW_NAME) {
		updatedSet->roomName = newSettings.roomName;
		m_pSettings->roomName = newSettings.roomName;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK) {
		updatedSet->unk00 = newSettings.unk00;
		m_pSettings->unk00 = newSettings.unk00;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK2) {
		m_pSettings->unk01 = newSettings.unk01;
		m_pSettings->unk02 = newSettings.unk02;
		m_pSettings->unk03 = newSettings.unk03;
		m_pSettings->unk04 = newSettings.unk04;
		updatedSet->unk01 = newSettings.unk01;
		updatedSet->unk02 = newSettings.unk02;
		updatedSet->unk03 = newSettings.unk03;
		updatedSet->unk04 = newSettings.unk04;
	}
	if (newSettings.lowFlag & ROOM_LOW_PASSWORD) {
		updatedSet->password = newSettings.unk05;
		m_pSettings->password = newSettings.unk05;
	}
	if (newSettings.lowFlag & ROOM_LOW_LEVELLIMIT) {
		m_pSettings->unk06 = newSettings.unk06;
		updatedSet->unk06 = newSettings.unk06;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK7) {
		m_pSettings->unk07 = newSettings.unk07;
		updatedSet->unk07 = newSettings.unk07;
	}
	if (newSettings.lowFlag & ROOM_LOW_GAMEMODE) {
		m_pSettings->gameMode = newSettings.unk08;
		updatedSet->gameMode = newSettings.unk08;
	}
	if (newSettings.lowFlag & ROOM_LOW_MAPID) {
		m_pSettings->mapId = newSettings.unk09;
		updatedSet->mapId = newSettings.unk09;
	}
	if (newSettings.lowFlag & ROOM_LOW_MAXPLAYERS) {
		m_pSettings->maxPlayers = newSettings.unk10;
		updatedSet->maxPlayers = newSettings.unk10;
	}
	if (newSettings.lowFlag & ROOM_LOW_WINLIMIT) {
		m_pSettings->winLimit = newSettings.unk11;
		updatedSet->winLimit = newSettings.unk11;
	}
	if (newSettings.lowFlag & ROOM_LOW_NEEDEDKILLS) {
		m_pSettings->neededKills = newSettings.unk12;
		updatedSet->neededKills = newSettings.unk12;
	}
	if (newSettings.lowFlag & ROOM_LOW_GAMETIME) {
		m_pSettings->gameTime = newSettings.unk13;
		updatedSet->gameTime = newSettings.unk13;
	}
	if (newSettings.lowFlag & ROOM_LOW_ROUNDTIME) {
		m_pSettings->roundTime = newSettings.unk14;
		updatedSet->roundTime = newSettings.unk14;
	}
	if (newSettings.lowFlag & ROOM_LOW_ARMSRESTRICTION) {
		m_pSettings->armsRestriction = newSettings.unk15;
		updatedSet->armsRestriction = newSettings.unk15;
	}
	if (newSettings.lowFlag & ROOM_LOW_HOSTAGEKILLLIMIT) {
		m_pSettings->unk16 = newSettings.unk16;
		updatedSet->unk16 = newSettings.unk16;
	}
	if (newSettings.lowFlag & ROOM_LOW_FREEZETIME) {
		m_pSettings->freezeTime = newSettings.unk17;
		updatedSet->freezeTime = newSettings.unk17;
	}
	if (newSettings.lowFlag & ROOM_LOW_BUYTIME) {
		m_pSettings->buyTime = newSettings.unk18;
		updatedSet->buyTime = newSettings.unk18;
	}
	if (newSettings.lowFlag & ROOM_LOW_DISPLAYGAMENAME) {
		m_pSettings->displayNickname = newSettings.unk19;
		updatedSet->displayNickname = newSettings.unk19;
	}
	if (newSettings.lowFlag & ROOM_LOW_TEAMBALANCE) {
		m_pSettings->teamBalance = newSettings.unk20;
		updatedSet->teamBalance = newSettings.unk20;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK21) {
		m_pSettings->unk21 = newSettings.unk21;
		updatedSet->unk21 = newSettings.unk21;
	}
	if (newSettings.lowFlag & ROOM_LOW_FRIENDLYFIRE) {
		m_pSettings->friendlyFire = newSettings.unk22;
		updatedSet->friendlyFire = newSettings.unk22;
	}
	if (newSettings.lowFlag & ROOM_LOW_FLASHLIGHT) {
		m_pSettings->flashlight = newSettings.unk23;
		updatedSet->flashlight = newSettings.unk23;
	}
	if (newSettings.lowFlag & ROOM_LOW_FOOTSTEPS) {
		m_pSettings->footsteps = newSettings.unk24;
		updatedSet->footsteps = newSettings.unk24;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK25) {
		m_pSettings->unk25 = newSettings.unk25;
		updatedSet->unk25 = newSettings.unk25;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK26) {
		m_pSettings->unk26 = newSettings.unk26;
		updatedSet->unk26 = newSettings.unk26;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK27) {
		m_pSettings->unk27 = newSettings.unk27;
		updatedSet->unk27 = newSettings.unk27;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK28) {
		m_pSettings->unk28 = newSettings.unk28;
		updatedSet->unk28 = newSettings.unk28;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK29) {
		m_pSettings->unk29 = newSettings.unk29;
		updatedSet->unk29 = newSettings.unk29;
	}
	if (newSettings.lowFlag & ROOM_LOW_VIEWFLAG) {
		m_pSettings->unk30 = newSettings.unk30;
		updatedSet->unk30 = newSettings.unk30;
	}
	if (newSettings.lowFlag & ROOM_LOW_VOICECHAT) {
		m_pSettings->voiceChat = newSettings.unk31;
		updatedSet->voiceChat = newSettings.unk31;
	}
	if (newSettings.lowFlag & ROOM_LOW_STATUS) {
		m_pSettings->status = newSettings.unk32;
		updatedSet->status = newSettings.unk32;
	}
	if (newSettings.lowFlag & ROOM_LOW_UNK33) {
		m_pSettings->unk33 = newSettings.unk33;
		updatedSet->unk33 = newSettings.unk33;
	}

	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK) {
		m_pSettings->unk34 = newSettings.unk34;
		m_pSettings->unk35 = newSettings.unk35;
		m_pSettings->unk36 = newSettings.unk36;
		m_pSettings->unk37 = newSettings.unk37;

		updatedSet->unk34 = newSettings.unk34;
		updatedSet->unk35 = newSettings.unk35;
		updatedSet->unk36 = newSettings.unk36;
		updatedSet->unk37 = newSettings.unk37;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_C4TIMER) {
		m_pSettings->unk38 = newSettings.unk38;
		updatedSet->unk38 = newSettings.unk38;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_BOT) {
		m_pSettings->botDifficulty = newSettings.unk39;
		m_pSettings->friendlyBots = newSettings.unk40;
		m_pSettings->enemyBots = newSettings.unk41;
		m_pSettings->botBalance = newSettings.unk42;
		m_pSettings->botAdd = newSettings.unk43;

		updatedSet->botDifficulty = newSettings.unk39;
		updatedSet->friendlyBots = newSettings.unk40;
		updatedSet->enemyBots = newSettings.unk41;
		updatedSet->botBalance = newSettings.unk42;
		updatedSet->botAdd = newSettings.unk43;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_KDRULE) {
		m_pSettings->kdRule = newSettings.unk44;
		updatedSet->kdRule = newSettings.unk44;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_STARTINGCASH) {
		m_pSettings->startingCash = newSettings.unk45;
		updatedSet->startingCash = newSettings.unk45;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MOVINGSHOT) {
		m_pSettings->unk46 = newSettings.unk46;
		updatedSet->unk46 = newSettings.unk46;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK47) {
		m_pSettings->unk47 = newSettings.unk47;
		updatedSet->unk47 = newSettings.unk47;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_STATUSSYMBOL) {
		m_pSettings->unk48 = newSettings.unk48;
		updatedSet->unk48 = newSettings.unk48;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_RANDOMMAP) {
		m_pSettings->unk49 = newSettings.unk49;
		updatedSet->unk49 = newSettings.unk49;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MULTIPLEMAPS) {
		m_pSettings->unk50 = newSettings.unk50;
		m_pSettings->unk50_vec = newSettings.unk50_vec;
		updatedSet->unk50 = newSettings.unk50;
		updatedSet->unk50_vec = newSettings.unk50_vec;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK51) {
		m_pSettings->unk51 = newSettings.unk51;
		updatedSet->unk51 = newSettings.unk51;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_WPNENHANCERESTRICT) {
		m_pSettings->enhancement = newSettings.unk52;
		updatedSet->enhancement = newSettings.unk52;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_SD) {
		m_pSettings->unk53 = newSettings.unk53;
		updatedSet->unk53 = newSettings.unk53;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ZSDIFFICULTY) {
		m_pSettings->zsDifficulty = newSettings.unk54;
		m_pSettings->unk55 = newSettings.unk55;
		m_pSettings->unk56 = newSettings.unk56;

		updatedSet->zsDifficulty = newSettings.unk54;
		updatedSet->unk55 = newSettings.unk55;
		updatedSet->unk56 = newSettings.unk56;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_LEAGUERULE) {
		m_pSettings->league = newSettings.unk57;
		updatedSet->league = newSettings.unk57;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_MANNERLIMIT) {
		m_pSettings->mannerLimit = newSettings.unk58;
		updatedSet->mannerLimit = newSettings.unk58;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK59) {
		m_pSettings->unk59 = newSettings.unk59;
		updatedSet->unk59 = newSettings.unk59;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ZBLIMIT) {
		m_pSettings->unk60 = newSettings.unk60;
		updatedSet->unk60 = newSettings.unk60;

		m_pSettings->zbLimit = newSettings.zbLimit;
		updatedSet->zbLimit = newSettings.zbLimit;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK61) {
		m_pSettings->unk61 = newSettings.unk61;
		updatedSet->unk61 = newSettings.unk61;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK62) {
		m_pSettings->unk62 = newSettings.unk62;
		m_pSettings->unk62_vec = newSettings.unk62_vec;
		updatedSet->unk62 = newSettings.unk62;
		updatedSet->unk62_vec = newSettings.unk62_vec;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK63) {
		m_pSettings->unk63 = newSettings.unk63;
		updatedSet->unk63 = newSettings.unk63;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_TEAMSWITCH) {
		m_pSettings->teamSwitch = newSettings.teamSwitch;
		updatedSet->teamSwitch = newSettings.teamSwitch;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK65) {
		m_pSettings->unk65 = newSettings.unk65;
		updatedSet->unk65 = newSettings.unk65;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK66) {
		m_pSettings->unk66 = newSettings.unk66;
		updatedSet->unk66 = newSettings.unk66;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK67) {
		m_pSettings->unk67 = newSettings.unk67;
		updatedSet->unk67 = newSettings.unk67;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK68) {
		m_pSettings->unk68 = newSettings.unk68;
		updatedSet->unk68 = newSettings.unk68;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_ISZBCOMPETITIVE) {
		m_pSettings->isZbCompetitive = newSettings.isZbCompetitive;
		updatedSet->isZbCompetitive = newSettings.isZbCompetitive;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK70) {
		m_pSettings->unk70 = newSettings.unk70;
		updatedSet->unk70 = newSettings.unk70;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK71) {
		m_pSettings->unk71 = newSettings.unk71;
		updatedSet->unk71 = newSettings.unk71;
	}
	if (newSettings.lowMidFlag & ROOM_LOWMID_UNK72) {
		m_pSettings->unk72 = newSettings.unk72;
		updatedSet->unk72 = newSettings.unk72;
	}

	if (newSettings.highMidFlag & ROOM_HIGHMID_UNK73) {
		m_pSettings->unk73 = newSettings.unk73;
		updatedSet->unk73 = newSettings.unk73;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_UNK74) {
		m_pSettings->unk74 = newSettings.unk74;
		m_pSettings->unk74_vec = newSettings.unk74_vec;
		updatedSet->unk74 = newSettings.unk74;
		m_pSettings->unk74_vec = newSettings.unk74_vec;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_UNK75) {
		m_pSettings->unk75 = newSettings.unk75;
		updatedSet->unk75 = newSettings.unk75;
	}
	if (newSettings.highMidFlag & ROOM_HIGHMID_UNK76) {
		m_pSettings->unk76 = newSettings.unk76;
		updatedSet->unk76 = newSettings.unk76;
	}

	return updatedSet;
}

void CRoom::OnUserMessage(CReceivePacket* msg, CUser* user)
{
	string message = msg->ReadString();

	CUserCharacter character = user->GetCharacter(UFLAG_GAMENAME);
	string senderName = character.gameName;

	g_pConsole->Log("User '%s' write to room chat: '%s'\n", senderName.c_str(), message.c_str());

	if (!message.find("/changegm") && m_pHostUser == user)
	{
		istringstream iss(message);
		vector<string> results((istream_iterator<string>(iss)),
			istream_iterator<string>());

		if (results.size() == 2)
		{
			if (isNumber(results[1]))
			{
				int lowFlag = ROOM_LOW_GAMEMODE;
				int newGameMode = stoi(results[1]);

				CRoomSettings newRoomSettings;
				newRoomSettings.gameMode = newGameMode;
				m_pSettings->gameMode = newGameMode;

				for (auto u : m_Users) // send gamemode update to all users
				{
					g_pPacketManager->SendRoomUpdateSettings(u->GetExtendedSocket(), &newRoomSettings, lowFlag, 0);
				}
			}
			else
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changegm usage: /changegm <gameMode>");
			}
		}
		else
		{
			g_pPacketManager->SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changegm usage: /changegm <gameMode>");
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
				int lowFlag = ROOM_LOW_MAPID;
				int mapId = stoi(results[1]);

				CRoomSettings newRoomSettings;
				newRoomSettings.mapId = mapId;
				m_pSettings->mapId = mapId;

				for (auto u : m_Users)
				{
					g_pPacketManager->SendRoomUpdateSettings(u->GetExtendedSocket(), &newRoomSettings, lowFlag, 0);
				}
			}
			else
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changemap usage: /changemap <mapId>");
			}
		}
		else
		{
			g_pPacketManager->SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changemap usage: /changemap <mapId>");
		}

		return;
	}
	else if (!message.find("/changebotscount") && m_pHostUser == user)
	{
		int newEnemyBotsCount, newFriendlyBotsCount;
		istringstream iss(message);
		vector<string> results((istream_iterator<string>(iss)),
			istream_iterator<string>());

		if (results.size() == 3)
		{
			if (isNumber(results[1]) && isNumber(results[2]))
			{
				int lowMidFlag = ROOM_LOWMID_BOT;

				newEnemyBotsCount = stoi(results[1]);
				newFriendlyBotsCount = stoi(results[2]);

				CRoomSettings newRoomSettings;
				newRoomSettings.enemyBots = newEnemyBotsCount;
				newRoomSettings.friendlyBots = newFriendlyBotsCount;
				newRoomSettings.botAdd = 1;
				m_pSettings->enemyBots = newEnemyBotsCount;
				m_pSettings->friendlyBots = newFriendlyBotsCount;
				m_pSettings->botAdd = 1;

				for (auto u : m_Users)
				{
					g_pPacketManager->SendRoomUpdateSettings(u->GetExtendedSocket(), &newRoomSettings, 0, lowMidFlag);
				}
			}
		}
		else
		{
			g_pPacketManager->SendUMsgNoticeMessageInChat(user->GetExtendedSocket(), "/changebotscount usage: /changebotscount <enemy bots> <friendly bots>");
		}

		return;
	}

	for (auto user : m_Users)
	{
		g_pPacketManager->SendUMsgRoomMessage(user->GetExtendedSocket(), senderName, message);
	}
}

void CRoom::OnUserTeamMessage(CReceivePacket* msg, CUser* user)
{
	string message = msg->ReadString();
	int userTeam = GetUserTeam(user);

	CUserCharacter character = user->GetCharacter(UFLAG_GAMENAME);

	for (auto u : m_Users)
	{
		if (userTeam == GetUserTeam(u))
		{
			g_pPacketManager->SendUMsgRoomTeamMessage(u->GetExtendedSocket(), character.gameName, message);
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

		if (GetUserReadyStatus(u) == RoomReadyStatus::READY_STATUS_YES)
		{
			UserGameJoin(u);
		}
	}
}

void CRoom::KickUser(CUser* user)
{
	m_KickedUsers.push_back(user->GetID());

	for (auto u : m_Users)
	{
		g_pPacketManager->SendRoomKick(u->GetExtendedSocket(), user->GetID());
	}
}

void CRoom::VoteKick(CUser* user, bool kick)
{
	g_pConsole->Warn("CRoom::VoteKick: not implemented!\n");
}

void CRoom::SendRoomSettings(CUser* user)
{
	g_pPacketManager->SendRoomUpdateSettings(user->GetExtendedSocket(), m_pSettings);
}

void CRoom::SendUpdateRoomSettings(CUser* user, CRoomSettings* settings, int lowFlag, int lowMidFlag, int highMidFlag, int highFlag)
{
	g_pPacketManager->SendRoomUpdateSettings(user->GetExtendedSocket(), settings, lowFlag, lowMidFlag, highMidFlag, highFlag);
}

void CRoom::SendRoomUsersReadyStatus(CUser* user)
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

void CRoom::SendReadyStatusToAll(CUser* user)
{
	for (auto u : m_Users)
	{
		SendUserReadyStatus(u, user);
	}
}

void CRoom::SendNewUser(CUser* user, CUser* newUser)
{
	g_pPacketManager->SendRoomPlayerJoin(user->GetExtendedSocket(), newUser, RoomTeamNum::CounterTerrorist);
}

void CRoom::SendUserReadyStatus(CUser* user, CUser* player)
{
	if (player->GetRoomData() == NULL)
	{
		g_pConsole->Error("CRoom::SendUserReadyStatus: GetRoomData() == NULL, users count: %d\n", m_Users.size());
		return;
	}
	g_pPacketManager->SendRoomSetPlayerReady(user->GetExtendedSocket(), player, player->GetRoomData()->m_Ready);
}

void CRoom::SendConnectHost(CUser* user, CUser* host)
{
	//g_pPacketManager->SendUDPHostData(user->GetExtendedSocket(), true, host->GetData()->userId, host->GetNetworkConfig().m_szExternalIpAddress, host->GetNetworkConfig().m_nExternalServerPort);
	if (g_pServerConfig->room.connectingMethod)
	{
		//g_pPacketManager->SendHostJoin(user->GetExtendedSocket(), host->GetData()->userId);
		g_pPacketManager->SendHostServerJoin(user->GetExtendedSocket(), ip_string_to_int(host->GetNetworkConfig().m_szExternalIpAddress), false, host->GetNetworkConfig().m_nExternalServerPort, user->GetID());
	}
	else
	{
		if (m_pServer)
			g_pPacketManager->SendHostServerJoin(user->GetExtendedSocket(), m_pServer->GetIP(), true, m_pServer->GetPort(), user->GetID());
		else
			g_pPacketManager->SendHostServerJoin(user->GetExtendedSocket(), ip_string_to_int(host->GetNetworkConfig().m_szExternalIpAddress), false, host->GetNetworkConfig().m_nExternalServerPort, user->GetID());
	}
}

void CRoom::SendGuestData(CUser* host, CUser* guest)
{
	//CPacket_UDP hostData(host->m_pSocket);
	//hostData.Send(hostData.Build(0, guest->m_pData->userId, guest->externalIpAddress, 27015));
}

void CRoom::SendStartMatch(CUser* host)
{
	if (g_pServerConfig->room.connectingMethod)
	{
		g_pPacketManager->SendHostGameStart(host->GetExtendedSocket(), host->GetID());
	}
	else
	{
		m_pServer = g_pDedicatedServerManager->GetAvailableServerFromPools(this);

		if (m_pServer)
		{
			g_pPacketManager->SendRoomCreateAndJoin(m_pServer->GetSocket(), this);
			g_pPacketManager->SendRoomUpdateSettings(m_pServer->GetSocket(), m_pSettings, 0x40 | 0x80);
			g_pPacketManager->SendHostGameStart(m_pServer->GetSocket(), m_pServer->GetPort());

			g_pPacketManager->SendHostGameStart(host->GetExtendedSocket(), m_pServer->GetPort());
			g_pPacketManager->SendHostServerJoin(host->GetExtendedSocket(), m_pServer->GetIP(), true, m_pServer->GetPort(), host->GetID());
		}
		else
		{
			g_pPacketManager->SendHostGameStart(host->GetExtendedSocket(), host->GetID());
		}
	}

}

void CRoom::SendCloseResultWindow(CUser* user)
{
	g_pPacketManager->SendHostLeaveResultWindow(user->GetExtendedSocket());
}

void CRoom::SendTeamChange(CUser* user, CUser* player, RoomTeamNum newTeamNum)
{
	g_pPacketManager->SendRoomSetUserTeam(user->GetExtendedSocket(), player, newTeamNum);
}

void CRoom::SendGameEnd(CUser* user)
{
	g_pPacketManager->SendHostStop(user->GetExtendedSocket());
	g_pPacketManager->SendRoomGameResult(user->GetExtendedSocket(), this, m_pGameMatch);

	CGameMatchUserStat* stat = m_pGameMatch->GetGameUserStat(user);
	if (stat)
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), va("m_nKills: %d\nm_nDeaths: %d\nm_nScore: %d", stat->m_nKills, stat->m_nDeaths, stat->m_nScore));
	}
}

void CRoom::SendUserMessage(string senderName, string msg, CUser* user)
{
	g_pPacketManager->SendUMsgRoomMessage(user->GetExtendedSocket(), senderName, msg);
}

void CRoom::SendRoomStatus(CUser* user)
{
	int lowFlag = ROOM_LOW_STATUS;
	int lowMidFlag = ROOM_LOWMID_STATUSSYMBOL;

	CRoomSettings newSettings = CRoomSettings();
	newSettings.status = m_pSettings->status;
	newSettings.unk47 = m_pSettings->unk47;

	g_pPacketManager->SendRoomUpdateSettings(user->GetExtendedSocket(), &newSettings, lowFlag, lowMidFlag);
}

void CRoom::SendPlayerLeaveIngame(CUser* user)
{
	g_pPacketManager->SendRoomPlayerLeaveIngame(user->GetExtendedSocket());
}

void CRoom::OnUserRemoved(CUser* user)
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

void CRoom::SendRemovedUser(CUser* deletedUser)
{
	for (auto u : m_Users)
	{
		g_pPacketManager->SendRoomPlayerLeave(u->GetExtendedSocket(), deletedUser->GetID());
	}
}

void CRoom::UpdateHost(CUser* newHost)
{
	m_pHostUser = newHost;
	for (auto u : m_Users)
	{
		g_pPacketManager->SendRoomSetHost(u->GetExtendedSocket(), newHost);
	}

	if (m_pGameMatch && m_pServer == NULL)
	{
		m_pGameMatch->OnHostChanged(newHost);
	}
}

bool CRoom::FindAndUpdateNewHost()
{
	if (m_Users.size() == 0)
	{
		return false;
	}

	UpdateHost(m_Users[0]);

	return true;
}

void CRoom::HostStartGame()
{
	// set random map for zb competitive
	if (m_pSettings->gameMode == 45 && m_pSettings->isZbCompetitive)
	{
		vector<int> zbCompetitiveMaps;
		vector<int> column = g_pMapListTable->GetColumn<int>(";map_name");
		for (auto mapID : column)
		{
			int isZbCompetitive = g_pMapListTable->GetCell<int>("zb_competitive", to_string(mapID));
			if (isZbCompetitive)
			{
				zbCompetitiveMaps.push_back(mapID);
			}
		}

		Randomer randomMap(zbCompetitiveMaps.size() - 1);
		m_pSettings->mapId = zbCompetitiveMaps[randomMap()];

		CRoomSettings newRoomSettings;
		newRoomSettings.mapId = m_pSettings->mapId;

		for (auto u : m_Users)
		{
			g_pPacketManager->SendRoomUpdateSettings(u->GetExtendedSocket(), &newRoomSettings, ROOM_LOW_MAPID);
		}
	}

	StopCountdown(); // TODO

	if (!m_pGameMatch)
	{
		m_pGameMatch = new CGameMatch(this, m_pSettings->gameMode, m_pSettings->mapId);
	}
	else
	{
		g_pConsole->Error("CRoom::HostStartGame: m_pGameMatch != NULL\n");
		return;
	}

	SendStartMatch(m_pHostUser);

	SendReadyStatusToAll();

	m_pHostUser->GetCurrentChannel()->SendUpdateRoomList(this);

	if (m_pServer)
	{
		char ip[INET_ADDRSTRLEN];
		int iIp = m_pServer->GetIP();
		inet_ntop(AF_INET, &iIp, ip, sizeof(ip));

		g_pConsole->Log("Host '%s' started room match (RID: %d, IP: %s, port: %d)\n", m_pHostUser->GetUsername().c_str(), m_nID, ip, m_pServer->GetPort());
	}
	else
		g_pConsole->Log("Host '%s' started room match (RID: %d, IP: %s, port: %d)\n", m_pHostUser->GetUsername().c_str(), m_nID, m_pHostUser->GetNetworkConfig().m_szExternalIpAddress.c_str(), m_pHostUser->GetNetworkConfig().m_nExternalServerPort);
}

void CRoom::UserGameJoin(CUser* user)
{
	SetUserIngame(user, true);
	SendGuestData(m_pHostUser, user);
	SendConnectHost(user, m_pHostUser);
	SendReadyStatusToAll(user);

	//m_pGameMatch->Connect(user);

	g_pConsole->Log("User '%d, %s' joining room match (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), m_nID);
}

void CRoom::EndGame()
{
	m_pGameMatch->OnGameMatchEnd();

	SetStatus(RoomStatus::STATUS_WAITING);
	ResetStatusIngameUsers();

	if (m_pServer)
		g_pPacketManager->SendHostStop(m_pServer->GetSocket());

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

	delete m_pGameMatch;
	m_pGameMatch = NULL;
}

int CRoom::GetID()
{
	return m_nID;
}

CUser* CRoom::GetHostUser()
{
	return m_pHostUser;
}

vector<CUser*> CRoom::GetUsers()
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