#pragma once

#include "user/userloadout.h"

#include <string>
#include <vector>

#define CSO_24_HOURS_IN_MINUTES 1440
#define CSO_24_HOURS_IN_SECONDS 86400

//#define LOG_PACKET Console().SetLastPacket(__FUNCTION__);
#define LOG_PACKET

enum PacketId
{
	Version = 0,
	Reply = 1,
	Transfer = 2,
	CreateCharacter = 2,
	Login = 3,
	TransferLogin = 5,
	ServerList = 5,
	Character = 6,
	Crypt = 7,
	RequestTransfer = 7,
	RequestServerList = 10,
	RecvCrypt = 12,
	MileageBingo = 15,
	Statistic = 16,
	SessionID = 17,
	Mobile = 18,
	// Missing 46 packets
	Room = 65,
	ClientCheck = 66,
	UMsg = 67,
	Host = 68,
	UpdateInfo = 69,
	Udp = 70,
	Clan = 71,
	Shop = 72,
	Rank = 73,
	Ban = 74,
	Option = 76,
	Favorite = 77,
	Item = 78,
	GameGuard = 79,
	SearchRoom = 80,
	HostServer = 81,
	HackShield = 82,
	Report = 83,
	Title = 84,
	Buff = 85,
	QuickStart = 86,
	UserSurvey = 87,
	Quest = 88,
	MiniGame = 89,
	Hack = 90,
	Metadata = 91,
	SNS = 92,
	Messenger = 93,
	Comrade = 94,
	WeeklyClanLeague = 95,
	Gift_Item = 96,
	_2nd_Password = 97,
	Request2nd_Password = 98,
	// Packet_GameMatch & Packet_Match
	GameMatch = 99,
	ZBEnhance = 100,
	CleanSystem = 101,
	RibbonSystem = 102,
	// Packet_VoxelGameSave & Packet_VoxelOutUI
	Voxel = 103,
	AuctionEvent = 104,
	Analysis = 105,
	LiveStream = 106,
	CoDisassemble = 107,
	MileageShop = 108,
	Help = 109,
	PopularInfo = 110,
	Unk111 = 111,
	Kick = 112,
	HonorShop = 113,
	EpicPieceShop = 114,
	Addon = 115,
	QuestBadgeShop = 116,
	ReverseAuctionSystem = 117,
	SeasonSystem = 118,
	Unk119 = 119,
	GuideQuest = 120,
	Unk121 = 121,
	Unk122 = 122,
	UserStartStep = 123,
	// Missing 26 packets
	UserStart = 150,
	GameMatchRoomList = 151,
	DefaultItems = 152,
	Lobby = 153,
	Inventory = 154,
	ClanStock = 155,
	CafeItems = 156,
	UserUpdateInfo = 157,
	FabItems = 158,
	Event = 159,
	CostumeInven = 160,
	ZombieScenarioMaps = 161,
	RotationWeaponInven = 162,
	SaleCoupon = 163,
	Alarm = 164,
	MonthlyWeapon = 165,
	LifeWeaponInven = 166,
	VipSystem = 167,
	FreePassWeaponInven = 168,
	ServerLog = 169,
	Unk170 = 170,
	NxLog = 171,
	Dictionary = 172,
	TestPacket = 173,
	ZBSGoldenZB = 174,
	Friend = 175,
	EventItemInven = 176,
	Expedition = 177,
	ScenarioTX = 178,
	UserRestrict = 179,
	League = 180,
	ClassInven = 181,
	PartsInven = 182,
	ItemInven = 183,
	SwitchConfig = 184,
	RewardHistory = 186,
	Steam = 192,
	Captcha = 193,
	// missing 62 packets
};

enum HostPacketType
{
	GameStart = 0,
	HostJoin = 1,
	SaveData = 1,
	UpdateUserStatus = 2,
	HostRestart = 2,
	HostStop = 3,
	OnKillEvent = 3,
	LeaveResultWindow = 4,
	HostServerJoin = 5,
	OnGameEnd = 5,
	AdBalloon = 6,
	OnUpdateKillCounter = 7,
	OnUpdateDeathCounter = 8,
	OnUpdateWinCounter = 9,
	OnUpdateScore = 10,
	OnGameEvent = 11,
	SetZBAddons = 11,
	OnUserWeapon = 13, // Or OnUpdateWeapon? not very sure what to name :D
	OnUpdateClass = 14,
	OnUserSpawn = 16, // Spawn Location
	OnChangeMap = 20,
	OnRoundStart = 24,
	UseScenItem = 100,
	SetInventory = 101,
	FlyerFlock = 102,
};

enum EMetadataPacketType
{
	kPacket_Metadata_MapList = 0,
	kPacket_Metadata_ClientTable = 1,
	kPacket_Metadata_ModeList = 2,
	kPacket_Metadata_Unk3 = 3,
	kPacket_Metadata_ItemBox = 4,
	KPacket_Metadata_PaintItemList = 6,
	kPacket_Metadata_Unk8 = 8,
	kPacket_Metadata_MatchOption = 9,
	kPacket_Metadata_Unk15 = 15,
	kPacket_Metadata_RandomWeaponList = 16,
	kPacket_Metadata_WeaponParts = 17,
	kPacket_Metadata_Unk20 = 20,
	kPacket_Metadata_Encyclopedia = 23,
	kPacket_Metadata_GameModeList = 24,
	kPacket_Metadata_ProgressUnlock = 27,
	kPacket_Metadata_ReinforceMaxLvl = 28,
	kPacket_Metadata_ReinforceMaxEXP = 29,
	kPacket_Metadata_ReinforceItemsExp = 30,
	kPacket_Metadata_Unk31 = 31,
	kPacket_Metadata_Item = 32,
	kPacket_Metadata_CodisData = 35,
	kPacket_Metadata_HonorMoneyShop = 36,
	kPacket_Metadata_ItemExpireTime = 37,
	kPacket_Metadata_ScenarioTX_Common = 38,
	kPacket_Metadata_ScenarioTX_Dedi = 39,
	kPacket_Metadata_ShopItemList_Dedi = 40,
	kPacket_Metadata_WeaponProp = 42,
	kPacket_Metadata_Unk43 = 43,
	kPacket_Metadata_PPSystem = 45,
	kPacket_Metadata_ZBCompetitive = 48,
	kPacket_Metadata_Unk49 = 49,
	kPacket_Metadata_ModeEvent = 50,
	kPacket_Metadata_Hash = 255
};

enum EServerConfig_MetadataFlag : uint64_t
{
	kMetadataFlag_MapList = 1LL << 0,
	kMetadataFlag_ClientTable = 1LL << 1,
	kMetadataFlag_ModeList = 1LL << 2,
	kMetadataFlag_Unk3 = 1LL << 3,
	kMetadataFlag_ItemBox = 1LL << 4,
	kMetadataFlag_WeaponPaint = 1LL << 5,
	kMetadataFlag_Unk8 = 1LL << 6,
	kMetadataFlag_MatchOption = 1LL << 7,
	kMetadataFlag_Unk15 = 1LL << 8,
	kMetadataFlag_WeaponParts = 1LL << 9,
	kMetadataFlag_Unk20 = 1LL << 10,
	kMetadataFlag_Encyclopedia = 1LL << 11,
	kMetadataFlag_GameModeList = 1LL << 12,
	kMetadataFlag_ProgressUnlock = 1LL << 13,
	kMetadataFlag_ReinforceMaxLvl = 1LL << 14,
	kMetadataFlag_ReinforceMaxEXP = 1LL << 15,
	kMetadataFlag_ReinforceItemsExp = 1LL << 16,
	kMetadataFlag_Unk31 = 1LL << 17,
	kMetadataFlag_HonorMoneyShop = 1LL << 18,
	kMetadataFlag_ItemExpireTime = 1LL << 19,
	kMetadataFlag_ScenarioTX_Common = 1LL << 20,
	kMetadataFlag_ScenarioTX_Dedi = 1LL << 21,
	kMetadataFlag_ShopItemList_Dedi = 1LL << 22,
	kMetadataFlag_ZBCompetitive = 1LL << 23,
	kMetadataFlag_Unk43 = 1LL << 24,
	kMetadataFlag_Unk49 = 1LL << 25,
	kMetadataFlag_WeaponProp = 1LL << 26,
	kMetadataFlag_Hash = 1LL << 27,
	kMetadataFlag_PPSystem = 1LL << 28,
	kMetadataFlag_Item = 1LL << 29,
	kMetadataFlag_CodisData = 1LL << 30,
	kMetadataFlag_RandomWeaponList = 1LL << 31,
	kMetadataFlag_ModeEvent = 1LL << 32,
};

enum ItemPacketType
{
	UseItem = 0,
	OpenDecoder = 4,
	WeaponAssem = 5,
	MaterialAssem = 6,
	Disassemble = 7,
	DailyRewards = 8,
	ReqTattooEquip = 9,
	GachaponNotice = 10,
	GiftboxNotice = 11,
	FabenduNotice = 12,
	TattooEquip = 13,
	EnhanceReq = 14,
	ReqCostumeEquip = 15,
	ItemExtend = 16,
	WeaponPaintReq = 17,
	Part = 21,
	SwitchInUse = 23,
	WeaponPaintSwitchReq = 24,
	LockItem = 26,
};

enum UMsgPacketType
{
	WhisperUserMessage = 0,
	LobbyUserMessage = 1,
	RoomUserMessage = 2,
	ClanUserMessage = 3,
	RoomTeamUserMessage = 4,
	PartyUserMessage = 5,
	ServerYellUserMessage = 6,
	ServerNoticeMessageMsgBox = 10,
	ServerNoticeMessageInChat = 11,
	SystemReply_Red = 20,
	SystemReply_Green = 21,
	SystemReply_MsgBox = 30,
	GMNoticeUserMessage = 40,
	GMNoticeUserMessage2 = 41,
	ServerDisconnectMessage = 50,
	ServerDisconnectMessage2 = 51,
	RewardNoticeMsgLocalized = 60,
	RewardNoticeMsg = 61,
	ExpiredItem = 62,
	RewardInGameNoticeMsgLocalized = 63,
	RewardInGameNoticeMsg = 64,
	Notice = 65,
	RewardNoticeSelect = 67,
	RewardInGameNoticeScenario = 69,
};

enum FavoritePacketType
{
	SetBuyMenu = 0,
	SetFastBuy = 1,
	SetLoadout = 2,
	SetBookmark = 6,
};

enum RoomListPacketType
{
	FullRoomList = 0,
	AddRoom = 1,
	RemoveRoom = 2,
	UpdateRoom = 3,
};

enum LobbyPacketType
{
	Join = 0,
	UserJoin = 1,
	UserLeft = 2,
};

struct RewardItem
{
	int selectID;
	int itemID;
	int count;
	int duration;
	int enhValue;
	int eventFlag;
};
struct RewardItemRandomDuration
{
	int chance;
	std::vector<int> durations;
};
struct RewardItemRandom
{
	int chance;
	std::vector<RewardItem> items;
};
struct RewardItemsRandom
{
	std::vector<RewardItemRandomDuration> durations;
	std::vector<RewardItemRandom> items;
};
struct Reward
{
	int rewardId;
	int lvlRestriction;
	bool select;
	std::string title;
	std::string description;
	std::vector<int> points;
	std::vector<int> exp;
	std::vector<int> honorPoints;
	std::vector<RewardItem> items;
	std::vector<RewardItemsRandom> randomItems;
};
struct RewardNotice
{
	bool status;
	int addItemStatus;
	int rewardId;
	int points;
	int exp;
	int honorPoints;
	std::vector<RewardItem> items;

	RewardNotice& operator += (const RewardNotice& other)
	{
		status = other.status;
		addItemStatus = other.addItemStatus;
		rewardId = other.rewardId;
		points += other.points;
		exp += other.exp;
		honorPoints += other.honorPoints;
		items.insert(items.end(), other.items.begin(), other.items.end());

		return *this;
	}
};

enum NoticeType
{
	NOTICE_HOT = 1,
	NOTICE_NOTICE = 2,
	NOTICE_EVENT = 3,
	NOTICE_ETC = 4
};
struct Notice_s
{
	int id;
	NoticeType type;
	std::string name;
	std::string description;
	int startDate;
	int endDate;
};

struct DefaultUser
{
	bool gameMaster;
	int level;
	uint64_t exp;
	uint64_t points;
	int honorPoints;
	int prefixID;
	int kills;
	int deaths;
	int battles;
	int win;
	int passwordBoxes;
	int mileagePoints;
	std::vector<int> defaultItems;
	std::vector<int> pseudoDefaultItems;
	CUserLoadout loadouts;
	std::vector<CUserBuyMenu> buyMenu;
};

struct GameMatchCoefficients_s
{
	int gameMode;
	int exp;
	int points;
};

struct BonusPercentage_s
{
	int itemID;
	int coef;
	int exp;
	int points;
};

struct ServerConfigGameMatch_s
{
	std::vector<GameMatchCoefficients_s> gameModeCoefficients;
	std::vector<BonusPercentage_s> bonusPercentageItems;
	std::vector<BonusPercentage_s> bonusPercentageClasses;
	std::vector<BonusPercentage_s> bonusPlayerCoop;
};

struct ServerConfigRoom_s
{
	int connectingMethod; // 0 - connecting to the dedicated server (host restart functionality is unavailable), 1 - connecting to the server hosted by user
	bool validateSettings;
	std::vector<std::string> roomDefaultTitles;
};

struct ServerConfigBingo
{
	std::vector<RewardItem> prizeItems;
};

struct ServerBanList
{
	std::vector<std::string> ip;
	std::vector<std::vector<unsigned char>> hwid;
};

enum ServerReply
{
	S_REPLY_YES = 0,
	S_REPLY_CREATEOK = 1,
	S_REPLY_PLAYING = 4,
	S_REPLY_CREATE_ID_TOO_SHORT = 18,
	S_REPLY_CREATE_ID_TOO_LONG = 20,
	S_REPLY_CREATE_ID_INVALID_CHAR = 21,
	S_REPLY_CREATE_ID_ALREADY_EXIST = 25,
	S_REPLY_EXCEED_MAX_CONNECTION = 26,
	S_REPLY_INVALID_CLIENT_VERSION = 37,
};

enum UpdateInfoPacketType
{
	RequestUpdateNickname = 2,
	RequestUpdateLocation = 4,
	RequestUpdateTutorial = 7,
};

// QUEST STUFF
enum QuestPacketType
{
	SpecialMissionRequest = 31,
	ReceiveReward = 42,
	SetFavouriteQuest = 51,
};
enum QuestType
{
	QUEST_DAILY = 0,
	QUEST_SPECIAL = 1,
	QUEST_HONOR = 2,
	QUEST_WEEKLY = 3
};
enum QuestTaskEventType
{
	EVENT_TIMEMATCH = 0,
	EVENT_MATCHWIN = 1,
	EVENT_KILL = 2,
	EVENT_LOGIN = 3,
	EVENT_LEVELUP = 5,
	EVENT_BOMBEXPLODE = 6,
	EVENT_BOMBDEFUSE = 7,
	EVENT_HOSTAGEESCAPE = 8,
	EVENT_VIPKILL = 9,
	EVENT_KILLMONSTER = 10,
	EVENT_KILLMOSQUITO = 11,
	EVENT_KILLKITE = 12,
};
struct QuestTaskEvent_Kill_s
{
	int victimKillType;
	int victimTeam;
	int continuous;
	int gun; // 0 - melee, 1 - primary, 2 - secondary, 3 - he
	int gunID;
	bool bot;
	bool human;
};
struct QuestTaskEvent_Time_s
{
	int time;
};
struct QuestTask_s
{
	int id;
	std::string name;
	int unitsToFinish;
	bool temp; // user task progress resets after leaving game match

	// condition
	int gameMode;
	int playerCount;
	int eventType;
	QuestTaskEvent_Kill_s killEvent;
	QuestTaskEvent_Time_s timeEvent;
};
struct QuestReward_s
{
	int id;
	int rewardID;
	std::string title;
	std::string description;
	int resID;
	int honor;
};
struct QuestEventReward
{
	int id;
	int rewardID;
	std::string userMsg;
};
struct GameMatch_KillEvent
{
	int killerUserID;
	int gunID;
	int killerTeam;
	int victimUserID;
	int victimTeam;
	int victimKillType;
	float killerPos[3];
	float victimPos[3];
};
// outdated
struct Quest_s
{
	int id;
	int type; // 0 - daily, 1 - special, 2 - honor,  3 - weekly
	std::string title;
	std::string description;
	int npcID;
	std::vector<QuestTask_s> tasks;
	std::vector<QuestReward_s> rewards;
};
struct UserQuestStat
{
	int continiousSpecialQuest;
	int dailyMissionsCompletedToday;
	int dailyMissionsCleared;
};
struct UserQuestTaskProgress_KillEvent_s
{
	int continuousKillsCounter; // temp variable
};
struct UserQuestTaskProgress_TimeEvent_s
{
	int timeCounter;
};
struct UserQuestTaskProgress
{
	int taskID;
	int unitsDone;
	int taskVar;
	bool finished;
	UserQuestTaskProgress_KillEvent_s killEventProgress;
	UserQuestTaskProgress_TimeEvent_s timeEventProgress;
};
struct UserQuestProgress
{
	int questID;
	int status;
	bool favourite;
	bool started; // user started task
	std::vector<UserQuestTaskProgress> tasks;
};
enum UserQuestStatus
{
	QUEST_IN_PROGRESS = 0,
	QUEST_DONE = 1,
	QUEST_DONE_AND_REWARD_RECEIVED = 2,
	QUEST_WAIT_FOR_OPEN = 3,
};

// ITEMMGR STUFF
enum ItemAddStatus
{
	ITEM_ADD_INVENTORY_FULL = -1,
	ITEM_ADD_UNKNOWN_ITEMID = -2,
	ITEM_ADD_SUCCESS = 1,
};
enum ItemUseStatus
{
	ITEM_USE_BAD_SLOT = -1,
	ITEM_USE_WRONG_ITEM = -2,
	ITEM_USE_SUCCESS = 1,
};

// MINIGAME STUFF
enum
{
	kEventFlag_Bingo = 1 << 0,
	kEventFlag_Union = 1 << 1,
	kEventFlag_Poker = 1 << 2,
	kEventFlag_Coin = 1 << 3,
	kEventFlag_Baseball = 1 << 4,
	kEventFlag_GoldenKey = 1 << 5,
	kEventFlag_WeaponRelease = 1 << 6,
	kEventFlag_ShootTheZombie = 1 << 7,
	KEventFlag_MileageAuction = 1 << 10,
	KEventFlag_BonusDisassembly = 1 << 11,
	kEventFlag_Bingo_NEW = 1 << 13,
	kEventFlag_SeasonPass = 1 << 14,
	kEventFlag_Exploration = 1 << 15,
	kEventFlag_HappyNewYear = 1 << 17,
	kEventFlag_ResonanceEnergyShop = 1 << 18,
	kEventFlag_MissionChain = 1 << 19,
	kEventFlag_WebEvent = 1 << 21, // WTF?
};
enum MiniGamePacketType
{
	RequestMiniGameBingo = 0,
	ReuqestMiniGameBaseball = 4,
	RequestMiniGameWeaponRelease = 6,
	RequestMiniGameFindTheZombie = 7,
};

/*WEAPON RELEASE*/
enum MiniGameWeaponReleasePacketType
{
	RequestMiniGameWeaponReleaseUpdate = 60,
	RequestMiniGameWeaponReleaseSetCharacter = 61,
	RequestMiniGameWeaponReleaseGetJoker = 62,
};
struct WeaponReleaseConfigRow
{
	RewardItem item;
	std::string rowName;
};
struct WeaponReleaseConfig
{
	std::vector<WeaponReleaseConfigRow> rows;
	std::vector<unsigned char> characters;
};
struct UserWeaponReleaseRow
{
	int id;
	char progress;
	bool opened;
};
struct UserWeaponReleaseCharacter
{
	char character;
	int count;
};
struct UserWeaponRelease
{
	std::vector<UserWeaponReleaseRow> rows;
	std::vector<UserWeaponReleaseCharacter> characters;
};

/* BINGO */
enum MiniGameBingoPacketType
{
	RequestMiniGameBingoUpdate = 0,
	RequestMiniGameBingoOpenRandomNumber = 1,
	RequestMiniGameBingoReset = 3,
	RequestMiniGameBingoShuffle = 4,
};
enum UserBingoStatus
{
	BINGO_UNINITIALIZED = 0,
	BINGO_IN_PROCCESS = 1,
	BINGO_CLEAN = 2
};
struct UserBingoSlot
{
	int number;
	bool opened;
};
struct UserBingoPrizeSlot
{
	int index;
	RewardItem item;
	bool opened;
	std::vector<int> bingoIndexes; // TODO: do we need it?
};
struct UserBingoOpenNumberResult
{
	int number;
	bool opened;
	std::vector<UserBingoPrizeSlot> prizes;
};
struct UserBingo
{
	std::vector<UserBingoSlot> slots;
	std::vector<UserBingoPrizeSlot> prizes;
	int status;
	bool canPlay;
};

/* ROOM STUFF */
enum RoomTeamNum
{
	Unassigned = 0,
	Terrorist = 1,
	CounterTerrorist = 2,
};
enum RoomReadyStatus
{
	READY_STATUS_NO = 0,
	READY_STATUS_INGAME = 1,
	READY_STATUS_YES = 2,
};
enum RoomGamemode
{
	GAMEMODE_ORIGINAL = 0,
	GAMEMODE_DEATHMATCH = 1,
	GAMEMODE_TEAMDEATHMATCH = 2,
	GAMEMODE_BOT = 3,
	GAMEMODE_BOTDM = 4,
	GAMEMODE_BOTTDM = 5,
	GAMEMODE_OFFICIAL = 6,
	GAMEMODE_OFFICIAL_TIEBREAK = 7,
	GAMEMODE_ZOMBI = 8,
	GAMEMODE_ZOMBI_EXPAND = 9,
	GAMEMODE_CHALLENGE = 12,
	GAMEMODE_ZOMBI_3 = 14,
	GAMEMODE_ZOMBI_SURVIVAL = 15,
	GAMEMODE_SOCCER = 16,
	GAMEMODE_HUMAN_SCENARIO = 17,
	GAMEMODE_TDM_ITEM = 19,
	GAMEMODE_ZOMBI_ESCAPE = 20,
	GAMEMODE_GUNDEATH = 22,
	GAMEMODE_LIGHTORI = 23,
	GAMEMODE_BOTZOMBIE = 24,
	GAMEMODE_SNOWMAN = 27,
	GAMEMODE_ZOMBIESHELTERT = 28,
	GAMEMODE_ZOMBI_4 = 29,
	GAMEMODE_ZOMBIEGIANT = 30,
	GAMEMODE_ZOMBI_EXTERMINATE = 32,
	GAMEMODE_STAND_ALONE = 33,
	GAMEMODE_ZOMBIEOFFICIAL = 35,
	GAMEMODE_ZOMBIEOFFICIAL_TIEBREAK = 36,
	GAMEMODE_ZOMBIETAG = 37,
	GAMEMODE_VOXELCREATE = 38,
	GAMEMODE_VOXELPVE = 39,
	GAMEMODE_ALLSTAR = 40,
	GAMEMODE_PLAYROOM = 41,
	GAMEMODE_SEASONORIGINAL = 42,
	GAMEMODE_SEASONZOMBIEEX = 43,
	GAMEMODE_SEASONZOMBIE_3 = 44,
	GAMEMODE_ZOMBI_3Z = 45,
	GAMEMODE_PLAYROOM2 = 48,
};
enum RoomTeamBalance //òóò òîæå íóæíî ìåíÿòü, â êñíç íå òàê áàëàíñ ðàáîòàåò
{
	Disabled = 0,
	Enabled = 1,
	WithBots = 2,
	ByKadRatio = 4,
};
enum RoomStatus
{
	STATUS_WAITING = 0,
	STATUS_INGAME = 1,
};

/*USER STUFF*/
#define	UFLAG_NAMEPLATE			(1<<0)
#define	UFLAG_GAMENAME			(1<<1)
#define	UFLAG_GAMENAME2			(1<<2)
#define	UFLAG_LEVEL				(1<<3)
#define	UFLAG_UNK2				(1<<4)
#define	UFLAG_EXP				(1<<5)
#define	UFLAG_CASH				(1<<6)
#define	UFLAG_POINTS			(1<<7)
#define	UFLAG_STAT				(1<<8)
#define UFLAG_LOCATION			(1<<9)
#define	UFLAG_CASH2				(1<<10)
#define	UFLAG_UNK5				(1<<11)
#define UFLAG_CLAN				(1<<12)
#define UFLAG_TOURNAMENT		(1<<13)
#define UFLAG_RANK				(1<<14)
#define UFLAG_UNK4				(1<<15)
#define	UFLAG_PASSWORDBOXES		(1<<16)
#define	UFLAG_UNK11				(1<<17)
#define UFLAG_ACHIEVEMENT		(1<<18)
#define UFLAG_ACHIEVEMENTLIST	(1<<19)
#define	UFLAG_UNK13				(1<<20)
#define	UFLAG_UNK14				(1<<21)
#define UFLAG_TITLES			(1<<22)
#define	UFLAG_UNK16				(1<<23)
#define	UFLAG_UNK18				(1<<25)
#define	UFLAG_UNK19				(1<<26)
#define	UFLAG_UNK20				(1<<27)
#define	UFLAG_UNK21				(1<<28)
#define	UFLAG_UNK22				(1<<29)
#define	UFLAG_UNK23				(1<<30)
#define	UFLAG_UNK24				(1<<31)
#define	UFLAG_ALL				(-1)

#define	EXT_UFLAG_GAMEMASTER				(1<<0)
#define	EXT_UFLAG_KILLSTOGETGACHAPONITEM	(1<<1)
#define	EXT_UFLAG_NEXTINVENTORYSLOT			(1<<2)
#define	EXT_UFLAG_CONFIG					(1<<3)
#define EXT_UFLAG_CURLOADOUT				(1<<4)
#define EXT_UFLAG_CHARACTERID				(1<<5)
#define EXT_UFLAG_BANSETTINGS				(1<<6)
#define EXT_UFLAG_2NDPASSWORD				(1<<7)
#define EXT_UFLAG_SECURITYQNA				(1<<8)
#define EXT_UFLAG_ZBRESPAWNEFFECT			(1<<9)
#define EXT_UFLAG_KILLERMARKEFFECT			(1<<10)

#define	UDATA_FLAG_USERNAME			(1<<0)
#define	UDATA_FLAG_PASSWORD			(1<<1)
#define	UDATA_FLAG_REGISTERTIME		(1<<2)
#define	UDATA_FLAG_REGISTERIP		(1<<3)
#define	UDATA_FLAG_FIRSTLOGONTIME	(1<<4)
#define	UDATA_FLAG_LASTLOGONTIME	(1<<5)
#define	UDATA_FLAG_LASTIP			(1<<6)
#define	UDATA_FLAG_LASTHWID			(1<<7)

struct UserNetworkConfig_s
{
	std::string m_szExternalIpAddress;
	int m_nExternalClientPort;
	int m_nExternalServerPort;

	std::string m_szLocalIpAddress;
	int m_nLocalClientPort;
	int m_nLocalServerPort;
};
enum UserStatus
{
	STATUS_MENU = 0,
	STATUS_INROOM = 1,
	STATUS_PLAYING = 2
};
struct UserRestoreData
{
	int channelServerID;
	int channelID;
};

/*SHOP STUFF*/
struct SubProduct
{
	int productID;
	std::vector<RewardItem> items;
	int price;
	int additionalPoints;
	int additionalClanPoints;
	int adType;
};

struct Product
{
	int relationProductID;
	bool isPoints;
	std::vector<SubProduct> subProducts;
};
enum ShopPacketType
{
	UpdateProducts = 0,
	RequestBuyProduct = 1,
	RequestUpdate = 3,
	BuyReply = 3,
	UpdateRecommendedProducts = 15,
	UpdatePopularProducts = 16,
};
enum ShopBuyProductReply
{
	BUY_FAIL_SYSTEM_ERROR = -1,
	BUY_OK = 1,
	BUY_FAIL_NOITEM = 2,
	BUY_FAIL_NO_POINT = 3,
	BUY_FAIL_INVENTORY_FULL = 4
};

enum EnhanceStatus
{
	ENHANCE_SUCCESS = 0,
	ENHANCE_FAILURE = 1,
	ENHANCE_INSERT_EXP = 2,
	ENHANCE_LEVELDOWN = 3,
	ENHANCE_FULLLEVELDOWN = 4,
	ENHANCE_SYSTEMERROR = 5,
};

struct EnhResult
{
	int itemSlot;
	int enhAttribute;
	int enhLevel;
	EnhanceStatus status;
};

enum EnhItemType
{
	ENH_NOR = 5203,
	ENH_ADV = 5204,
	ENH_ANTI = 5205,
	ENH_ANTIDAMAGE = 5206,
	ENH_ANTIWEIGHT = 5207,
	ENH_ANTIACCURACY = 5208,
	ENH_ANTIFIRERATE = 5209,
	ENH_ANTIKICKBACK = 5210,
	ENH_ANTIAMMO = 5211,
	ENH_PER_PART_DAMAGE = 5212,
	ENH_PER_PART_WEIGHT = 5213,
	ENH_PER_PART_ACCURACY = 5214,
	ENH_PER_PART_FIRERATE = 5215,
	ENH_PER_PART_KICKBACK = 5216,
	ENH_PER_PART_AMMO = 5217,
	ENH_PER_DAMAGE = 5218,
	ENH_PER_WEIGHT = 5219,
	ENH_PER_ACCURACY = 5220,
	ENH_PER_FIRERATE = 5221,
	ENH_PER_KICKBACK = 5222,
	ENH_PER_AMMO = 5223,
	ENH_ANIT1GRADE = 5250
};

class CUserData
{
public:
	int flag;

	int userID;
	std::string userName;
	std::string gameName;
	std::string password;
	int registerTime;
	std::string registerIP;
	int firstLogonTime;
	int lastLogonTime;
	std::string lastIP;
	std::vector<unsigned char> lastHWID;
};

class CUserCharacter
{
public:
	int flag;
	int statFlag;
	int achievementFlag;

	std::string gameName;
	int level;
	int64_t exp;
	int64_t cash;
	int64_t points;
	int battles;
	int win;
	int kills;
	int deaths;
	std::string regionName; // TODO: add to table
	int nation;
	int city;
	int town;
	int leagueID;
	int tier[4];
	int passwordBoxes;
	int honorPoints;
	int prefixId;
	std::vector<int> achievementList;
	std::vector<int> titles;
	int clanID;
	int clanMarkID;
	std::string clanName;
	int tournament;
	int banSettings;
	int mileagePoints;
	int nameplateID;
};

// internal user data(last login time, config data etc)
class CUserCharacterExtended
{
public:
	CUserCharacterExtended()
	{

	}

	CUserCharacterExtended(int _flag)
	{
		flag = _flag;
	}

	void Reset()
	{
		flag = 0;
		gameMaster = false;
	}

	int flag;

	bool gameMaster;
	int killsToGetGachaponItem;
	int nextInventorySlot;
	std::vector<int> expiryNotices;
	std::vector<int> rewardsNotices;
	std::vector<RewardNotice> customRewardsNotices;
	std::vector<unsigned char> config;
	std::vector<UserQuestProgress> questsProgress;
	int curLoadout;
	int characterID;
	int banSettings;
	std::vector<unsigned char> _2ndPassword;
	int securityQuestion;
	std::vector<unsigned char> securityAnswer;
	int zbRespawnEffect;
	int killerMarkEffect;
};

class CUserQuestStats
{
	int dailyMissionsCompletedToday;
	int dailyMissionsCleared;
};

// UserSession table + some additional info from usercharacter
struct UserSession
{
	int userID;
	std::string userName;
	std::string gameName;
	std::string ip;
	int status;
	int uptime;
};

// like UserSession struct but server-side (without accessing db)
struct Session
{
	int clientID;
	int userID;
	std::string userName;
	std::string ip;
	std::vector<unsigned char> hwid;
	int status;
	int uptime;
	int roomID;
};

enum ClanPacketType
{
	RequestClanList = 0,
	RequestClanInfo,
	RequestClanCreate,
	RequestClanJoin,
	RequestClanCancelJoin,
	RequestClanJoinApprove,
	RequestClanJoinResult,
	RequestClanLeave,
	RequestClanInvite,
	RequestClanChangeMemberGrade = 11,
	RequestClanKickMember = 12,
	RequestClanUpdateConfig = 14,
	RequestClanUpdateMark = 15,
	RequestClanSetNotice = 16,
	RequestClanGiveItem = 24,
	RequestClanGetItem = 25,
	RequestClanStorage = 26,
	RequestClanDissolve = 27,
	RequestClanDelegateMaster = 28,
	RequestClanMemberUserList = 29,
	RequestClanJoinUserList = 30,
	RequestClanDeleteItem = 31,
	ClanUpdateNoticeMsg = 101,
	ClanKicked = 106,
	ClanUpdate = 107,
	ClanUserList = 109,
	ClanInvite = 112,
	ClanChatMessage = 113,
	RequestClanAchievement = 161,
	RequestClanInactiveAccounts = 169,
};
struct ClanChronicle
{
	int date;
	int type;
	std::string str;
};
struct Clan_s
{
	int id;
	std::string name;
	std::string description;
	std::string clanMaster;
	int masterUserID;
	int time;
	int gameModeID;
	int mapID;
	int region;
	int memberCount;
	int maxMemberCount;
	int joinMethod;
	int onlineMembers;
	int expBoost;
	int pointBoost;
	std::string noticeMsg;
	int score;
	int markChangeCount;
	int markID;
	int markColor;
	std::vector<RewardItem> lastStorageItems;
	std::vector<ClanChronicle> chronicle;
};
struct ClanList_s
{
	int id;
	std::string name;
	std::string clanMaster;
	int time;
	int gameModeID;
	int mapID;
	int region;
	int memberCount;
	int joinMethod;
	int score;
	int markID;
	std::string noticeMsg;
};
struct ClanCreateConfig
{
	std::string name;
	std::string description;
	std::string clanMaster;
	std::string noticeMsg;
	int masterUserID;
	int points;
	int time;
	int gameModeID;
	int mapID;
	int region;
	int joinMethod;
	int onlineMembers;
	int expBoost;
	int pointBoost;
	int markChangeCount;
};
struct ClanStoragePage
{
	int pageID;
	std::vector<RewardItem> items;
};
struct ClanStorageHistory
{
};
struct ClanUser
{
	int userID;
	std::string userName;
	CUserCharacter character;
	class IUser* user;
	int memberGrade;
};
struct ClanUserJoinRequest
{
	int userID;
	std::string userName;
	CUserCharacter character;
	std::string inviterGameName;
	int date;
};
#define	CFLAG_NAME				(1<<0)
#define	CFLAG_MASTERUID			(1<<1)
#define	CFLAG_TIME			(1<<2)
#define	CFLAG_GAMEMODEID				(1<<3)
#define	CFLAG_MAPID			(1<<4)
#define	CFLAG_REGION				(1<<5)
#define	CFLAG_JOINMETHOD				(1<<6)
#define	CFLAG_EXPBOOST			(1<<7)
#define	CFLAG_POINTBOOST				(1<<8)
#define	CFLAG_NOTICEMSG				(1<<9)
#define	CFLAG_SCORE				(1<<10)
#define	CFLAG_MARKID				(1<<11)
#define	CFLAG_MARKCOLOR				(1<<12)
#define	CFLAG_ID				(1<<13)
#define	CFLAG_CLANMASTER				(1<<14)
#define	CFLAG_MARKCHANGECOUNT				(1<<15)
#define	CFLAG_MAXMEMBERCOUNT				(1<<16)
#define CFLAG_CHRONICLE				(1<<17)

enum BanPacketType
{
	BanList = 0,
	RequestBanAddNickname = 0,
	RequestBanRemoveNickname = 1,
	BanAddNicknameReply = 1,
	BanRemoveNicknameReply = 2,
	RequestBanSettings = 2,
	BanSettingsReply = 3,
	RequestBanListMaxSize = 4,
	BanListMaxSizeReply = 4,
};

struct UserBan
{
	int banType; // 0 - remove ban, 1 - with msg, 2 - silent
	std::string reason;
	int term; // timestamp, 0 - perm
};

struct UserDailyRewards
{
	int day;
	bool canGetReward;
	std::vector<RewardItem> randomItems;
};

enum SystemReplyMsg
{
	ETC_POLICY_1 = 1,
	ETC_POLICY_2 = 2,
	ETC_POLICY_3 = 3,
	LOTTERY_WIN_PREMIUM = 4,
	LOTTERY_WIN_PREMIUM_NUM = 5,
	LOTTERY_WIN_PREMIUM_PERIOD = 6,
	LOTTERY_WIN_SPREMIUM = 7,
	LOTTERY_WIN_SPREMIUM_NUM = 8,
	LOTTERY_WIN_SPREMIUM_PERIOD = 9,
	MSG_TELL_USER_NOT_FOUND = 10,
	FABRICATION_MAKE_PERMANENT = 11,
	GM_CUT = 12,
	REINFORCE_MAX_SUCCESS = 13,
};

enum InRoomType
{
	NewRoomRequest = 0,
	JoinRoomRequest_OLD = 1,
	LeaveRoomRequest = 2,
	ToggleReadyRequest = 3,
	GameStartRequest = 4,
	RequestUpdateSettings = 5,
	OnCloseResultWindow = 6, // (result confirm)
	KickRequest = 7,
	VoteKick = 8,
	VoteReply = 9,
	OnConnectionFailure = 11,
	UserInviteListRequest = 15,
	SetUserTeamRequest = 17,
	UserInviteRequest = 19,
	RoomListRequest = 22,
	JoinRoomRequest = 23,
	SetZBAddonsRequest = 35,
};

enum UMsgReceiveType
{
	WhisperChat = 0,
	LobbyChat = 1,
	RoomChat = 2,
	ClanChat = 3,
	RoomTeamChat = 4,
	PartyChat = 5,
	ServerYellChat = 6,
	RewardSelect = 67,
};

enum UMsgWhisperType
{
	To = 0,
	From = 1,
};

/* SURVEY */
struct SurveyQuestionTextEntry
{
	int unk;
};
struct SurveyQuestionAnswerCheckBox
{
	int id;
	std::string answer;
};
struct SurveyQuestion
{
	int id;
	std::string question;
	int answerType;
	SurveyQuestionTextEntry answerTextEntry;
	int answerCheckBoxType;
	std::vector<SurveyQuestionAnswerCheckBox> answersCheckBox;
};
struct Survey
{
	int id;
	std::string title;
	std::vector<SurveyQuestion> questions;
};
struct UserSurveyQuestionAnswer
{
	int questionID;
	bool checkBox;
	std::vector<std::string> answers;
};
struct UserSurveyAnswer
{
	int surveyID;
	std::vector<UserSurveyQuestionAnswer> questionsAnswers;
};
enum UserSurveyAnswerResult
{
	ANSWER_OK = 0,
	ANSWER_INVALID,
	ANSWER_DB_ERROR,
};

// ROOM LOW FLAGS
#define	ROOM_LOW_ROOMNAME				(1<<0)
#define	ROOM_LOW_UNK					(1<<1)
#define	ROOM_LOW_CLANBATTLE				(1<<2)
#define	ROOM_LOW_PASSWORD				(1<<3)
#define	ROOM_LOW_LEVELLIMIT				(1<<4)
#define	ROOM_LOW_UNK7					(1<<5)
#define	ROOM_LOW_GAMEMODEID				(1<<6)
#define	ROOM_LOW_MAPID					(1<<7)
#define	ROOM_LOW_MAXPLAYERS				(1<<8)
#define ROOM_LOW_WINLIMIT				(1<<9)
#define ROOM_LOW_KILLLIMIT				(1<<10)
#define	ROOM_LOW_GAMETIME				(1<<11)
#define ROOM_LOW_ROUNDTIME				(1<<12)
#define ROOM_LOW_WEAPONLIMIT			(1<<13)
#define ROOM_LOW_HOSTAGEKILLLIMIT		(1<<14)
#define ROOM_LOW_FREEZETIME				(1<<15)
#define	ROOM_LOW_BUYTIME				(1<<16)
#define ROOM_LOW_DISPLAYNICKNAME		(1<<17)
#define ROOM_LOW_TEAMBALANCE			(1<<18)
#define ROOM_LOW_UNK21					(1<<19)
#define ROOM_LOW_FRIENDLYFIRE			(1<<20)
#define ROOM_LOW_FLASHLIGHT				(1<<21)
#define ROOM_LOW_FOOTSTEPS				(1<<22)
#define ROOM_LOW_UNK25					(1<<23)
#define ROOM_LOW_TKPUNISH				(1<<24)
#define ROOM_LOW_AUTOKICK				(1<<25)
#define ROOM_LOW_UNK28					(1<<26)
#define ROOM_LOW_UNK29					(1<<27)
#define ROOM_LOW_VIEWFLAG				(1<<28)
#define ROOM_LOW_VOICECHAT				(1<<29)
#define ROOM_LOW_STATUS					(1<<30)
#define ROOM_LOW_UNK33					(1<<31)
#define ROOM_LOW_ALL					(-1)

// ROOM LOW-MID FLAGS
#define	ROOM_LOWMID_UNK34				(1<<0)
#define	ROOM_LOWMID_C4TIMER				(1<<1)
#define	ROOM_LOWMID_BOT					(1<<2)
#define	ROOM_LOWMID_KDRULE				(1<<3)
#define	ROOM_LOWMID_STARTINGCASH		(1<<4)
#define	ROOM_LOWMID_MOVINGSHOT			(1<<5)
#define	ROOM_LOWMID_BALLNUMBER			(1<<6)
#define	ROOM_LOWMID_STATUSSYMBOL		(1<<7)
#define	ROOM_LOWMID_RANDOMMAP			(1<<8)
#define ROOM_LOWMID_MAPPLAYLIST			(1<<9)
#define ROOM_LOWMID_MAPPLAYLISTINDEX	(1<<10)
#define ROOM_LOWMID_ENHANCERESTRICT		(1<<11)
#define ROOM_LOWMID_SD					(1<<12)
#define ROOM_LOWMID_ZSDIFFICULTY		(1<<13)
#define ROOM_LOWMID_LEAGUERULE			(1<<14)
#define ROOM_LOWMID_MANNERLIMIT			(1<<15)
#define ROOM_LOWMID_MAPID2				(1<<16)
#define ROOM_LOWMID_ZBLIMIT				(1<<17)
#define ROOM_LOWMID_UNK62				(1<<18)
#define ROOM_LOWMID_UNK63				(1<<20)
#define ROOM_LOWMID_UNK64				(1<<21)
#define ROOM_LOWMID_TEAMSWITCH			(1<<22)
#define ROOM_LOWMID_ZBRESPAWN			(1<<23)
#define ROOM_LOWMID_ZBBALANCE			(1<<24)
#define ROOM_LOWMID_GAMERULE			(1<<25)
#define ROOM_LOWMID_SUPERROOM			(1<<26)
#define ROOM_LOWMID_ISZBCOMPETITIVE		(1<<27)
#define ROOM_LOWMID_ZBAUTOHUNTING		(1<<28)
#define ROOM_LOWMID_INTEGRATEDTEAM		(1<<29)
#define ROOM_LOWMID_UNK73				(1<<30)
#define ROOM_LOWMID_ALL					(-1)

// ROOM HIGH-MID FLAGS
#define	ROOM_HIGHMID_FIREBOMB			(1<<0)
#define	ROOM_HIGHMID_MUTATIONRESTRICT	(1<<1)
#define	ROOM_HIGHMID_MUTATIONLIMIT		(1<<2)
#define	ROOM_HIGHMID_FLOATINGDAMAGESKIN	(1<<3)
#define	ROOM_HIGHMID_PLAYERONETEAM		(1<<4)
#define	ROOM_HIGHMID_WEAPONRESTRICT		(1<<5)
#define	ROOM_HIGHMID_ALL_SAFE			(ROOM_HIGHMID_FIREBOMB | ROOM_HIGHMID_MUTATIONRESTRICT | ROOM_HIGHMID_MUTATIONLIMIT | ROOM_HIGHMID_FLOATINGDAMAGESKIN | ROOM_HIGHMID_PLAYERONETEAM | ROOM_HIGHMID_WEAPONRESTRICT)
#define ROOM_HIGHMID_ALL				(-1)

// ROOM HIGH FLAGS
#define	ROOM_HIGH_UNK77					(1<<31)
#define ROOM_HIGH_ALL_SAFE				(ROOM_HIGH_UNK77)
#define ROOM_HIGH_ALL					(-1)

// ROOM LIST FLAGS
#define	RLFLAG_NAME				(1<<0)
#define	RLFLAG_UNK				(1<<1)
#define	RLFLAG_HASPASSWORD		(1<<2)
#define	RLFLAG_LEVELLIMIT		(1<<3)
#define	RLFLAG_GAMEMODE			(1<<4)
#define	RLFLAG_MAPID			(1<<5)
#define	RLFLAG_PLAYERS			(1<<6)
#define	RLFLAG_MAXPLAYERS		(1<<7)
#define	RLFLAG_WEAPONLIMIT		(1<<8)
#define	RLFLAG_SUPERROOM		(1<<9)
#define RLFLAG_UNK2				(1<<10)
#define RLFLAG_HOSTNETINFO		(1<<11)
#define RLFLAG_CLANBATTLE		(1<<12)
#define RLFLAG_UNK3				(1<<13)
#define RLFLAG_STATUSSYMBOL		(1<<14)
#define RLFLAG_UNK4				(1<<15)
#define RLFLAG_KDRULE			(1<<16)
#define RLFLAG_UNK6				(1<<17)
#define RLFLAG_UNK7				(1<<18)
#define RLFLAG_FRIENDLYFIRE		(1<<19)
#define RLFLAG_UNK9				(1<<20)
#define RLFLAG_RANDOMMAP		(1<<21)
#define RLFLAG_MAPPLAYLIST		(1<<22)
#define RLFLAG_UNK12			(1<<23)
#define RLFLAG_SD				(1<<24)
#define RLFLAG_ZSDIFFICULTY		(1<<25)
#define RLFLAG_LEAGUERULE		(1<<26)
#define RLFLAG_MANNERLIMIT		(1<<27)
#define RLFLAG_ZBLIMIT			(1<<29)
#define RLFLAG_UNK18			(1<<30)
#define RLFLAG_ALL				(-1)

// room list high flags
#define	RLHFLAG_UNK					(1<<0)
#define	RLHFLAG_ISZBCOMPETITIVE		(1<<1)
#define	RLHFLAG_ZBAUTOHUNTING		(1<<2)
#define	RLHFLAG_UNK4				(1<<3)
#define	RLHFLAG_UNK5				(1<<5)
#define	RLHFLAG_FIREBOMB			(1<<7)
#define	RLHFLAG_MUTATIONRESTRICT	(1<<8)
#define	RLHFLAG_MUTATIONLIMIT		(1<<9)
#define	RLHFLAG_UNK9				(1<<10)
#define	RLHFLAG_UNK10				(1<<11)
#define RLHFLAG_WEAPONRESTRICT		(1<<12)
#define RLHFLAG_ALL_SAFE			RLHFLAG_UNK | RLHFLAG_ISZBCOMPETITIVE | RLHFLAG_ZBAUTOHUNTING | RLHFLAG_UNK4 | RLHFLAG_UNK5 | RLHFLAG_FIREBOMB | RLHFLAG_MUTATIONRESTRICT | RLHFLAG_MUTATIONLIMIT | RLHFLAG_UNK9 | RLHFLAG_UNK10 | RLHFLAG_WEAPONRESTRICT // all flags that are supported by the server
#define RLHFLAG_ALL					(-1)

// inventory related
#define LOADOUT_COUNT 12
#define LOADOUT_SLOT_COUNT 4
#define BUYMENU_COUNT 17
#define BUYMENU_SLOT_COUNT 9
#define ZB_COSTUME_SLOT_COUNT_MAX 18

// RANK
enum RankPacketType
{
	RankReply = 0,
	RankInfoReply = 1,
	RequestRankInfo = 1,
	RankLeagueReply = 2,
	RequestRankInRoom = 2,
	RequestRankSearchNickname = 3,
	RequestRankLeague = 4,
	RequestRankLeagueChangePage = 5,
	RequestRankLeagueSearchNickname = 6,
	RequestRankLeagueHallOfFame = 7,
	RequestRankLeagueHallOfFameSearchNickame = 8,
	RequestRankClan = 9,
	RankChangeLeague = 20,
	RankUserInfo = 21,
	unk31 = 31,
	unk32
};
enum RankReplyCode
{
	RankDisable = 0,
	RankTimeOut,
	RankNotFound,
	RankIsquerying,
	RankErrorData,
	RankNotPeriod
};

// LUCKY ITEM
struct ItemBoxRate
{
	double rate;
	int grade;
	std::vector<int> duration;
	std::vector<int> items;
};

struct ItemBox
{
	int itemId;
	double totalRate;
	std::vector<ItemBoxRate> rates;
};

struct ItemBoxItem
{
	int itemBoxItemID;
	int itemId;
	int count;
	int duration;
	int grade;
};

struct ItemBoxOpenResult
{
	int itemBoxItemId;
	std::vector<ItemBoxItem> items;
};

enum ItemBoxError
{
	FAIL_INVENTORY_FULL = 1,
	FAIL_USEITEM = 2,
	FAIL_PAUSED = 3,
	NOT_KEY = 4,
};

enum ItemBoxGrades
{
	DEFAULT = 1,
	NORMAL = 2,
	ADVANCED = 3,
	PREMIUM = 4,
};

// database defines
#define LOGIN_DB_ERROR 0 
#define LOGIN_OK 1
#define LOGIN_NO_SUCH_USER -1
#define LOGIN_USER_ALREADY_LOGGED_IN_UID -2
#define LOGIN_USER_ALREADY_LOGGED_IN_UUID -3
#define LOGIN_USER_BANNED -4
#define LOGIN_USER_INVALID_CLIENT_VERSION -5
#define LOGIN_SERVER_IS_FULL -6
#define LOGIN_SERVER_CANNOT_VALIDATE_CLIENT -7

#define REGISTER_DB_ERROR 0 
#define REGISTER_OK 1
#define REGISTER_USERNAME_EXIST -1
#define REGISTER_USERNAME_WRONG -2
#define REGISTER_PASSWORD_WRONG -3
#define REGISTER_IP_LIMIT -4

// Dedicated Server
enum HostServerPacketType
{
	AddServer = 0,
	StopServer = 0,
	MemoryUsage = 1,
	TransferServer = 1,
	Unk2 = 2,
	Unk3 = 3,
};