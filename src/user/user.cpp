#include "user.h"
#include "manager/userdatabase.h"
#include "manager/packetmanager.h"
#include "manager/questmanager.h"
#include "serverconfig.h"

using namespace std;

CUser::CUser(IExtendedSocket* sock, int userID, const std::string& userName)
{
	m_pCurrentRoom = NULL;
	m_pCurrentChannel = NULL;
	m_pLastChannelServer = NULL;
	m_pRoomData = NULL;

	m_pSocket = sock;
	m_nID = userID;
	m_UserName = userName;

	m_Status = UserStatus::STATUS_MENU;

	m_NetworkData.m_szExternalIpAddress = m_pSocket->GetIP();
	m_NetworkData.m_nExternalClientPort = 27015;
	m_NetworkData.m_nExternalServerPort = 27005;
	m_NetworkData.m_szLocalIpAddress = m_pSocket->GetIP();
	m_NetworkData.m_nLocalClientPort = 27015;
	m_NetworkData.m_nLocalServerPort = 27005;

	m_nUptime = 0;
}

CUser::~CUser()
{
	g_pUserDatabase->DropSession(m_nID);

	if (m_pCurrentRoom)
		m_pCurrentRoom->RemoveUser(this);

	if (m_pCurrentChannel)
		m_pCurrentChannel->UserLeft(this);

	if (m_pRoomData)
		delete m_pRoomData;
}

void CUser::SetCurrentChannel(CChannel* channel)
{
	m_pCurrentChannel = channel;
}

void CUser::SetLastChannelServer(CChannelServer* channelServer)
{
	m_pLastChannelServer = channelServer;
}

void CUser::SetStatus(UserStatus newStatus)
{
	m_Status = newStatus;
}

void CUser::SetCurrentRoom(IRoom* room)
{
	m_pCurrentRoom = room;
}

void CUser::SetRoomData(CRoomUser* roomUser)
{
	m_pRoomData = roomUser;
}

UserNetworkConfig_s CUser::GetNetworkConfig()
{
	return m_NetworkData;
}

IExtendedSocket* CUser::GetExtendedSocket()
{
	return m_pSocket;
}

CChannel* CUser::GetCurrentChannel()
{
	return m_pCurrentChannel;
}

CChannelServer* CUser::GetLastChannelServer()
{
	return m_pLastChannelServer;
}

IRoom* CUser::GetCurrentRoom()
{
	return m_pCurrentRoom;
}

CRoomUser* CUser::GetRoomData()
{
	return m_pRoomData;
}

UserStatus CUser::GetStatus()
{
	return m_Status;
}

int CUser::GetUptime()
{
	return m_nUptime;
}

int CUser::GetID()
{
	return m_nID;
}

string CUser::GetUsername()
{
	return m_UserName;
}

const char* CUser::GetLogName()
{
	return va("%d, %s", m_nID, m_UserName.c_str());
}

CUserData CUser::GetUser(int flag)
{
	CUserData data = {};
	data.flag = flag;
	g_pUserDatabase->GetUserData(m_nID, data);

	return data;
}

CUserCharacter CUser::GetCharacter(int flag)
{
	CUserCharacter character = {};
	character.flag = flag;

	if (g_pUserDatabase->GetCharacter(m_nID, character) <= 0)
		character.flag = 0;

	return character;
}

CUserCharacterExtended CUser::GetCharacterExtended(int flag)
{
	CUserCharacterExtended character = {};
	character.flag = flag;

	if (g_pUserDatabase->GetCharacterExtended(m_nID, character) <= 0)
		character.flag = 0;

	return character;
}

int CUser::UpdateHolepunch(int portId, int localPort, int externalPort)
{
	switch (portId)
	{
	case 0:
		m_NetworkData.m_nLocalClientPort = localPort;
		m_NetworkData.m_nExternalClientPort = externalPort;
		return 0;
	case 256:
		m_NetworkData.m_nLocalServerPort = localPort;
		m_NetworkData.m_nExternalServerPort = externalPort;
		return 1;
	default:
		return -1;
	};
}
void CUser::UpdateClientUserInfo(int flag, CUserCharacter character)
{
	if (m_pCurrentChannel)
	{
		if (flag & UFLAG_GAMENAME || flag & UFLAG_EXP || flag & UFLAG_LEVEL || flag & UFLAG_GAMENAME2 || flag & UFLAG_STAT || flag & UFLAG_LOCATION || flag & UFLAG_RANK || flag & UFLAG_ACHIEVEMENT || flag & UFLAG_TITLES || flag & UFLAG_CLAN || flag & UFLAG_TOURNAMENT || flag & UFLAG_NAMEPLATE)
		{
			m_pCurrentChannel->UpdateUserInfo(this, character);
		}
	}

	// TODO: update user info in room

	g_pPacketManager->SendUserUpdateInfo(m_pSocket, this, character);
}

void CUser::UpdateGameName(const string& gameName)
{
	CUserCharacter character;
	character.gameName = gameName;
	character.flag = UFLAG_GAMENAME;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_GAMENAME | UFLAG_GAMENAME2, character);
}

int CUser::UpdatePoints(int64_t points)
{
	CUserCharacter character = GetCharacter(UFLAG_POINTS);
	character.points += points;

	if (g_pUserDatabase->UpdateCharacter(m_nID, character) <= 0)
	{
		return 0;
	}

	UpdateClientUserInfo(UFLAG_POINTS, character);

	return 1;
}

void CUser::UpdateCash(int64_t cash)
{
	CUserCharacter character = GetCharacter(UFLAG_CASH);
	character.cash += cash;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_CASH | UFLAG_CASH2, character);
}

void CUser::UpdateHonorPoints(int honorPoints)
{
	CUserCharacter character = GetCharacter(UFLAG_ACHIEVEMENT);
	character.honorPoints += honorPoints;
	character.achievementFlag = 1;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_ACHIEVEMENT, character);
}

void CUser::UpdatePrefix(int prefixID)
{
	CUserCharacter character = GetCharacter(UFLAG_ACHIEVEMENT);
	character.prefixId = prefixID;
	character.achievementFlag = 2;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_ACHIEVEMENT, character);
}

void CUser::UpdateStat(int battles, int win, int kills, int deaths)
{
	CUserCharacter character = GetCharacter(UFLAG_STAT);
	character.battles += battles;
	character.win += win;
	character.kills += kills;
	character.deaths += deaths;
	character.statFlag |= 0x1 | 0x2 | 0x4 | 0x8;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_STAT, character);
}

void CUser::UpdateLocation(int nation, int city, int town)
{
	CUserCharacter character = {};
	character.flag = UFLAG_LOCATION;
	character.nation = nation;
	character.city = city;
	character.town = town;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_LOCATION, character);
}

void CUser::UpdateRank(int leagueID)
{
	CUserCharacter character = GetCharacter(UFLAG_RANK);
	character.leagueID = leagueID;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_RANK, character);
}

void CUser::UpdateLevel(int level)
{
}

void CUser::UpdateExp(int64_t exp)
{
	CUserCharacter character = GetCharacter(UFLAG_EXP | UFLAG_LEVEL);
	if (character.exp >= 30069814)
		return;

	character.exp += exp;

	if (character.exp > 30069814)
		character.exp = 30069814;

	int newLvl = CheckForLvlUp(character.exp);

	if (newLvl > character.level)
	{
		g_pQuestManager->OnLevelUpEvent(this, character.level, newLvl);
		character.level = newLvl;
	}

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	// update userinfo on client side
	UpdateClientUserInfo(UFLAG_EXP | UFLAG_LEVEL, character);
}

int CUser::UpdatePasswordBoxes(int passwordBoxes)
{
	CUserCharacter character = GetCharacter(UFLAG_PASSWORDBOXES);
	if (character.flag == 0)
		return 0;

	character.passwordBoxes += passwordBoxes;

	if (g_pUserDatabase->UpdateCharacter(m_nID, character) <= 0)
		return 0;

	UpdateClientUserInfo(UFLAG_PASSWORDBOXES, character);

	return 1;
}

void CUser::UpdateTitles(int slot, int titleID)
{
	CUserCharacter character = GetCharacter(UFLAG_TITLES);
	character.titles[slot] = titleID;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_TITLES, character);
}

void CUser::UpdateAchievementList(int titleID)
{
	CUserCharacter character = GetCharacter(UFLAG_ACHIEVEMENTLIST);
	character.achievementList.push_back(titleID);

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_ACHIEVEMENTLIST, character);
}

void CUser::UpdateClan(int clanID)
{
	CUserCharacter character = {};
	character.flag = UFLAG_CLAN;
	character.clanID = clanID;

	g_pUserDatabase->UpdateCharacter(m_nID, character);
	g_pUserDatabase->GetCharacter(m_nID, character); // get clan mark, name

	UpdateClientUserInfo(UFLAG_CLAN, character);
}

void CUser::UpdateTournament(int tournament)
{
	CUserCharacter character = {};
	character.flag = UFLAG_TOURNAMENT;
	character.tournament = tournament;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_TOURNAMENT, character);
}

int CUser::UpdateBanList(const string& gameName, bool remove)
{
	int result = g_pUserDatabase->UpdateBanList(m_nID, gameName, remove);

	// TODO: update channel user info

	return result;
}

void CUser::UpdateBanSettings(int settings)
{
	CUserCharacterExtended characterExt = {};
	characterExt.flag = EXT_UFLAG_BANSETTINGS;
	characterExt.banSettings = settings;

	g_pUserDatabase->UpdateCharacterExtended(m_nID, characterExt);
}

void CUser::UpdateNameplate(int nameplateID)
{
	CUserCharacter character = {};
	character.flag = UFLAG_NAMEPLATE;
	character.nameplateID = nameplateID;

	g_pUserDatabase->UpdateCharacter(m_nID, character);

	UpdateClientUserInfo(UFLAG_NAMEPLATE, character);
}

void CUser::UpdateZbRespawnEffect(int zbRespawnEffect)
{
	CUserCharacterExtended characterExt = {};
	characterExt.flag = EXT_UFLAG_ZBRESPAWNEFFECT;
	characterExt.zbRespawnEffect = zbRespawnEffect;

	g_pUserDatabase->UpdateCharacterExtended(m_nID, characterExt);
}

void CUser::UpdateKillerMarkEffect(int killerMarkEffect)
{
	CUserCharacterExtended characterExt = {};
	characterExt.flag = EXT_UFLAG_KILLERMARKEFFECT;
	characterExt.killerMarkEffect = killerMarkEffect;

	g_pUserDatabase->UpdateCharacterExtended(m_nID, characterExt);
}

int CUser::CheckForLvlUp(int64_t exp)
{
	int newLvl = 1;
	const vector<int64_t> expToLvlUp = // this array needs to be load from level.csv // According to Latest Feb6 Update (Level 249)
	{
		100, 250, 572, 1179, 2000, 3000, 4200, 6073, 8692, 11491, 14488, 17692, 21112, 24775, 28762,
		33109, 37852, 43018, 48652, 54790, 61477, 68767, 76714, 85381, 94822, 105118, 116341, 128572, 141901, 156562,
		172690, 190438, 209959, 231433, 255049, 281032, 309607, 341044, 375622, 413656, 455497, 501523, 552148, 607840,
		669094, 736477, 810601, 892132, 981817, 1080475, 1188997, 1308373, 1439683, 1584124, 1743010, 1917790, 2110039,
		2321521, 2554144, 2812363, 3101560, 3428359, 3800905, 4229341, 4726321, 5307784, 5993917, 7297567, 8861947,
		10754845, 13073832, 13738611, 14612239, 15573229, 16620708, 17877683, 20391634, 22930724, 25495205, 28085330,
		30701357, 33343545, 36012154, 38707449, 41429697, 44179168, 46956133, 49760868, 52593650, 55454761, 58344482,
		61263100, 64210905, 67188188, 70195243, 73232369, 76299867, 79398039, 82527193, 85687639, 88879689, 92103659,
		95359870, 98648642, 101970302,
		105325179, 108713604, 112135914, 115592447, 119083545, 122609554, 126170823, 129767705, 133400556, 137069735,
		140775606, 144518536, 148298895, 152117057, 155973401, 159868309, 163802166, 167775361, 171788288, 175841345,
		179934932, 184069455, 188245323, 192462950, 196722753, 201025155, 205370580, 209759459, 214192227, 218669323,
		223191190, 227758276, 232371032, 237029916, 241735389, 246487917, 251287969, 256136023, 261032557, 265978056,
		270973010, 276017914, 281113267, 286259573, 291457342, 296707089, 302009334, 307364601, 312773421, 318236329,
		323753866, 329326578, 334955017, 340639741, 346381312, 352180299, 358037276, 363952822, 369927524, 375961973,
		382056766, 388212507, 394429806, 400709278, 407051544, 413457233, 419926979, 426461423, 433061211, 439726997,
		446459440, 453259208, 460126974, 467063417, 474069225, 481145091, 488291716, 495509807, 502800078, 510163253,
		517600059, 525111233, 532697519, 540359668, 548098438, 555914596, 563808916, 571782179, 579835174, 587968700,
		596183560, 604480570, 612860549, 621324328, 629872745, 638506646, 647226886, 656034329, 664929846, 673914318,
		682988635, 692153695, 701410405, 710759683, 720202454, 729739652, 739372222, 749101118, 758927303, 768851749,
		778875440, 788999369, 799224536, 809551955, 819982648, 830517648, 841157998, 851904752, 862758973, 873721737,
		884794128, 895977243, 907272189, 918680084, 930202059, 941839253, 953592819, 965463921, 977453734, 989563445,
		1001794253, 1014147369, 1026624017, 1039225430, 1051952858, 1064807561, 1077790810, 1090903892, 1104148104,
		1117524759, 1131035180, 1144680706, 1158462686, 1172382487
	};

	for (size_t i = 0; i < expToLvlUp.size(); i++)
	{
		if (exp >= expToLvlUp[i])
		{
			newLvl++;
		}
	}

	return newLvl;
}

void CUser::OnTick()
{
	m_nUptime++;

	if (m_nUptime % 3600 == 0)
	{
		int hours = m_nUptime / 3600;
		g_pPacketManager->SendUMsgSystemReply(m_pSocket, UMsgPacketType::SystemReply_Red, hours > 2 ? (char*)OBFUSCATE("ETC_PLAYTIME_LONG") : (char*)OBFUSCATE("ETC_PLAYTIME"), vector<string> {to_string(hours)});
	}

	if (m_nUptime % 1800 == 0)
	{
		g_pPacketManager->SendUMsgNoticeMessageInChat(m_pSocket, OBFUSCATE("The server team would appreciate your financial support: https://discord.gg/EvUAY6D"));
	}
}

bool CUser::IsCharacterExists()
{
	CUserCharacter character = {};
	character.flag = UFLAG_GAMENAME;

	int result = g_pUserDatabase->GetCharacter(m_nID, character);
	if (result <= 0)
		return false;

	return true;
}

bool CUser::CreateCharacter(const string& gameName)
{
	int result = g_pUserDatabase->CreateCharacter(m_nID, gameName);
	if (result <= 0)
		return false;

	g_pQuestManager->OnLevelUpEvent(this, 1, 1);

	return true;
}