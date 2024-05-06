#pragma once

#include "interface/ipacketmanager.h"
#include "manager.h"

#include "net/sendpacket.h"
#include "user/user.h"
#include "quest/quest.h"

struct Notice_s;

class CBinMetadata
{
public:
	CBinMetadata(void* buf, size_t bufsize)
	{
		m_pBuf = buf;
		m_nBufsize = bufsize;
	}

	~CBinMetadata()
	{
		if (m_pBuf)
			free(m_pBuf);
	}

	void* GetBuf()
	{
		return m_pBuf;
	}

	size_t GetBufSize()
	{
		return m_nBufsize;
	}

private:
	void* m_pBuf;
	size_t m_nBufsize;
};

class CPacketManager : public CBaseManager<IPacketManager>
{
public:
	CPacketManager();
	~CPacketManager();

	virtual bool Init();
	virtual void Shutdown();

	CSendPacket* CreatePacket(IExtendedSocket* socket, int msgID);

	void SendUMsgNoticeMsgBoxToUuid(IExtendedSocket* socket, const std::string& text);
	void SendUMsgNoticeMessageInChat(IExtendedSocket* socket, const std::string& text);
	void SendUMsgSystemReply(IExtendedSocket* socket, int type, const std::string& msg, const std::vector<std::string>& additionalText = {});
	void SendUMsgUserMessage(IExtendedSocket* socket, int type, const std::string& senderName, const std::string& text, int whisperType = 0);
	void SendUMsgNotice(IExtendedSocket* socket, const Notice_s& notice, bool unk = 1);
	void SendUMsgExpiryNotice(IExtendedSocket* socket, const std::vector<int>& expiryItems);
	void SendUMsgRewardNotice(IExtendedSocket* socket, const RewardNotice& reward, std::string title = "", std::string description = "", bool inGame = false, bool scen = false);
	void SendUMsgRewardSelect(IExtendedSocket* socket, Reward* reward);

	void SendServerList(IExtendedSocket* socket);

	void SendStatistic(IExtendedSocket* socket);

	void SendInventoryAdd(IExtendedSocket* socket, const std::vector<CUserInventoryItem>& items, int curSlot = 0);
	void SendInventoryRemove(IExtendedSocket* socket, const std::vector<CUserInventoryItem>& items, bool gameSlot = true);

	void SendDefaultItems(IExtendedSocket* socket, const std::vector<CUserInventoryItem>& items);

	void SendVersion(IExtendedSocket* socket, int result);

	void SendUserStart(IExtendedSocket* socket, int userID, const std::string& userName, const std::string& gameName, bool firstConnect);
	void SendUserUpdateInfo(IExtendedSocket* socket, IUser* user, const CUserCharacter& character);
	void SendUserSurvey(IExtendedSocket* socket, const Survey& survey);
	void SendUserSurveyReply(IExtendedSocket* socket, int result);

	void SendOption(IExtendedSocket* socket, std::vector<unsigned char>& config);
	void SendOptionUnk(IExtendedSocket* socket);
	void SendOptionUnk2(IExtendedSocket* socket);
	void SendOptionUnk3(IExtendedSocket* socket);

	void SendMetadataMaplist(IExtendedSocket* socket);
	void SendMetadataClientTable(IExtendedSocket* socket);
	void SendMetadataWeaponParts(IExtendedSocket* socket);
	void SendMetadataModelist(IExtendedSocket* socket);
	void SendMetadataMatchOption(IExtendedSocket* socket);
	void SendMetadataItemBox(IExtendedSocket* socket, const std::vector<ItemBoxItem>& items);
	void SendMetadataEncyclopedia(IExtendedSocket* socket);
	void SendMetadataGameModeList(IExtendedSocket* socket);
	void SendMetadataReinforceMaxLvl(IExtendedSocket* socket);
	void SendMetadataReinforceMaxEXP(IExtendedSocket* socket);
	void SendMetadataUnk8(IExtendedSocket* socket);
	void SendMetadataProgressUnlock(IExtendedSocket* socket);
	void SendMetadataWeaponPaint(IExtendedSocket* socket);
	void SendMetadataUnk3(IExtendedSocket* socket);
	void SendMetadataReinforceItemsExp(IExtendedSocket* socket);
	void SendMetadataItemExpireTime(IExtendedSocket* socket);
	void SendMetadataUnk20(IExtendedSocket* socket);
	void SendMetadataUnk15(IExtendedSocket* socket);
	void SendMetadataRandomWeaponList(IExtendedSocket* socket);
	void SendMetadataHash(IExtendedSocket* socket);
	void SendMetadataUnk31(IExtendedSocket* socket);
	void SendMetadataHonorMoneyShop(IExtendedSocket* socket);
	void SendMetadataScenarioTX_Common(IExtendedSocket* socket);
	void SendMetadataScenarioTX_Dedi(IExtendedSocket* socket);
	void SendMetadataShopItemList_Dedi(IExtendedSocket* socket);
	void SendMetadataZBCompetitive(IExtendedSocket* socket);
	void SendMetadataUnk43(IExtendedSocket* socket);
	void SendMetadataUnk49(IExtendedSocket* socket);
	void SendMetadataWeaponProp(IExtendedSocket* socket);
	void SendMetadataPPSystem(IExtendedSocket* socket);
	void SendMetadataCodisData(IExtendedSocket* socket);
	void SendMetadataItem(IExtendedSocket* socket);
	void SendMetadataModeEvent(IExtendedSocket* socket);

	void SendGameMatchInfo(IExtendedSocket* socket);
	void SendGameMatchUnk(IExtendedSocket* socket);
	void SendGameMatchUnk9(IExtendedSocket* socket);
	void SendGameMatchFailMessage(IExtendedSocket* socket, int type);

	void SendReply(IExtendedSocket* socket, int type);

	void SendItemUnk1(IExtendedSocket* socket);
	void SendItemUnk3(IExtendedSocket* socket);
	void SendItemEquipTattoo(IExtendedSocket* socket);
	void SendItemDailyRewardsUpdate(IExtendedSocket* socket, const UserDailyRewards& dailyRewards);
	void SendItemDailyRewardsSpinResult(IExtendedSocket* socket, const RewardItem& item);
	void SendItemOpenDecoderResult(IExtendedSocket* socket, const ItemBoxOpenResult& result);
	void SendItemOpenDecoderErrorReply(IExtendedSocket* socket, ItemBoxError code);
	void SendItemEnhanceResult(IExtendedSocket* socket, const EnhResult& result);
	void SendItemWeaponPaintReply(IExtendedSocket* socket);
	void SendItemPartCheck(IExtendedSocket* socket, int slot, int partNum);
	void SendItemGachapon(IExtendedSocket* socket, int gachaponItem);

	void SendLobbyJoin(IExtendedSocket* socket, CChannel* channel);
	void SendLobbyUserJoin(IExtendedSocket* socket, IUser* joinedUser);
	void SendLobbyUserLeft(IExtendedSocket* socket, IUser* user);

	void SendRoomListFull(IExtendedSocket* socket, const std::vector<IRoom*>& rooms);
	void SendRoomListAdd(IExtendedSocket* socket, IRoom* room);
	void SendRoomListUpdate(IExtendedSocket* socket, IRoom* room);
	void SendRoomListRemove(IExtendedSocket* socket, int roomID);

	void SendShopUpdate(IExtendedSocket* socket, const std::vector<Product>& products);
	void SendShopBuyProductReply(IExtendedSocket* socket, int replyCode);
	void SendShopReply(IExtendedSocket* socket, int replyCode);
	void SendShopRecommendedProducts(IExtendedSocket* socket, const std::vector<std::vector<int>>& products);
	void SendShopPopularProducts(IExtendedSocket* socket, const std::vector<int>& products);
	
	void SendSearchRoomNotice(IExtendedSocket* socket, IRoom* room, const std::string& invitersGameName, const std::string& inviteMsg);

	void SendRoomCreateAndJoin(IExtendedSocket* socket, IRoom* roomInfo);
	void SendRoomPlayerJoin(IExtendedSocket* socket, IUser* user, RoomTeamNum num);
	void SendRoomUpdateSettings(IExtendedSocket* socket, CRoomSettings* newSettings, int low = 0, int lowMid = 0, int highMid = 0, int high = 0);
	void SendRoomSetUserTeam(IExtendedSocket* socket, IUser* user, int teamNum);
	void SendRoomSetPlayerReady(IExtendedSocket* socket, IUser* user, RoomReadyStatus readyStatus);
	void SendRoomSetHost(IExtendedSocket* socket, IUser* user);
	void SendRoomPlayerLeave(IExtendedSocket* socket, int userId);
	void SendRoomPlayerLeaveIngame(IExtendedSocket* socket);
	void SendRoomInviteUserList(IExtendedSocket* socket, IUser* user);
	void SendRoomGameResult(IExtendedSocket* socket, IRoom* room, CGameMatch* match);
	void SendRoomKick(IExtendedSocket* socket, int userID);
	void SendRoomInitiateVoteKick(IExtendedSocket* socket, int userID, int destUserID, int reason);
	void SendRoomVoteKickResult(IExtendedSocket* socket, bool kick, int userID, int reason);
	void SendRoomWeaponSurvey(IExtendedSocket* socket, const std::vector<int>& weapons);
	
	void SendHostOnItemUse(IExtendedSocket* socket, int userId, int itemId);
	void SendHostServerJoin(IExtendedSocket* socket, int ipAddress, int port, int userId);
	void SendHostStop(IExtendedSocket* socket);
	void SendHostLeaveResultWindow(IExtendedSocket* socket);
	void SendHostUserInventory(IExtendedSocket* socket, int userId, const std::vector<CUserInventoryItem>& items);
	void SendHostGameStart(IExtendedSocket* socket, int userId);
	void SendHostZBAddon(IExtendedSocket* socket, int userID, const std::vector<int>& addons);
	void SendHostJoin(IExtendedSocket* socket, IUser* host);
	void SendHostFlyerFlock(IExtendedSocket* socket, int type);
	void SendHostAdBalloon(IExtendedSocket* socket);
	void SendHostRestart(IExtendedSocket* socket, int newHostUserID, bool host, CGameMatch* match);

	void SendCharacter(IExtendedSocket* socket);

	void SendEventAdd(IExtendedSocket* socket, int eventsFlag);
	void SendEventUnk(IExtendedSocket* socket);
	void SendEventMainMenuSkin(IExtendedSocket* socket, int skin);

	void SendMiniGameBingoUpdate(IExtendedSocket* socket, const UserBingo& bingo, const std::vector<UserBingoSlot>& slots, const std::vector<UserBingoPrizeSlot>& prizes);
	void SendMiniGameWeaponReleaseUpdate(IExtendedSocket* socket, const WeaponReleaseConfig& cfg, const std::vector<UserWeaponReleaseRow>& rows, const std::vector<UserWeaponReleaseCharacter>& characters, int totalCount);
	void SendMiniGameWeaponReleaseSetCharacter(IExtendedSocket* socket, int status, int weaponSlot, int slot, int character, int charLeft);
	void SendMiniGameWeaponReleaseUnk2(IExtendedSocket* socket);
	void SendMiniGameWeaponReleaseIGNotice(IExtendedSocket* socket, char character);

	void SendQuests(IExtendedSocket* socket, int userID, const std::vector<CQuest*>& quests, const std::vector<UserQuestProgress>& questsProgress, int infoFlag = 0xFFFF, int taskFlag = 0xFF, int rewardFlag = 0xFF, int statFlag = 0xFFFF);
	//void SendQuestUnk(IExtendedSocket* socket);
	void SendQuestUpdateMainInfo(IExtendedSocket* socket, int flag, CQuest* quest, const UserQuestProgress& questProgress);
	void SendQuestUpdateTaskInfo(IExtendedSocket* socket, int flag, int questID, CQuestTask* task, const UserQuestTaskProgress& taskProgress);
	void SendQuestUpdateRewardInfo(IExtendedSocket* socket, int flag, int questID, const QuestReward_s& reward);
	void SendQuestUpdateQuestStat(IExtendedSocket* socket, int flag, int honorPoints, const UserQuestStat& stat);

	void SendFavoriteLoadout(IExtendedSocket* socket, int characterItemID, int currentLoadout, const CUserLoadout& loadouts);
	void SendFavoriteFastBuy(IExtendedSocket* socket, const std::vector<CUserFastBuy>& fastbuy);
	void SendFavoriteBuyMenu(IExtendedSocket* socket, const std::vector<CUserBuyMenu>& buyMenu);
	void SendFavoriteBookmark(IExtendedSocket* socket, const std::vector<int>& bookmark);

	void SendAlarm(IExtendedSocket* socket, const std::vector<Notice_s>& notices);

	void SendQuestUnk1(IExtendedSocket* socket);
	void SendQuestUnk11(IExtendedSocket* socket);
	void SendQuestUnk12(IExtendedSocket* socket);
	void SendQuestUnk13(IExtendedSocket* socket);

	void SendUpdateInfoNicknameChangeReply(IExtendedSocket* socket, int replyCode);

	void SendTitle(IExtendedSocket* socket, int id);

	void SendUDPHostData(IExtendedSocket* socket, bool host, int userID, const std::string& ipAddress, int port);

	void SendHostServerStop(IExtendedSocket* socket);
	void SendHostServerTransfer(IExtendedSocket* socket, const std::string& ipAddress, int port);

	void SendClanList(IExtendedSocket* socket, const std::vector<ClanList_s>& clans, int pageID, int pageMax);
	void SendClanInfo(IExtendedSocket* socket, const Clan_s& clan);
	void SendClanReply(IExtendedSocket* socket, int replyID, int replyCode, const char* errStr);
	void SendClanJoinReply(IExtendedSocket* socket, int replyCode, const char* errStr);
	void SendClanCreateUserList(IExtendedSocket* socket, const std::vector<ClanUser>& users);
	void SendClanUpdateUserList(IExtendedSocket* socket, const ClanUser& user, bool remove = false);
	void SendClanStoragePage(IExtendedSocket* socket, const ClanStoragePage& clanStoragePage);
	void SendClanStorageHistory(IExtendedSocket* socket);
	void SendClanStorageAccessGrade(IExtendedSocket* socket, const std::vector<int>& accessGrade);
	void SendClanStorageReply(IExtendedSocket* socket, int replyCode, const char* errStr);
	void SendClanCreateMemberUserList(IExtendedSocket* socket, const std::vector<ClanUser>& users);
	void SendClanUpdateMemberUserList(IExtendedSocket* socket, const ClanUser& user, bool remove = false);
	void SendClanCreateJoinUserList(IExtendedSocket* socket, const std::vector<ClanUserJoinRequest>& users);
	void SendClanUpdateJoinUserList(IExtendedSocket* socket, const ClanUserJoinRequest& user, bool remove = false);
	void SendClanDeleteJoinUserList(IExtendedSocket* socket);
	void SendClanUpdate(IExtendedSocket* socket, int type, int memberGrade, const Clan_s& clan);
	void SendClanUpdateNotice(IExtendedSocket* socket, const Clan_s& clan);
	void SendClanMarkColor(IExtendedSocket* socket);
	void SendClanMarkReply(IExtendedSocket* socket, int replyCode, const char* errStr);
	void SendClanInvite(IExtendedSocket* socket, const std::string& inviterGameName, int clanID);
	void SendClanMasterDelegate(IExtendedSocket* socket);
	void SendClanKick(IExtendedSocket* socket);
	void SendClanChatMessage(IExtendedSocket* socket, const std::string& gameName, const std::string& message);

	void SendBanList(IExtendedSocket* socket, const std::vector<std::string>& banList);
	void SendBanUpdateList(IExtendedSocket* socket, const std::string& gameName, bool remove = false);
	void SendBanSettings(IExtendedSocket* socket, int settings);
	void SendBanMaxSize(IExtendedSocket* socket, int maxSize);

	void SendMessengerUserInfo(IExtendedSocket* socket, int userID, const CUserCharacter& character);

	void SendRankReply(IExtendedSocket* socket, int replyCode);
	void SendRankUserInfo(IExtendedSocket* socket, int userID, const CUserCharacter& character);

	void SendAddonPacket(IExtendedSocket* socket, const std::vector<int>& addons);

	void SendLeaguePacket(IExtendedSocket* socket);
	void SendLeagueGaugePacket(IExtendedSocket* socket, int gameModeId);

	void SendCrypt(IExtendedSocket* socket, int type, unsigned char* key, unsigned char* iv);

	void SendUpdateInfo(IExtendedSocket* socket);

	void SendPacketFromFile(IExtendedSocket* socket, const std::string& filename);

	void SendKickPacket(IExtendedSocket* socket, int userID);

private:
	CBinMetadata* LoadBinaryMetadata(const char* fileName, bool zip = false);

	CBinMetadata* m_pMapListZip;
	CBinMetadata* m_pClientTableZip;
	CBinMetadata* m_pWeaponPartsZip;
	CBinMetadata* m_pMatchingZip;
	CBinMetadata* m_pProgressUnlockZip;
	CBinMetadata* m_pGameModeListZip;
	CBinMetadata* m_pReinforceMaxLvlZip;
	CBinMetadata* m_pReinforceMaxExpZip;
	CBinMetadata* m_pItemExpireTimeZip;
	CBinMetadata* m_pHonorMoneyShopZip;
	CBinMetadata* m_pScenarioTX_CommonZip;
	CBinMetadata* m_pScenarioTX_DediZip;
	CBinMetadata* m_pShopItemList_DediZip;
	CBinMetadata* m_pZBCompetitiveZip;
	CBinMetadata* m_pPPSystemZip;
	CBinMetadata* m_pItemZip;
	CBinMetadata* m_pCodisDataZip;
	CBinMetadata* m_pWeaponPropZip;
	CBinMetadata* m_pPaintItemList;
	CBinMetadata* m_pReinforceItemsExp;
	CBinMetadata* m_pRandomWeaponList;
	CBinMetadata* m_pUnk3;
	CBinMetadata* m_pUnk8;
	CBinMetadata* m_pUnk15;
	CBinMetadata* m_pUnk20;
	CBinMetadata* m_pUnk31;
	CBinMetadata* m_pUnk43;
	CBinMetadata* m_pUnk49;
	CBinMetadata* m_pModeEventZip;
};

extern CPacketManager g_PacketManager;