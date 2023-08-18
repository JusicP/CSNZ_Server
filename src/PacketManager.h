#pragma once

#include "SendPacket.h"
#include "User.h"
#include "LuckyItemManager.h"
#include "ShopManager.h"
#include "XZip.h"
#include "Quest.h"

struct Notice_s;

struct BinMetadata
{
	void* buffer;
	int size;
};

class CPacketManager
{
public:
	CPacketManager();
	~CPacketManager();

	CSendPacket* CreatePacket(CExtendedSocket* socket, int msgID);

	void SendUMsgNoticeMsgBoxToUuid(CExtendedSocket* socket, std::string text);
	void SendUMsgNoticeMessageInChat(CExtendedSocket* socket, std::string text);
	void SendUMsgChatMessage(CExtendedSocket* socket, int type, std::string gameName, std::string text, bool from = false);
	void SendUMsgWhisperMessage(CExtendedSocket* socket, std::string msg, std::string destName, CUser* user, int type); // use SendUMsgChatMessage
	void SendUMsgRoomMessage(CExtendedSocket* socket, std::string senderName, std::string text); // use SendUMsgChatMessage
	void SendUMsgRoomTeamMessage(CExtendedSocket* socket, std::string senderName, std::string text); // use SendUMsgChatMessage
	void SendUMsgSystemReply(CExtendedSocket* socket, int type, std::string msg, std::vector<std::string> additionalText = {});
	void SendUMsgLobbyMessage(CExtendedSocket* socket, std::string senderName, std::string text); // use SendUMsgChatMessage
	void SendUMsgNotice(CExtendedSocket* socket, Notice_s& notice, bool unk = 1);
	void SendUMsgExpiryNotice(CExtendedSocket* socket, std::vector<int>& expiryItems);
	void SendUMsgRewardNotice(CExtendedSocket* socket, RewardNotice& reward, std::string title = "", std::string description = "", bool inGame = false, bool scen = false);
	void SendUMsgRewardSelect(CExtendedSocket* socket, Reward* reward);

	void SendServerList(CExtendedSocket* socket);

	void SendStatistic(CExtendedSocket* socket);

	void SendInventoryAdd(CExtendedSocket* socket, std::vector<CUserInventoryItem>& items, int curSlot = 0);
	void SendInventoryRemove(CExtendedSocket* socket, std::vector<CUserInventoryItem>& items, bool gameSlot = true);

	void SendDefaultItems(CExtendedSocket* socket, std::vector<CUserInventoryItem>& items);

	void SendVersion(CExtendedSocket* socket, int result);

	void SendUserStart(CExtendedSocket* socket, int userID, std::string userName, std::string gameName, bool firstConnect);
	void SendUserUpdateInfo(CExtendedSocket* socket, CUser* user, const CUserCharacter& character);
	void SendUserSurvey(CExtendedSocket* socket, Survey& survey);
	void SendUserSurveyReply(CExtendedSocket* socket, int result);

	void SendOption(CExtendedSocket* socket, std::vector<unsigned char>& config);
	void SendOptionUnk(CExtendedSocket* socket);
	void SendOptionUnk2(CExtendedSocket* socket);
	void SendOptionUnk3(CExtendedSocket* socket);

	void SendMetadataMaplist(CExtendedSocket* socket);
	void SendMetadataClientTable(CExtendedSocket* socket);
	void SendMetadataWeaponParts(CExtendedSocket* socket);
	void SendMetadataModelist(CExtendedSocket* socket);
	void SendMetadataMatchOption(CExtendedSocket* socket);
	void SendMetadataItemBox(CExtendedSocket* socket, std::vector<ItemBoxItem>& items);
	void SendMetadataEncyclopedia(CExtendedSocket* socket);
	void SendMetadataGameModeList(CExtendedSocket* socket);
	void SendMetadataReinforceMaxLvl(CExtendedSocket* socket);
	void SendMetadataReinforceMaxEXP(CExtendedSocket* socket);
	void SendMetadataUnk8(CExtendedSocket* socket);
	void SendMetadataProgressUnlock(CExtendedSocket* socket);
	void SendMetadataWeaponPaint(CExtendedSocket* socket);
	void SendMetadataUnk3(CExtendedSocket* socket);
	void SendMetadataReinforceItemsExp(CExtendedSocket* socket);
	void SendMetadataItemExpireTime(CExtendedSocket* socket);
	void SendMetadataUnk20(CExtendedSocket* socket);
	void SendMetadataUnk15(CExtendedSocket* socket);
	void SendMetadataRandomWeaponList(CExtendedSocket* socket);
	void SendMetadataHash(CExtendedSocket* socket);
	void SendMetadataUnk31(CExtendedSocket* socket);
	void SendMetadataHonorMoneyShop(CExtendedSocket* socket);
	void SendMetadataScenarioTX_Common(CExtendedSocket* socket);
	void SendMetadataScenarioTX_Dedi(CExtendedSocket* socket);
	void SendMetadataShopItemList_Dedi(CExtendedSocket* socket);
	void SendMetadataZBCompetitive(CExtendedSocket* socket);
	void SendMetadataUnk43(CExtendedSocket* socket);
	void SendMetadataUnk49(CExtendedSocket* socket);
	void SendMetadataWeaponProp(CExtendedSocket* socket);
	void SendMetadataPPSystem(CExtendedSocket* socket);
	void SendMetadataCodisData(CExtendedSocket* socket);
	void SendMetadataItem(CExtendedSocket* socket);

	void SendGameMatchInfo(CExtendedSocket* socket);
	void SendGameMatchUnk(CExtendedSocket* socket);
	void SendGameMatchUnk9(CExtendedSocket* socket);

	void SendReply(CExtendedSocket* socket, int type);

	void SendItemUnk1(CExtendedSocket* socket);
	void SendItemUnk3(CExtendedSocket* socket);
	void SendItemEquipTattoo(CExtendedSocket* socket);
	void SendItemDailyRewardsUpdate(CExtendedSocket* socket, UserDailyRewards& dailyRewards);
	void SendItemDailyRewardsSpinResult(CExtendedSocket* socket, RewardItem& item);
	void SendItemOpenDecoderResult(CExtendedSocket* socket, ItemBoxOpenResult& result);
	void SendItemOpenDecoderErrorReply(CExtendedSocket* socket, ItemBoxError code);
	void SendItemEnhanceResult(CExtendedSocket* socket, EnhResult& result);
	void SendItemWeaponPaintReply(CExtendedSocket* socket);
	void SendItemPartCheck(CExtendedSocket* socket, int slot, int partNum);
	void SendItemGachapon(CExtendedSocket* socket, int gachaponItem);

	void SendLobbyJoin(CExtendedSocket* socket, CChannel* channel);
	void SendLobbyUserJoin(CExtendedSocket* socket, CUser* joinedUser);
	void SendLobbyUserLeft(CExtendedSocket* socket, CUser* user);

	void SendRoomListFull(CExtendedSocket* socket, std::vector<CRoom*>& rooms);
	void SendRoomListAdd(CExtendedSocket* socket, CRoom* room);
	void SendRoomListUpdate(CExtendedSocket* socket, CRoom* room);
	void SendRoomListRemove(CExtendedSocket* socket, int roomID);

	void SendShopUpdate(CExtendedSocket* socket, std::vector<Product>& products);
	void SendShopBuyProductReply(CExtendedSocket* socket, int replyCode);
	void SendShopReply(CExtendedSocket* socket, int replyCode);
	void SendShopRecommendedProducts(CExtendedSocket* socket, std::vector<std::vector<int>>& products);
	void SendShopPopularProducts(CExtendedSocket* socket, std::vector<int>& products);
	
	void SendSearchRoomNotice(CExtendedSocket* socket, CRoom* room, std::string invitersGameName, std::string inviteMsg);

	void SendRoomCreateAndJoin(CExtendedSocket* socket, CRoom* roomInfo);
	void SendRoomPlayerJoin(CExtendedSocket* socket, CUser* user, RoomTeamNum num);
	void SendRoomUpdateSettings(CExtendedSocket* socket, CRoomSettings* newSettings, int low = 0, int lowMid = 0, int highMid = 0, int high = 0);
	void SendRoomSetUserTeam(CExtendedSocket* socket, CUser* user, int teamNum);
	void SendRoomSetPlayerReady(CExtendedSocket* socket, CUser* user, RoomReadyStatus readyStatus);
	void SendRoomSetHost(CExtendedSocket* socket, CUser* user);
	void SendRoomPlayerLeave(CExtendedSocket* socket, int userId);
	void SendRoomPlayerLeaveIngame(CExtendedSocket* socket);
	void SendRoomInviteUserList(CExtendedSocket* socket, CUser* user);
	void SendRoomGameResult(CExtendedSocket* socket, CRoom* room, CGameMatch* match);
	void SendRoomKick(CExtendedSocket* socket, int userID);
	void SendRoomInitiateVoteKick(CExtendedSocket* socket, int userID, int destUserID, int reason);
	void SendRoomVoteKickResult(CExtendedSocket* socket, bool kick, int userID, int reason);
	void SendRoomZBAddonSurvey(CExtendedSocket* socket, std::vector<int>& addons);
	
	void SendHostOnItemUse(CExtendedSocket* socket, int userId, int itemId);
	void SendHostServerJoin(CExtendedSocket* socket, int ipAddress, bool bigEndian, int port, int userId);
	void SendHostStop(CExtendedSocket* socket);
	void SendHostLeaveResultWindow(CExtendedSocket* socket);
	void SendHostUserInventory(CExtendedSocket* socket, int userId, std::vector<CUserInventoryItem>& items);
	void SendHostGameStart(CExtendedSocket* socket, int userId);
	void SendHostZBAddon(CExtendedSocket* socket, int userID, std::vector<int>& addons);
	void SendHostJoin(CExtendedSocket* socket, int hostID);
	void SendHostFlyerFlock(CExtendedSocket* socket, int type);
	void SendHostAdBalloon(CExtendedSocket* socket);
	void SendHostRestart(CExtendedSocket* socket, int newHostUserID, bool host, CGameMatch* match);

	void SendCharacter(CExtendedSocket* socket);

	void SendEventAdd(CExtendedSocket* socket, int eventsFlag);
	void SendEventUnk(CExtendedSocket* socket);
	void SendEventUnk2(CExtendedSocket* socket);

	void SendMiniGameBingoUpdate(CExtendedSocket* socket, UserBingo& bingo, std::vector<UserBingoSlot>& slots, std::vector<UserBingoPrizeSlot>& prizes);
	void SendMiniGameWeaponReleaseUpdate(CExtendedSocket* socket, WeaponReleaseConfig& cfg, std::vector<UserWeaponReleaseRow>& rows, std::vector<UserWeaponReleaseCharacter>& characters, int totalCount);
	void SendMiniGameWeaponReleaseSetCharacter(CExtendedSocket* socket, int status, int weaponSlot, int slot, int character, int charLeft);
	void SendMiniGameWeaponReleaseUnk2(CExtendedSocket* socket);
	void SendMiniGameWeaponReleaseIGNotice(CExtendedSocket* socket, char character);

	void SendQuests(CExtendedSocket* socket, int userID, std::vector<CQuest*>& quests, std::vector<UserQuestProgress>& questsProgress, int infoFlag = 0xFFFF, int taskFlag = 0xFF, int rewardFlag = 0xFF, int statFlag = 0xFFFF);
	//void SendQuestUnk(CExtendedSocket* socket);
	void SendQuestUpdateMainInfo(CExtendedSocket* socket, int flag, CQuest* quest, UserQuestProgress& questProgress);
	void SendQuestUpdateTaskInfo(CExtendedSocket* socket, int flag, int questID, CQuestTask* task, UserQuestTaskProgress& taskProgress);
	void SendQuestUpdateRewardInfo(CExtendedSocket* socket, int flag, int questID, QuestReward_s& reward);
	void SendQuestUpdateQuestStat(CExtendedSocket* socket, int flag, int honorPoints, UserQuestStat& stat);

	void SendFavoriteLoadout(CExtendedSocket* socket, int characterItemID, int currentLoadout, CUserLoadout& loadouts);
	void SendFavoriteFastBuy(CExtendedSocket* socket, std::vector<CUserFastBuy>& fastbuy);
	void SendFavoriteBuyMenu(CExtendedSocket* socket, std::vector<CUserBuyMenu>& buyMenu);
	void SendFavoriteBookmark(CExtendedSocket* socket, std::vector<int>& bookmark);

	void SendAlarm(CExtendedSocket* socket, std::vector<Notice_s>& notices);

	void SendQuestUnk1(CExtendedSocket* socket);
	void SendQuestUnk11(CExtendedSocket* socket);
	void SendQuestUnk12(CExtendedSocket* socket);
	void SendQuestUnk13(CExtendedSocket* socket);

	void SendUpdateInfoNicknameChangeReply(CExtendedSocket* socket, int replyCode);

	void SendTitle(CExtendedSocket* socket, int id);

	void SendUDPHostData(CExtendedSocket* socket, bool host, int userID, std::string ipAddress, int port);

	void SendHostServerStop(CExtendedSocket* socket);

	void SendClanList(CExtendedSocket* socket, std::vector<ClanList_s>& clans, int pageID, int pageMax);
	void SendClanInfo(CExtendedSocket* socket, Clan_s& clan);
	void SendClanReply(CExtendedSocket* socket, int replyID, int replyCode, const char* errStr);
	void SendClanJoinReply(CExtendedSocket* socket, int replyCode, const char* errStr);
	void SendClanCreateUserList(CExtendedSocket* socket, std::vector<ClanUser>& users);
	void SendClanUpdateUserList(CExtendedSocket* socket, ClanUser& user, bool remove = false);
	void SendClanStoragePage(CExtendedSocket* socket, ClanStoragePage& clanStoragePage);
	void SendClanStorageHistory(CExtendedSocket* socket);
	void SendClanStorageAccessGrade(CExtendedSocket* socket, std::vector<int>& accessGrade);
	void SendClanStorageReply(CExtendedSocket* socket, int replyCode, const char* errStr);
	void SendClanCreateMemberUserList(CExtendedSocket* socket, std::vector<ClanUser>& users);
	void SendClanUpdateMemberUserList(CExtendedSocket* socket, ClanUser& user, bool remove = false);
	void SendClanCreateJoinUserList(CExtendedSocket* socket, std::vector<ClanUserJoinRequest>& users);
	void SendClanUpdateJoinUserList(CExtendedSocket* socket, ClanUserJoinRequest& user, bool remove = false);
	void SendClanDeleteJoinUserList(CExtendedSocket* socket);
	void SendClanUpdate(CExtendedSocket* socket, int type, int memberGrade, Clan_s& clan);
	void SendClanUpdateNotice(CExtendedSocket* socket, Clan_s& clan);
	void SendClanMarkColor(CExtendedSocket* socket);
	void SendClanMarkReply(CExtendedSocket* socket, int replyCode, const char* errStr);
	void SendClanInvite(CExtendedSocket* socket, std::string inviterGameName, int clanID);
	void SendClanMasterDelegate(CExtendedSocket* socket);
	void SendClanKick(CExtendedSocket* socket);
	void SendClanChatMessage(CExtendedSocket* socket, std::string gameName, std::string message);

	void SendBanList(CExtendedSocket* socket, std::vector<UserBanList>& banList);
	void SendBanUpdateList(CExtendedSocket* socket, std::string gameName, bool remove = false);
	void SendBanSettings(CExtendedSocket* socket, int settings);

	void SendMessengerUserInfo(CExtendedSocket* socket, int userID, CUserCharacter& character);

	void SendRankReply(CExtendedSocket* socket, int replyCode);
	void SendRankUserInfo(CExtendedSocket* socket, int userID, CUserCharacter& character);

	void SendAddonPacket(CExtendedSocket* socket, std::vector<int>& addons);

private:
	BinMetadata* LoadBinaryMetadata(const char* fileName);

	HZIP m_hMapListZip;
	HZIP m_hClientTableZip;
	HZIP m_hWeaponPartsZip;
	HZIP m_hMatchingZip;
	HZIP m_hProgressUnlockZip;
	HZIP m_hGameModeListZip;
	HZIP m_hReinforceMaxLvlZip;
	HZIP m_hReinforceMaxExpZip;
	HZIP m_hItemExpireTimeZip;
	HZIP m_hHonorMoneyShopZip;
	HZIP m_hScenarioTX_CommonZip;
	HZIP m_hScenarioTX_DediZip;
	HZIP m_hShopItemList_DediZip;
	HZIP m_hZBCompetitiveZip;
	HZIP m_hPPSystemZip;
	HZIP m_hItemZip;
	HZIP m_hCodisDataZip;
	HZIP m_hWeaponPropZip;
	BinMetadata* m_pPaintItemList;
	BinMetadata* m_pReinforceItemsExp;
	BinMetadata* m_pRandomWeaponList;
	BinMetadata* m_pUnk3;
	BinMetadata* m_pUnk8;
	BinMetadata* m_pUnk15;
	BinMetadata* m_pUnk20;
	BinMetadata* m_pUnk31;
	BinMetadata* m_pUnk43;
	BinMetadata* m_pUnk49;
};
